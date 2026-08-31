#include "line_follow.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"
#include "wheel_speed_pi.h"

#define SENSOR_HISTORY_MASK (0x07U)

typedef enum {
    LINE_STATE_WAIT_START = 0,
    LINE_STATE_TRACKING,
    LINE_STATE_LOST_SEARCH,
    LINE_STATE_STOPPED
} LineState;

static GPIO_Regs *const g_sensorPorts[LINE_SENSOR_COUNT] = {
    TRACK_OUT1_PORT, TRACK_OUT2_PORT, TRACK_OUT3_PORT,
    TRACK_OUT4_PORT, TRACK_OUT5_PORT
};

static const uint32_t g_sensorPins[LINE_SENSOR_COUNT] = {
    TRACK_OUT1_PIN, TRACK_OUT2_PIN, TRACK_OUT3_PIN,
    TRACK_OUT4_PIN, TRACK_OUT5_PIN
};

/* 参考开源项目的非线性位置权重，适配为五路数字传感器。 */
static const float g_sensorPosition[LINE_SENSOR_COUNT] = {
    -4.0f, -2.0f, 0.0f, 2.0f, 4.0f
};

static LineState g_state;
static uint8_t g_sensorHistory[LINE_SENSOR_COUNT];
static uint8_t g_pattern;
static uint8_t g_rawHighPattern;
static uint8_t g_startSamples;
static uint8_t g_markerSamples;
static uint8_t g_lostSamples;
static uint16_t g_lostElapsedMs;
static bool g_lineVisible;
static float g_lastVisibleError;
static float g_filteredError;
static float g_previousError;
static float g_filteredDerivative;
static float g_steeringCommand;
static float g_speedCommand;
static int8_t g_lastTurnSign;

volatile uint8_t g_lineRunStateDebug;
volatile uint8_t g_linePatternDebug;
volatile uint8_t g_lineEdgeDebug;
volatile uint8_t g_lineActiveCountDebug;
volatile uint8_t g_lineMarkerDebug;
volatile int16_t g_lineErrorX1000Debug;
volatile int16_t g_lineSteeringX10Debug;
volatile uint16_t g_lineSpeedCommandX10Debug;
volatile uint16_t g_lineLostElapsedMs;
volatile uint16_t g_lineLostLeftCommandX10;
volatile uint16_t g_lineLostRightCommandX10;

static float absoluteFloat(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float clampFloat(float value, float minimum, float maximum)
{
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

static float slewFloat(float current, float target, float maximumStep)
{
    float difference = target - current;

    if (difference > maximumStep) difference = maximumStep;
    if (difference < -maximumStep) difference = -maximumStep;
    return current + difference;
}

static bool sensorPinHigh(uint8_t index)
{
    return (DL_GPIO_readPins(g_sensorPorts[index],
        g_sensorPins[index]) & g_sensorPins[index]) != 0U;
}

static bool sensorSeesBlack(uint8_t index)
{
    bool pinHigh = sensorPinHigh(index);
    return TRACK_ACTIVE_LOW ? !pinHigh : pinHigh;
}

static bool filterSensor(uint8_t index, bool black)
{
    uint8_t history;
    uint8_t activeSamples;

    history = (uint8_t) (((g_sensorHistory[index] << 1) |
        (black ? 1U : 0U)) & SENSOR_HISTORY_MASK);
    g_sensorHistory[index] = history;
    activeSamples = (uint8_t) ((history & 1U) +
        ((history >> 1) & 1U) + ((history >> 2) & 1U));
    return activeSamples >= 2U;
}

static void updateDebug(void)
{
    g_lineRunStateDebug = (uint8_t) g_state;
    g_linePatternDebug = g_pattern;
    g_lineErrorX1000Debug = (int16_t) (g_filteredError * 1000.0f);
    g_lineSteeringX10Debug = (int16_t) (g_steeringCommand * 10.0f);
    g_lineSpeedCommandX10Debug =
        (uint16_t) (g_speedCommand * 10.0f);
    g_lineLostElapsedMs = g_lostElapsedMs;
}

static void stopOutput(void)
{
    g_speedCommand = 0.0f;
    g_steeringCommand = 0.0f;
    g_lineLostLeftCommandX10 = 0U;
    g_lineLostRightCommandX10 = 0U;
    WheelSpeedPI_Reset();
    updateDebug();
}

static void outputTargets(float leftTarget, float rightTarget)
{
    leftTarget *= (float) MOTOR_LEFT_GAIN_PERCENT / 100.0f;
    rightTarget *= (float) MOTOR_RIGHT_GAIN_PERCENT / 100.0f;
    leftTarget = clampFloat(leftTarget, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    rightTarget = clampFloat(rightTarget, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    WheelSpeedPI_Update(leftTarget, rightTarget);
}

static void outputLostSearch(void)
{
    float outerTarget;
    float innerTarget;
    float leftTarget;
    float rightTarget;

    /* 丢线后只允许减速，不允许搜索动作把车重新加速。 */
    if (g_speedCommand > (float) LINE_LOST_OUTER_SPEED) {
        g_speedCommand = slewFloat(g_speedCommand,
            (float) LINE_LOST_OUTER_SPEED,
            LINE_LOST_DECEL_PER_TICK);
    }
    outerTarget = g_speedCommand;
    innerTarget = clampFloat((float) LINE_LOST_INNER_SPEED,
        0.0f, outerTarget);

    if (g_lastTurnSign < 0) {
        leftTarget = innerTarget;
        rightTarget = outerTarget;
    } else {
        leftTarget = outerTarget;
        rightTarget = innerTarget;
    }

    g_lineLostLeftCommandX10 = (uint16_t) (leftTarget * 10.0f);
    g_lineLostRightCommandX10 = (uint16_t) (rightTarget * 10.0f);
    outputTargets(leftTarget, rightTarget);
}

void LineFollow_Init(void)
{
    WheelSpeedPI_Init();
    LineFollow_CalibrateSensors();
}

void LineFollow_CalibrateSensors(void)
{
    uint8_t index;

    for (index = 0U; index < LINE_SENSOR_COUNT; index++) {
        g_sensorHistory[index] = sensorSeesBlack(index) ?
            SENSOR_HISTORY_MASK : 0U;
    }

    g_pattern = 0U;
    g_rawHighPattern = 0U;
    g_startSamples = 0U;
    g_markerSamples = 0U;
    g_lostSamples = 0U;
    g_lostElapsedMs = 0U;
    g_lineVisible = false;
    g_lastVisibleError = 0.0f;
    g_filteredError = 0.0f;
    g_previousError = 0.0f;
    g_filteredDerivative = 0.0f;
    g_steeringCommand = 0.0f;
    g_speedCommand = 0.0f;
    g_lastTurnSign = 0;
    g_lineEdgeDebug = 0U;
    g_lineActiveCountDebug = 0U;
    g_lineMarkerDebug = 0U;
    g_state = LINE_STATE_WAIT_START;
    stopOutput();
}

LineObservation LineFollow_Read(void)
{
    LineObservation result = {0};
    float weightedSum = 0.0f;
    uint8_t activeCount = 0U;
    uint8_t index;

    g_pattern = 0U;
    g_rawHighPattern = 0U;
    for (index = 0U; index < LINE_SENSOR_COUNT; index++) {
        bool pinHigh = sensorPinHigh(index);
        bool black = TRACK_ACTIVE_LOW ? !pinHigh : pinHigh;

        if (pinHigh) {
            g_rawHighPattern |= (uint8_t) (1U << index);
        }
        if (filterSensor(index, black)) {
            g_pattern |= (uint8_t) (1U << index);
            weightedSum += g_sensorPosition[index];
            activeCount++;
        }
    }

    result.pattern = g_pattern;
    result.lineVisible = activeCount != 0U;
    result.error = result.lineVisible ?
        weightedSum / (float) activeCount : g_lastVisibleError;

    if (result.lineVisible) {
        g_lastVisibleError = result.error;
    }

    if (activeCount == LINE_SENSOR_COUNT) {
        if (g_markerSamples < LINE_MARKER_CONFIRM_SAMPLES) {
            g_markerSamples++;
        }
    } else {
        g_markerSamples = 0U;
    }
    result.marker = g_markerSamples >= LINE_MARKER_CONFIRM_SAMPLES;

    g_lineVisible = result.lineVisible;
    g_lineActiveCountDebug = activeCount;
    g_lineMarkerDebug = result.marker ? 1U : 0U;
    return result;
}

LineObservation LineFollow_Run(uint8_t requestedBaseSpeed)
{
    LineObservation observation = LineFollow_Read();
    float derivative;
    float desiredSteering;
    float targetSpeed;
    float minimumSpeed;
    float minimumInnerTarget;
    float turnAmount;
    float leftTarget;
    float rightTarget;
    float speedDecelStep;
    bool leftEdge;
    bool rightEdge;
    bool leftPreturn;
    bool rightPreturn;
    bool edgeDetected;
    bool preturnDetected;

    requestedBaseSpeed = (uint8_t) clampFloat(
        (float) requestedBaseSpeed, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);

    if (g_state == LINE_STATE_STOPPED) {
        stopOutput();
        return observation;
    }

    if (g_state == LINE_STATE_WAIT_START) {
        if (observation.lineVisible &&
            (observation.marker ||
             (absoluteFloat(observation.error) <=
              LINE_START_MAX_ABS_ERROR))) {
            if (g_startSamples < LINE_START_CONFIRM_SAMPLES) {
                g_startSamples++;
            }
        } else {
            g_startSamples = 0U;
        }

        if (g_startSamples < LINE_START_CONFIRM_SAMPLES) {
            stopOutput();
            return observation;
        }
        g_state = LINE_STATE_TRACKING;
        g_filteredError = observation.error;
        g_previousError = observation.error;
    }

    if (!observation.lineVisible) {
        if (g_lostSamples < LINE_LOST_CONFIRM_SAMPLES) {
            g_lostSamples++;
            g_speedCommand = slewFloat(g_speedCommand, 0.0f,
                LINE_LOST_DECEL_PER_TICK);
            outputTargets(g_speedCommand, g_speedCommand);
            updateDebug();
            return observation;
        }

        g_state = LINE_STATE_LOST_SEARCH;
        g_lostElapsedMs += (uint16_t) CONTROL_PERIOD_MS;
        if (g_lostElapsedMs >= LINE_LOST_TIMEOUT_MS) {
            g_state = LINE_STATE_STOPPED;
            stopOutput();
            return observation;
        }

        if (g_lastTurnSign == 0) {
            g_lastTurnSign = (g_lastVisibleError < 0.0f) ? -1 : 1;
        }
        outputLostSearch();
        updateDebug();
        return observation;
    }

    if (g_state == LINE_STATE_LOST_SEARCH) {
        g_filteredError = observation.error;
        g_previousError = observation.error;
        g_filteredDerivative = 0.0f;
        g_steeringCommand = 0.0f;
        if (g_speedCommand > (float) LINE_REACQUIRE_SPEED) {
            g_speedCommand = (float) LINE_REACQUIRE_SPEED;
        }
    }

    g_state = LINE_STATE_TRACKING;
    g_lostSamples = 0U;
    g_lostElapsedMs = 0U;
    g_lineLostLeftCommandX10 = 0U;
    g_lineLostRightCommandX10 = 0U;

    leftEdge = (observation.pattern & LINE_LEFT_EDGE_MASK) != 0U;
    rightEdge = (observation.pattern & LINE_RIGHT_EDGE_MASK) != 0U;
    leftPreturn = (observation.pattern & LINE_LEFT_PRETURN_MASK) != 0U;
    rightPreturn = (observation.pattern & LINE_RIGHT_PRETURN_MASK) != 0U;
    edgeDetected = leftEdge || rightEdge;
    preturnDetected = leftPreturn || rightPreturn;

    g_filteredError += (observation.error - g_filteredError) *
        LINE_ERROR_FILTER_ALPHA;
    if (absoluteFloat(g_filteredError) < LINE_ERROR_DEADBAND) {
        g_filteredError = 0.0f;
    }

    derivative = g_filteredError - g_previousError;
    g_previousError = g_filteredError;
    g_filteredDerivative += (derivative - g_filteredDerivative) *
        LINE_D_FILTER_ALPHA;

    desiredSteering = (LINE_PD_KP * g_filteredError) +
        (LINE_PD_KD * g_filteredDerivative);
    desiredSteering = clampFloat(desiredSteering,
        -LINE_STEERING_LIMIT, LINE_STEERING_LIMIT);

    if (leftEdge && !rightEdge) {
        desiredSteering = clampFloat(desiredSteering,
            -LINE_STEERING_LIMIT, -LINE_EDGE_MIN_STEERING);
    } else if (rightEdge && !leftEdge) {
        desiredSteering = clampFloat(desiredSteering,
            LINE_EDGE_MIN_STEERING, LINE_STEERING_LIMIT);
    } else if (leftPreturn && !rightPreturn) {
        desiredSteering = clampFloat(desiredSteering,
            -LINE_STEERING_LIMIT, -LINE_PRETURN_MIN_STEERING);
    } else if (rightPreturn && !leftPreturn) {
        desiredSteering = clampFloat(desiredSteering,
            LINE_PRETURN_MIN_STEERING, LINE_STEERING_LIMIT);
    }

    if (absoluteFloat(g_filteredError) >=
        LINE_DIRECTION_MEMORY_MIN_ERROR) {
        g_lastTurnSign = (g_filteredError < 0.0f) ? -1 : 1;
    }

    g_steeringCommand = slewFloat(g_steeringCommand,
        desiredSteering, edgeDetected ? LINE_EDGE_STEER_SLEW :
        LINE_NORMAL_STEER_SLEW);

    minimumSpeed = (float) LINE_CURVE_SPEED;
    if (minimumSpeed > (float) requestedBaseSpeed) {
        minimumSpeed = (float) requestedBaseSpeed;
    }
    targetSpeed = (float) requestedBaseSpeed -
        (((float) requestedBaseSpeed - minimumSpeed) *
         clampFloat(absoluteFloat(g_filteredError) / 4.0f,
             0.0f, 1.0f));
    if (preturnDetected && (targetSpeed > LINE_PRETURN_SPEED)) {
        targetSpeed = LINE_PRETURN_SPEED;
    }
    if (edgeDetected) targetSpeed = minimumSpeed;

    speedDecelStep = edgeDetected ?
        LINE_EDGE_SPEED_DECEL_PER_TICK : LINE_SPEED_DECEL_PER_TICK;
    g_speedCommand = slewFloat(g_speedCommand, targetSpeed,
        (targetSpeed < g_speedCommand) ? speedDecelStep :
        LINE_SPEED_ACCEL_PER_TICK);

    minimumInnerTarget = g_speedCommand * LINE_INNER_WHEEL_MIN_RATIO;
    if ((g_speedCommand >= LINE_INNER_WHEEL_MIN_SPEED) &&
        (minimumInnerTarget < LINE_INNER_WHEEL_MIN_SPEED)) {
        minimumInnerTarget = LINE_INNER_WHEEL_MIN_SPEED;
    }
    minimumInnerTarget = clampFloat(minimumInnerTarget,
        0.0f, g_speedCommand);
    turnAmount = clampFloat(absoluteFloat(g_steeringCommand),
        0.0f, g_speedCommand - minimumInnerTarget);

    leftTarget = g_speedCommand;
    rightTarget = g_speedCommand;
    if (g_steeringCommand < 0.0f) {
        leftTarget -= turnAmount;
    } else if (g_steeringCommand > 0.0f) {
        rightTarget -= turnAmount;
    }
    outputTargets(leftTarget, rightTarget);

    g_lineEdgeDebug = (leftEdge ? 0x01U : 0U) |
        (rightEdge ? 0x02U : 0U) |
        (leftPreturn ? 0x04U : 0U) |
        (rightPreturn ? 0x08U : 0U);
    updateDebug();
    return observation;
}

void LineFollow_Stop(void)
{
    g_state = LINE_STATE_STOPPED;
    stopOutput();
}

uint8_t LineFollow_GetPattern(void)
{
    return g_pattern;
}

uint8_t LineFollow_GetRawHighPattern(void)
{
    return g_rawHighPattern;
}

bool LineFollow_IsVisible(void)
{
    return g_lineVisible;
}
