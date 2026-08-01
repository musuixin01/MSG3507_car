#include "line_follow.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"
#include "wheel_speed_pi.h"

#define TRACK_SENSOR_COUNT (8U)
#define TRACK_HISTORY_MASK (0x07U)

typedef enum {
    LINE_STATE_WAIT_START = 0,
    LINE_STATE_TRACKING,
    LINE_STATE_LOST_CONFIRM,
    LINE_STATE_LOST_SEARCH,
    LINE_STATE_REACQUIRE,
    LINE_STATE_LOST_STOP
} LineRunState;

static GPIO_Regs *const g_trackPorts[TRACK_SENSOR_COUNT] = {
    TRACK_OUT1_PORT, TRACK_OUT2_PORT, TRACK_OUT3_PORT,
    TRACK_OUT4_PORT, TRACK_OUT5_PORT, TRACK_OUT6_PORT,
    TRACK_OUT7_PORT, TRACK_OUT8_PORT
};

static const uint32_t g_trackPins[TRACK_SENSOR_COUNT] = {
    TRACK_OUT1_PIN, TRACK_OUT2_PIN, TRACK_OUT3_PIN,
    TRACK_OUT4_PIN, TRACK_OUT5_PIN, TRACK_OUT6_PIN,
    TRACK_OUT7_PIN, TRACK_OUT8_PIN
};

/*
 * Adapted from the 2025 reference design's nonlinear position model.
 * The outer sensors carry much more weight than the center sensors,
 * so an edge hit immediately produces a strong correction.
 */
static const float g_trackPosition[TRACK_SENSOR_COUNT] = {
    -30.0f, -15.0f, -5.0f, -2.5f,
      2.5f,   5.0f, 15.0f, 30.0f
};

static LineRunState g_runState;
static bool g_activeLow;
static uint8_t g_sensorHistory[TRACK_SENSOR_COUNT];
static uint8_t g_lastPattern;
static uint8_t g_lastRawHighPattern;
static bool g_lastLineVisible;
static float g_lastVisibleError;
static float g_filteredError;
static float g_controlError;
static float g_previousControlError;
static float g_derivativeError;
static bool g_errorFilterInitialized;
static float g_steeringCommand;
static float g_speedCommand;
static float g_diffCommand;
static float g_lastLeftTarget;
static float g_lastRightTarget;
static uint8_t g_startLineSamples;
static bool g_startLineConfirmed;
static uint8_t g_markerDetectSamples;
static uint8_t g_markerApproachSamples;
static int8_t g_centerCandidateSign;
static uint8_t g_centerCandidateSamples;
static uint8_t g_lostSamples;
static uint8_t g_reacquireSamples;
static uint16_t g_lostElapsedMs;
static uint16_t g_reacquireElapsedMs;
static uint16_t g_reliableDirectionAgeMs;
static int8_t g_lastReliableTurnSign;
static float g_lostLeftCommand;
static float g_lostRightCommand;

volatile uint8_t g_lineRunStateDebug;
volatile uint8_t g_linePatternDebug;
volatile uint8_t g_lineEdgeDebug;
volatile uint8_t g_lineActiveCountDebug;
volatile uint8_t g_lineMarkerDebug;
volatile uint8_t g_lineMarkerApproachSamplesDebug;
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

    if (difference > maximumStep) {
        difference = maximumStep;
    } else if (difference < -maximumStep) {
        difference = -maximumStep;
    }
    return current + difference;
}

static void updateDebugState(void)
{
    g_lineRunStateDebug = (uint8_t) g_runState;
    g_linePatternDebug = g_lastPattern;
    g_lineErrorX1000Debug =
        (int16_t) (g_controlError * 1000.0f);
    g_lineSteeringX10Debug =
        (int16_t) (g_steeringCommand * 10.0f);
    g_lineSpeedCommandX10Debug =
        (uint16_t) (clampFloat(g_speedCommand, 0.0f,
            LINE_MOTOR_HARD_LIMIT_PERCENT) * 10.0f);
    g_lineLostElapsedMs = g_lostElapsedMs;
    g_lineLostLeftCommandX10 =
        (uint16_t) (clampFloat(g_lostLeftCommand, 0.0f,
            LINE_MOTOR_HARD_LIMIT_PERCENT) * 10.0f);
    g_lineLostRightCommandX10 =
        (uint16_t) (clampFloat(g_lostRightCommand, 0.0f,
            LINE_MOTOR_HARD_LIMIT_PERCENT) * 10.0f);
}

static uint8_t logicalToHardwareIndex(uint8_t logicalIndex)
{
    return TRACK_SENSOR_ORDER_REVERSED ?
        (uint8_t) ((TRACK_SENSOR_COUNT - 1U) - logicalIndex) :
        logicalIndex;
}

static bool trackPinHigh(uint8_t logicalIndex)
{
    uint8_t hardwareIndex = logicalToHardwareIndex(logicalIndex);

    return (DL_GPIO_readPins(
        g_trackPorts[hardwareIndex], g_trackPins[hardwareIndex]) &
        g_trackPins[hardwareIndex]) != 0U;
}

static bool rawBlack(uint8_t logicalIndex)
{
    bool pinHigh = trackPinHigh(logicalIndex);
    return g_activeLow ? !pinHigh : pinHigh;
}

static bool filteredBlack(uint8_t logicalIndex, bool black)
{
    uint8_t history;
    uint8_t ones;

    history = (uint8_t) (((g_sensorHistory[logicalIndex] << 1) |
        (black ? 1U : 0U)) & TRACK_HISTORY_MASK);
    g_sensorHistory[logicalIndex] = history;
    ones = (uint8_t) ((history & 1U) +
        ((history >> 1) & 1U) +
        ((history >> 2) & 1U));
    return ones >= 2U;
}

static void clearControllerState(void)
{
    g_lastVisibleError = 0.0f;
    g_filteredError = 0.0f;
    g_controlError = 0.0f;
    g_previousControlError = 0.0f;
    g_derivativeError = 0.0f;
    g_errorFilterInitialized = false;
    g_steeringCommand = 0.0f;
    g_speedCommand = 0.0f;
    g_diffCommand = 0.0f;
    g_lastLeftTarget = 0.0f;
    g_lastRightTarget = 0.0f;
    g_startLineSamples = 0U;
    g_startLineConfirmed = false;
    g_markerDetectSamples = 0U;
    g_markerApproachSamples = 0U;
    g_centerCandidateSign = 0;
    g_centerCandidateSamples = 0U;
    g_lineMarkerApproachSamplesDebug = 0U;
    g_lostSamples = 0U;
    g_reacquireSamples = 0U;
    g_lostElapsedMs = 0U;
    g_reacquireElapsedMs = 0U;
    g_reliableDirectionAgeMs = 0U;
    g_lastReliableTurnSign = 0;
    g_lostLeftCommand = 0.0f;
    g_lostRightCommand = 0.0f;
    g_lineEdgeDebug = 0U;
}

static void stopMotorOutput(void)
{
    g_steeringCommand = 0.0f;
    g_speedCommand = 0.0f;
    g_diffCommand = 0.0f;
    g_lastLeftTarget = 0.0f;
    g_lastRightTarget = 0.0f;
    g_derivativeError = 0.0f;
    g_lineEdgeDebug = 0U;
    WheelSpeedPI_Reset();
    updateDebugState();
}

static void outputTrackingTargets(float speedLimit, bool edgeDetected,
    bool preturnDetected)
{
    float desiredDiff;
    float diffSlewStep;
    float minimumInnerTarget;
    float maximumTurnAmount;
    float turnAmount;
    float leftTarget;
    float rightTarget;

    speedLimit = clampFloat(speedLimit, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    minimumInnerTarget =
        speedLimit * LINE_INNER_WHEEL_MIN_RATIO;
    if ((speedLimit >= LINE_INNER_WHEEL_MIN_PERCENT) &&
        (minimumInnerTarget < LINE_INNER_WHEEL_MIN_PERCENT)) {
        minimumInnerTarget = LINE_INNER_WHEEL_MIN_PERCENT;
    }
    if (minimumInnerTarget > speedLimit) {
        minimumInnerTarget = speedLimit;
    }

    maximumTurnAmount = speedLimit - minimumInnerTarget;
    desiredDiff = clampFloat(g_steeringCommand,
        -maximumTurnAmount, maximumTurnAmount);
    if (edgeDetected) {
        diffSlewStep = LINE_EDGE_DIFF_SLEW_PERCENT_PER_TICK;
    } else if (preturnDetected) {
        diffSlewStep = LINE_PRETURN_DIFF_SLEW_PERCENT_PER_TICK;
    } else if (absoluteFloat(desiredDiff) <
        absoluteFloat(g_diffCommand)) {
        diffSlewStep = LINE_DIFF_RELEASE_SLEW_PERCENT_PER_TICK;
    } else {
        diffSlewStep = LINE_DIFF_SLEW_PERCENT_PER_TICK;
    }
    g_diffCommand = slewFloat(g_diffCommand,
        desiredDiff, diffSlewStep);
    turnAmount = absoluteFloat(g_diffCommand);
    if (turnAmount > maximumTurnAmount) {
        turnAmount = maximumTurnAmount;
    }

    /*
     * Same differential structure as the reference design after
     * desaturation: the outer wheel never exceeds the current speed
     * limit, and only the inner wheel is reduced.
     */
    if (g_diffCommand > 0.0f) {
        leftTarget = speedLimit;
        rightTarget = speedLimit - turnAmount;
    } else if (g_diffCommand < 0.0f) {
        leftTarget = speedLimit - turnAmount;
        rightTarget = speedLimit;
    } else {
        leftTarget = speedLimit;
        rightTarget = speedLimit;
    }

    leftTarget *= (float) MOTOR_LEFT_GAIN_PERCENT / 100.0f;
    rightTarget *= (float) MOTOR_RIGHT_GAIN_PERCENT / 100.0f;
    leftTarget = clampFloat(leftTarget,
        minimumInnerTarget, speedLimit);
    rightTarget = clampFloat(rightTarget,
        minimumInnerTarget, speedLimit);

    g_lastLeftTarget = leftTarget;
    g_lastRightTarget = rightTarget;
    WheelSpeedPI_Update(leftTarget, rightTarget);
}

static void outputLostSearch(void)
{
    float desiredLeftTarget;
    float desiredRightTarget;

    if (g_lastReliableTurnSign > 0) {
        desiredLeftTarget =
            (float) LINE_LOST_SEARCH_OUTER_PERCENT;
        desiredRightTarget =
            (float) LINE_LOST_SEARCH_INNER_PERCENT;
    } else {
        desiredLeftTarget =
            (float) LINE_LOST_SEARCH_INNER_PERCENT;
        desiredRightTarget =
            (float) LINE_LOST_SEARCH_OUTER_PERCENT;
    }

    desiredLeftTarget *=
        (float) MOTOR_LEFT_GAIN_PERCENT / 100.0f;
    desiredRightTarget *=
        (float) MOTOR_RIGHT_GAIN_PERCENT / 100.0f;

    /* A lost-line transition is never allowed to accelerate either wheel. */
    if (desiredLeftTarget > g_lastLeftTarget) {
        desiredLeftTarget = g_lastLeftTarget;
    }
    if (desiredRightTarget > g_lastRightTarget) {
        desiredRightTarget = g_lastRightTarget;
    }

    g_lostLeftCommand = slewFloat(g_lostLeftCommand,
        desiredLeftTarget, LINE_LOST_DECEL_PERCENT_PER_TICK);
    g_lostRightCommand = slewFloat(g_lostRightCommand,
        desiredRightTarget, LINE_LOST_DECEL_PERCENT_PER_TICK);
    g_diffCommand = g_lostLeftCommand - g_lostRightCommand;
    g_steeringCommand = g_diffCommand;
    g_speedCommand =
        (g_lostLeftCommand > g_lostRightCommand) ?
        g_lostLeftCommand : g_lostRightCommand;
    WheelSpeedPI_Update(g_lostLeftCommand, g_lostRightCommand);
}

void LineFollow_Init(void)
{
    uint8_t index;

    g_activeLow = TRACK_ACTIVE_LOW != 0;
    g_lastPattern = 0U;
    g_lastRawHighPattern = 0U;
    g_lastLineVisible = false;
    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        g_sensorHistory[index] = 0U;
    }
    g_runState = LINE_STATE_WAIT_START;
    clearControllerState();
    WheelSpeedPI_Init();
    LineFollow_CalibrateSensors();
}

void LineFollow_CalibrateSensors(void)
{
    uint8_t highCount = 0U;
    uint8_t index;

    stopMotorOutput();
    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        if (trackPinHigh(index)) highCount++;
    }

    if (TRACK_AUTO_POLARITY &&
        (highCount != 0U) &&
        (highCount != TRACK_SENSOR_COUNT)) {
        g_activeLow = highCount >= 5U;
    } else {
        g_activeLow = TRACK_ACTIVE_LOW != 0;
    }

    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        g_sensorHistory[index] = rawBlack(index) ?
            TRACK_HISTORY_MASK : 0U;
    }

    g_runState = LINE_STATE_WAIT_START;
    clearControllerState();
    WheelSpeedPI_Reset();
    updateDebugState();
}

LineObservation LineFollow_Read(void)
{
    LineObservation result = {0};
    float weightedSum = 0.0f;
    float instantaneousError = 0.0f;
    uint8_t activeCount = 0U;
    uint8_t index;

    g_lastRawHighPattern = 0U;
    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        bool pinHigh = trackPinHigh(index);
        bool black = g_activeLow ? !pinHigh : pinHigh;

        if (pinHigh) {
            g_lastRawHighPattern |= (uint8_t) (1U << index);
        }
        if (filteredBlack(index, black)) {
            result.pattern |= (uint8_t) (1U << index);
            weightedSum += g_trackPosition[index];
            activeCount++;
        }
    }

    result.lineVisible = activeCount != 0U;
    if (result.lineVisible) {
        instantaneousError = weightedSum / (float) activeCount;
    }
    /*
     * Accept A only after a short centered normal-line approach. An acute
     * curve can momentarily illuminate every sensor, but it does not have
     * this stable approach history.
     */
    if ((activeCount >= A_MARKER_MIN_ACTIVE_SENSORS) &&
        ((result.pattern & LINE_LEFT_EDGE_SENSOR_MASK) != 0U) &&
        ((result.pattern & LINE_RIGHT_EDGE_SENSOR_MASK) != 0U) &&
        (g_markerApproachSamples >= A_MARKER_APPROACH_SAMPLES)) {
        if (g_markerDetectSamples < A_MARKER_DETECT_SAMPLES) {
            g_markerDetectSamples++;
        }
    } else {
        g_markerDetectSamples = 0U;
    }
    if (activeCount >= A_MARKER_MIN_ACTIVE_SENSORS) {
        /* Keep the valid approach history while the sensor bar crosses A. */
    } else if (result.lineVisible &&
        (activeCount <= A_MARKER_APPROACH_MAX_ACTIVE) &&
        (absoluteFloat(instantaneousError) <=
         A_MARKER_APPROACH_MAX_ABS_ERROR)) {
        if (g_markerApproachSamples < A_MARKER_APPROACH_SAMPLES) {
            g_markerApproachSamples++;
        }
    } else {
        g_markerApproachSamples = 0U;
    }
    result.marker =
        g_markerDetectSamples >= A_MARKER_DETECT_SAMPLES;
    g_lineActiveCountDebug = activeCount;
    g_lineMarkerDebug = result.marker ? 1U : 0U;
    g_lineMarkerApproachSamplesDebug = g_markerApproachSamples;

    if (result.lineVisible) {
        result.error = instantaneousError;
        if (!result.marker) {
            g_lastVisibleError = result.error;
        }
    } else {
        result.error = g_lastVisibleError;
    }

    g_lastPattern = result.pattern;
    g_lastLineVisible = result.lineVisible;
    return result;
}

static bool confirmStart(const LineObservation *observation)
{
    bool validStart = observation->marker ||
        (observation->lineVisible &&
         (absoluteFloat(observation->error) <=
          LINE_START_MAX_ERROR));

    if (validStart) {
        if (g_startLineSamples < LINE_START_CONFIRM_SAMPLES) {
            g_startLineSamples++;
        }
        if (g_startLineSamples >= LINE_START_CONFIRM_SAMPLES) {
            g_startLineConfirmed = true;
            g_runState = LINE_STATE_TRACKING;
            g_speedCommand = 0.0f;
            return true;
        }
    } else {
        g_startLineSamples = 0U;
    }
    return false;
}

static void enterReacquire(void)
{
    g_runState = LINE_STATE_REACQUIRE;
    g_reacquireSamples = 1U;
    g_reacquireElapsedMs = 0U;
    g_errorFilterInitialized = false;
}

static void updateVisibleState(void)
{
    if ((g_runState == LINE_STATE_LOST_CONFIRM) ||
        (g_runState == LINE_STATE_LOST_SEARCH)) {
        enterReacquire();
    } else if (g_runState == LINE_STATE_REACQUIRE) {
        if (g_reacquireSamples < LINE_REACQUIRE_CONFIRM_SAMPLES) {
            g_reacquireSamples++;
        }
        if (g_reacquireElapsedMs < LINE_REACQUIRE_BLEND_MS) {
            g_reacquireElapsedMs += CONTROL_PERIOD_MS;
        }
        if ((g_reacquireSamples >= LINE_REACQUIRE_CONFIRM_SAMPLES) &&
            (g_reacquireElapsedMs >= LINE_REACQUIRE_BLEND_MS)) {
            g_runState = LINE_STATE_TRACKING;
            g_lostSamples = 0U;
            g_lostElapsedMs = 0U;
            g_reacquireSamples = 0U;
            g_reacquireElapsedMs = 0U;
            g_lostLeftCommand = 0.0f;
            g_lostRightCommand = 0.0f;
        }
    } else {
        g_runState = LINE_STATE_TRACKING;
        g_lostSamples = 0U;
        g_lostElapsedMs = 0U;
    }
}

static void runLostState(void)
{
    g_errorFilterInitialized = false;
    g_controlError = 0.0f;
    g_previousControlError = 0.0f;
    g_derivativeError = 0.0f;
    g_lineEdgeDebug = 0U;

    if (g_runState == LINE_STATE_TRACKING) {
        g_runState = LINE_STATE_LOST_CONFIRM;
        g_lostSamples = 1U;
        g_lostElapsedMs = CONTROL_PERIOD_MS;
        g_lostLeftCommand = g_lastLeftTarget;
        g_lostRightCommand = g_lastRightTarget;
    } else if (g_runState == LINE_STATE_REACQUIRE) {
        g_runState = LINE_STATE_LOST_SEARCH;
        g_reacquireSamples = 0U;
        g_reacquireElapsedMs = 0U;
        g_lostLeftCommand = g_lastLeftTarget;
        g_lostRightCommand = g_lastRightTarget;
        g_lostElapsedMs += CONTROL_PERIOD_MS;
    } else if (g_runState == LINE_STATE_LOST_CONFIRM) {
        if (g_lostSamples < LINE_LOST_CONFIRM_SAMPLES) {
            g_lostSamples++;
        }
        g_lostElapsedMs += CONTROL_PERIOD_MS;
        if (g_lostSamples >= LINE_LOST_CONFIRM_SAMPLES) {
            g_runState = (g_lastReliableTurnSign == 0) ?
                LINE_STATE_LOST_STOP : LINE_STATE_LOST_SEARCH;
        }
    } else if (g_runState == LINE_STATE_LOST_SEARCH) {
        g_lostElapsedMs += CONTROL_PERIOD_MS;
    }

    if ((g_runState == LINE_STATE_LOST_SEARCH) &&
        (g_lostElapsedMs >= LINE_LOST_SEARCH_TIMEOUT_MS)) {
        g_runState = LINE_STATE_LOST_STOP;
    }

    if (g_runState == LINE_STATE_LOST_STOP) {
        stopMotorOutput();
    } else if (g_runState == LINE_STATE_LOST_CONFIRM) {
        g_speedCommand =
            (g_lostLeftCommand > g_lostRightCommand) ?
            g_lostLeftCommand : g_lostRightCommand;
        WheelSpeedPI_Update(
            g_lostLeftCommand, g_lostRightCommand);
    } else {
        outputLostSearch();
    }
    updateDebugState();
}

LineObservation LineFollow_Run(uint8_t requestedBaseSpeed)
{
    LineObservation observation = LineFollow_Read();
    float normalizedError;
    float derivativeSample;
    float gainBlend;
    float steeringKp;
    float steeringKd;
    float pidOutput;
    float speedSlewStep;
    float speedTarget;
    float turnSpeedFloor;
    float curveRatio;
    bool firstControlSample = false;
    bool leftEdgeDetected;
    bool rightEdgeDetected;
    bool leftPreturnDetected;
    bool rightPreturnDetected;

    if (!g_startLineConfirmed) {
        (void) confirmStart(&observation);
        if (!g_startLineConfirmed) {
            stopMotorOutput();
            return observation;
        }
    }

    if (g_runState == LINE_STATE_LOST_STOP) {
        stopMotorOutput();
        return observation;
    }

    if (!observation.lineVisible) {
        runLostState();
        return observation;
    }

    updateVisibleState();

    normalizedError = clampFloat(
        observation.error / LINE_ERROR_FULL_SCALE,
        -1.0f, 1.0f);
    /*
     * OUT4 and OUT5 are two discrete samples around the vehicle center.
     * When they alternate every few control ticks, immediately reversing
     * the steering command creates a limit-cycle weave. Confirm a center
     * offset for several ticks before accepting it; a sign change first
     * passes through zero instead of commanding the opposite direction.
     */
    if ((observation.pattern == LINE_CENTER_LEFT_SENSOR_MASK) ||
        (observation.pattern == LINE_CENTER_RIGHT_SENSOR_MASK)) {
        int8_t centerSign =
            (observation.pattern == LINE_CENTER_LEFT_SENSOR_MASK) ? -1 : 1;

        if (centerSign != g_centerCandidateSign) {
            g_centerCandidateSign = centerSign;
            g_centerCandidateSamples = 1U;
        } else if (g_centerCandidateSamples <
            LINE_CENTER_CONFIRM_SAMPLES) {
            g_centerCandidateSamples++;
        }
        if (g_centerCandidateSamples < LINE_CENTER_CONFIRM_SAMPLES) {
            normalizedError = 0.0f;
        }
    } else {
        g_centerCandidateSign = 0;
        g_centerCandidateSamples = 0U;
    }
    leftEdgeDetected =
        (observation.pattern & LINE_LEFT_EDGE_SENSOR_MASK) != 0U;
    rightEdgeDetected =
        (observation.pattern & LINE_RIGHT_EDGE_SENSOR_MASK) != 0U;
    leftPreturnDetected =
        (observation.pattern & LINE_LEFT_PRETURN_SENSOR_MASK) != 0U;
    rightPreturnDetected =
        (observation.pattern & LINE_RIGHT_PRETURN_SENSOR_MASK) != 0U;
    g_lineEdgeDebug = (uint8_t) (
        (leftEdgeDetected ? 1U : 0U) |
        (rightEdgeDetected ? 2U : 0U) |
        (leftPreturnDetected ? 4U : 0U) |
        (rightPreturnDetected ? 8U : 0U));

    if (leftEdgeDetected && !rightEdgeDetected &&
        (normalizedError > -LINE_EDGE_MIN_NORMALIZED_ERROR)) {
        normalizedError = -LINE_EDGE_MIN_NORMALIZED_ERROR;
    } else if (rightEdgeDetected && !leftEdgeDetected &&
        (normalizedError < LINE_EDGE_MIN_NORMALIZED_ERROR)) {
        normalizedError = LINE_EDGE_MIN_NORMALIZED_ERROR;
    } else if (leftPreturnDetected && !rightPreturnDetected &&
        (normalizedError > -LINE_PRETURN_MIN_NORMALIZED_ERROR)) {
        normalizedError = -LINE_PRETURN_MIN_NORMALIZED_ERROR;
    } else if (rightPreturnDetected && !leftPreturnDetected &&
        (normalizedError < LINE_PRETURN_MIN_NORMALIZED_ERROR)) {
        normalizedError = LINE_PRETURN_MIN_NORMALIZED_ERROR;
    }
    if ((absoluteFloat(normalizedError) <= LINE_ERROR_DEADBAND) &&
        !leftEdgeDetected && !rightEdgeDetected) {
        normalizedError = 0.0f;
    }

    if (!g_errorFilterInitialized) {
        g_filteredError = normalizedError;
        g_previousControlError = normalizedError;
        g_derivativeError = 0.0f;
        g_errorFilterInitialized = true;
        firstControlSample = true;
    } else {
        g_filteredError +=
            (normalizedError - g_filteredError) *
            LINE_ERROR_FILTER_ALPHA;
    }
    g_controlError = g_filteredError;

    if (firstControlSample) {
        derivativeSample = 0.0f;
    } else {
        derivativeSample =
            g_controlError - g_previousControlError;
        g_derivativeError +=
            (derivativeSample - g_derivativeError) *
            LINE_STEER_D_FILTER_ALPHA;
    }
    g_previousControlError = g_controlError;

    /*
     * Continuous error-based gain scheduling. Near center, low P and high D
     * suppress weaving. As error grows, P rises smoothly so the vehicle
     * begins turning before the outer sensors are reached.
     */
    gainBlend = clampFloat(
        (absoluteFloat(g_controlError) - LINE_ERROR_DEADBAND) /
        (1.0f - LINE_ERROR_DEADBAND), 0.0f, 1.0f);
    steeringKp = LINE_STEER_CENTER_KP +
        ((LINE_STEER_CURVE_KP - LINE_STEER_CENTER_KP) * gainBlend);
    steeringKd = LINE_STEER_CENTER_KD +
        ((LINE_STEER_CURVE_KD - LINE_STEER_CENTER_KD) * gainBlend);
    pidOutput =
        (steeringKp * g_controlError) +
        (steeringKd * g_derivativeError);
    g_steeringCommand = clampFloat(
        (float) LINE_STEER_DIRECTION * pidOutput,
        -LINE_STEER_OUTPUT_LIMIT,
        LINE_STEER_OUTPUT_LIMIT);
    /*
     * Gain scheduling for digital edge sensors: keep the center PD gentle,
     * but guarantee both correction direction and minimum authority at an
     * edge. This avoids raising Kp globally and bringing back straight-line
     * oscillation.
     */
    if (leftEdgeDetected && !rightEdgeDetected &&
        (g_steeringCommand > -LINE_EDGE_MIN_STEERING_PERCENT)) {
        g_steeringCommand = -LINE_EDGE_MIN_STEERING_PERCENT;
    } else if (rightEdgeDetected && !leftEdgeDetected &&
        (g_steeringCommand < LINE_EDGE_MIN_STEERING_PERCENT)) {
        g_steeringCommand = LINE_EDGE_MIN_STEERING_PERCENT;
    } else if (leftPreturnDetected && !rightPreturnDetected &&
        (g_steeringCommand > -LINE_PRETURN_MIN_STEERING_PERCENT)) {
        g_steeringCommand = -LINE_PRETURN_MIN_STEERING_PERCENT;
    } else if (rightPreturnDetected && !leftPreturnDetected &&
        (g_steeringCommand < LINE_PRETURN_MIN_STEERING_PERCENT)) {
        g_steeringCommand = LINE_PRETURN_MIN_STEERING_PERCENT;
    } else if (!leftPreturnDetected && !rightPreturnDetected &&
        !leftEdgeDetected && !rightEdgeDetected &&
        (g_controlError <= -LINE_STRAIGHT_CAPTURE_ERROR_THRESHOLD) &&
        (g_steeringCommand > -LINE_STRAIGHT_MIN_STEERING_PERCENT)) {
        /*
         * With digital sensors, OUT4 alone is already a real left offset.
         * Apply a small correction before the line reaches OUT3, otherwise
         * the vehicle can drift straight out of the track.
         */
        g_steeringCommand = -LINE_STRAIGHT_MIN_STEERING_PERCENT;
    } else if (!leftPreturnDetected && !rightPreturnDetected &&
        !leftEdgeDetected && !rightEdgeDetected &&
        (g_controlError >= LINE_STRAIGHT_CAPTURE_ERROR_THRESHOLD) &&
        (g_steeringCommand < LINE_STRAIGHT_MIN_STEERING_PERCENT)) {
        /* OUT5 alone: gently steer right while keeping both wheels moving. */
        g_steeringCommand = LINE_STRAIGHT_MIN_STEERING_PERCENT;
    }
    if (g_runState == LINE_STATE_REACQUIRE) {
        g_steeringCommand = clampFloat(g_steeringCommand,
            -LINE_REACQUIRE_STEER_LIMIT,
            LINE_REACQUIRE_STEER_LIMIT);
    }

    if (absoluteFloat(g_controlError) >=
        LINE_LOST_DIRECTION_MIN_ERROR) {
        g_lastReliableTurnSign =
            (g_steeringCommand >= 0.0f) ? 1 : -1;
        g_reliableDirectionAgeMs = 0U;
    } else {
        if (g_reliableDirectionAgeMs <
            LINE_LOST_DIRECTION_MEMORY_MS) {
            g_reliableDirectionAgeMs += CONTROL_PERIOD_MS;
        }
        if (g_reliableDirectionAgeMs >=
            LINE_LOST_DIRECTION_MEMORY_MS) {
            g_lastReliableTurnSign = 0;
        }
    }

    /*
     * Symmetric speed planning: only the magnitude of the position
     * error affects speed. A linear curve starts reducing speed before the
     * edge sensors are reached, avoiding late braking and corner overshoot.
     */
    curveRatio = absoluteFloat(g_controlError);
    if (leftEdgeDetected || rightEdgeDetected) {
        curveRatio = 1.0f;
    } else if ((leftPreturnDetected || rightPreturnDetected) &&
        (curveRatio < LINE_PRETURN_MIN_CURVE_RATIO)) {
        curveRatio = LINE_PRETURN_MIN_CURVE_RATIO;
    }
    /*
     * A mode may request a base speed lower than the competition sharp-turn
     * speed. In that case the curve must never accelerate the vehicle.
     */
    turnSpeedFloor = (float) LINE_TURN_SPEED_SHARP;
    if (turnSpeedFloor > (float) requestedBaseSpeed) {
        turnSpeedFloor = (float) requestedBaseSpeed;
    }
    speedTarget = (float) requestedBaseSpeed -
        (((float) requestedBaseSpeed - turnSpeedFloor) * curveRatio);
    speedTarget = clampFloat(speedTarget,
        turnSpeedFloor,
        (float) requestedBaseSpeed);
    if ((g_runState == LINE_STATE_REACQUIRE) &&
        (speedTarget > (float) LINE_REACQUIRE_SPEED_PERCENT)) {
        speedTarget = (float) LINE_REACQUIRE_SPEED_PERCENT;
    }

    if (speedTarget < g_speedCommand) {
        speedSlewStep = (leftEdgeDetected || rightEdgeDetected) ?
            LINE_EDGE_SPEED_DECEL_PERCENT_PER_TICK :
            LINE_SPEED_DECEL_PERCENT_PER_TICK;
        g_speedCommand = slewFloat(g_speedCommand, speedTarget,
            speedSlewStep);
    } else {
        g_speedCommand = slewFloat(g_speedCommand, speedTarget,
            LINE_SPEED_ACCEL_PERCENT_PER_TICK);
    }

    outputTrackingTargets(g_speedCommand,
        leftEdgeDetected || rightEdgeDetected,
        leftPreturnDetected || rightPreturnDetected);
    updateDebugState();
    return observation;
}

void LineFollow_Stop(void)
{
    g_runState = LINE_STATE_WAIT_START;
    clearControllerState();
    stopMotorOutput();
}

uint8_t LineFollow_GetPattern(void)
{
    return g_lastPattern;
}

uint8_t LineFollow_GetRawHighPattern(void)
{
    return g_lastRawHighPattern;
}

bool LineFollow_IsVisible(void)
{
    return g_lastLineVisible;
}
