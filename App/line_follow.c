#include "line_follow.h"

#include "app_config.h"
#include "ti_msp_dl_config.h"
#include "wheel_speed_pi.h"

#define TRACK_SENSOR_COUNT (8U)
#define TRACK_HISTORY_MASK (0x07U)

typedef enum {
    LINE_STATE_WAIT_START = 0,
    LINE_STATE_TRACKING,
    LINE_STATE_LOST_HOLD,
    LINE_STATE_LOST_SEARCH,
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

static const int8_t g_trackWeights[TRACK_SENSOR_COUNT] = {
    -7, -5, -3, -1, 1, 3, 5, 7
};

static LineRunState g_runState;
static bool g_activeLow;
static uint8_t g_sensorHistory[TRACK_SENSOR_COUNT];
static uint8_t g_lastPattern;
static uint8_t g_lastRawHighPattern;
static bool g_lastLineVisible;
static float g_lastVisibleError;
static float g_filteredError;
static float g_previousFilteredError;
static bool g_errorFilterInitialized;
static float g_steeringCommand;
static float g_speedCommand;
static float g_diffCommand;
static uint8_t g_startLineSamples;
static bool g_startLineConfirmed;
static uint32_t g_lostElapsedMs;
static uint8_t g_markerDetectSamples;

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

static void stopMotorOutput(void)
{
    g_steeringCommand = 0.0f;
    g_diffCommand = 0.0f;
    WheelSpeedPI_Reset();
}

void LineFollow_Init(void)
{
    g_activeLow = TRACK_ACTIVE_LOW != 0;
    g_lastPattern = 0U;
    g_lastRawHighPattern = 0U;
    g_lastLineVisible = false;
    g_lastVisibleError = 0.0f;
    g_filteredError = 0.0f;
    g_previousFilteredError = 0.0f;
    g_errorFilterInitialized = false;
    g_runState = LINE_STATE_WAIT_START;
    g_startLineSamples = 0U;
    g_startLineConfirmed = false;
    g_lostElapsedMs = 0U;
    g_markerDetectSamples = 0U;
    g_speedCommand = 0.0f;
    g_diffCommand = 0.0f;
    WheelSpeedPI_Init();
    stopMotorOutput();
    LineFollow_CalibrateSensors();
}

void LineFollow_CalibrateSensors(void)
{
    uint8_t highCount = 0U;
    uint8_t index;

    /*
     * 每次启动/重新标定前先清除软件保存的旧电机指令。
     * 否则上一次短刹后，下一次运行会从旧占空比恢复，造成启动弹射。
     */
    stopMotorOutput();

    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        if (trackPinHigh(index)) highCount++;
    }
    if (TRACK_AUTO_POLARITY) {
        /*
         * Calibration must be performed over normal line/background, not
         * the full-width A marker. A majority-high background implies that
         * black is active-low.
         */
        if ((highCount != 0U) && (highCount != TRACK_SENSOR_COUNT)) {
            g_activeLow = highCount >= 5U;
        }
    } else {
        g_activeLow = TRACK_ACTIVE_LOW != 0;
    }

    for (index = 0U; index < TRACK_SENSOR_COUNT; index++) {
        g_sensorHistory[index] = rawBlack(index) ?
            TRACK_HISTORY_MASK : 0U;
    }
    g_lastVisibleError = 0.0f;
    g_filteredError = 0.0f;
    g_previousFilteredError = 0.0f;
    g_errorFilterInitialized = false;
    g_runState = LINE_STATE_WAIT_START;
    g_startLineSamples = 0U;
    g_startLineConfirmed = false;
    g_lostElapsedMs = 0U;
    g_markerDetectSamples = 0U;
    g_steeringCommand = 0.0f;
    g_speedCommand = 0.0f;
    g_diffCommand = 0.0f;
    WheelSpeedPI_Reset();
}

LineObservation LineFollow_Read(void)
{
    LineObservation result = {0};
    int16_t weightedSum = 0;
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
            weightedSum += g_trackWeights[index];
            activeCount++;
        }
    }

    result.lineVisible = activeCount != 0U;

    /*
     * 横线必须连续多帧全黑才成立。急弯造成的单帧多路触发仍按普通
     * 循迹数据处理，不能立即把 error 清零。
     */
    if (activeCount >= A_MARKER_MIN_ACTIVE_SENSORS) {
        if (g_markerDetectSamples < A_MARKER_DETECT_SAMPLES) {
            g_markerDetectSamples++;
        }
    } else {
        g_markerDetectSamples = 0U;
    }
    result.marker =
        g_markerDetectSamples >= A_MARKER_DETECT_SAMPLES;
    if (result.lineVisible) {
        result.error = (float) weightedSum / (float) activeCount;
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

LineObservation LineFollow_Run(uint8_t requestedBaseSpeed)
{
    LineObservation observation = LineFollow_Read();
    float absoluteError;
    float normalizedError;
    float steeringTarget;
    float curveRatio;
    float speedTarget;
    float searchSign;
    float turnAmount;
    float diffTarget;
    float motorOutputLimit;
    float diffSlew;
    int16_t leftTarget;
    int16_t rightTarget;
    bool leftEdgeDetected = false;
    bool rightEdgeDetected = false;

    if (!g_startLineConfirmed) {
        bool validStart = observation.marker ||
            (observation.lineVisible &&
             (absoluteFloat(observation.error) <=
              LINE_START_MAX_ERROR));

        if (validStart) {
            if (g_startLineSamples < LINE_START_CONFIRM_SAMPLES) {
                g_startLineSamples++;
            }
            if (g_startLineSamples >= LINE_START_CONFIRM_SAMPLES) {
                g_startLineConfirmed = true;
                g_runState = LINE_STATE_TRACKING;
                /*
                 * 启动后直接采用固定基础速度；速度平滑只用于之后
                 * 直线和弯道之间的切换。
                 */
                g_speedCommand = (float) requestedBaseSpeed;
            }
        } else {
            g_startLineSamples = 0U;
        }

        if (!g_startLineConfirmed) {
            stopMotorOutput();
            return observation;
        }
    }

    if (observation.lineVisible) {
        g_lostElapsedMs = 0U;
        g_runState = LINE_STATE_TRACKING;

        if (observation.marker) {
            observation.error = 0.0f;
        }
        if (!g_errorFilterInitialized) {
            g_filteredError = observation.error;
            g_previousFilteredError = observation.error;
            g_errorFilterInitialized = true;
        } else {
            g_previousFilteredError = g_filteredError;
            g_filteredError +=
                (observation.error - g_filteredError) *
                LINE_ERROR_FILTER_ALPHA;
        }

        absoluteError = absoluteFloat(g_filteredError);
        normalizedError = clampFloat(
            g_filteredError / LINE_ERROR_FULL_SCALE, -1.0f, 1.0f);
        leftEdgeDetected =
            (observation.pattern & LINE_LEFT_EDGE_SENSOR_MASK) != 0U;
        rightEdgeDetected =
            (observation.pattern & LINE_RIGHT_EDGE_SENSOR_MASK) != 0U;

        /*
         * 外侧两路直接保证正确的转向方向和最小转向比例。
         * 这可以避免多路传感器同时触发时加权平均接近0，导致两轮同速。
         */
        if (leftEdgeDetected && !rightEdgeDetected &&
            (normalizedError > -0.70f)) {
            normalizedError = -0.70f;
        } else if (rightEdgeDetected && !leftEdgeDetected &&
            (normalizedError < 0.70f)) {
            normalizedError = 0.70f;
        }
        if (absoluteError <= LINE_ERROR_DEADBAND &&
            !leftEdgeDetected && !rightEdgeDetected) {
            normalizedError = 0.0f;
        }

        steeringTarget =
            (float) LINE_STEER_DIRECTION * normalizedError;
        g_steeringCommand = slewFloat(g_steeringCommand,
            steeringTarget, LINE_LINEAR_STEER_SLEW_PER_TICK);

        /* 黑线偏差0..7线性映射为直线速度..急弯速度。 */
        curveRatio = clampFloat(
            absoluteFloat(observation.error) /
            LINE_ERROR_FULL_SCALE, 0.0f, 1.0f);
        speedTarget = (float) requestedBaseSpeed -
            (((float) requestedBaseSpeed -
              (float) LINE_TURN_SPEED_SHARP) * curveRatio);
        if (speedTarget < (float) LINE_TURN_SPEED_SHARP) {
            speedTarget = (float) LINE_TURN_SPEED_SHARP;
        }

        /*
         * 最左/最右区域检测到黑线时，不等待低通误差慢慢收敛，
         * 直接限制两轮共同基础速度。转向差速随后只会继续降低内侧轮，
         * 因此任何一侧都不会维持直线高速冲入弯道。
         */
        if (speedTarget < g_speedCommand) {
            g_speedCommand = slewFloat(g_speedCommand, speedTarget,
                LINE_SPEED_DECEL_PERCENT_PER_TICK);
        } else {
            g_speedCommand = slewFloat(g_speedCommand, speedTarget,
                LINE_SPEED_ACCEL_PERCENT_PER_TICK);
        }
        speedTarget = g_speedCommand;
    } else {
        g_lostElapsedMs += CONTROL_PERIOD_MS;
        g_errorFilterInitialized = false;

        if (g_lostElapsedMs <= LINE_LOST_HOLD_MS) {
            g_runState = LINE_STATE_LOST_HOLD;
            speedTarget = (float) LINE_LOST_HOLD_SPEED_PERCENT;
            /* Hold the last steering command briefly across sensor gaps. */
        } else if (g_lostElapsedMs <= LINE_LOST_STOP_MS) {
            g_runState = LINE_STATE_LOST_SEARCH;
            speedTarget = (float) LINE_LOST_SEARCH_BASE_PERCENT;
            searchSign = (g_lastVisibleError >= 0.0f) ? 1.0f : -1.0f;
            steeringTarget =
                (float) LINE_STEER_DIRECTION * searchSign;
            g_steeringCommand = slewFloat(g_steeringCommand,
                steeringTarget, LINE_LINEAR_STEER_SLEW_PER_TICK);
        } else {
            g_runState = LINE_STATE_LOST_STOP;
            stopMotorOutput();
            return observation;
        }

        /*
         * 丢线保持和搜索也统一经过速度平滑，避免从循迹速度瞬间跳到
         * 8%/10% 引起车身顿挫。搜索时使用更快的收敛速率。
         */
        g_speedCommand = slewFloat(g_speedCommand, speedTarget,
            LINE_LOST_SPEED_SLEW_PERCENT_PER_TICK);
        speedTarget = g_speedCommand;
    }

    /*
     * 基础速度直接采用设定值，不再随启动时间逐级增加。
     * 差速只负责循迹，并限制在当前基础速度以内。
     */
    g_steeringCommand = clampFloat(
        g_steeringCommand, -1.0f, 1.0f);

    /*
     * 单边减速差速：
     * 外侧轮最高只保持当前基础速度，禁止在弯道突然超过直线速度；
     * 内侧轮按转向量减速，大偏差时允许降到 0。
     * 这样从直线进入左右弯时只有平滑减速，不会出现外侧轮猛加速。
     */
    /*
     * 对带符号差速量再次限速。这样左右误差切换时，减速侧会平滑
     * 穿过 0 后再换边，不会把较大的减速量瞬间切到另一侧车轮。
     */
    motorOutputLimit = speedTarget;
    diffSlew = LINE_DIFF_SLEW_PERCENT_PER_TICK;

    if (motorOutputLimit > LINE_MOTOR_HARD_LIMIT_PERCENT) {
        motorOutputLimit = LINE_MOTOR_HARD_LIMIT_PERCENT;
    } else if (motorOutputLimit < 0.0f) {
        motorOutputLimit = 0.0f;
    }

    /*
     * 转向比例与黑线偏差线性对应。外侧轮保持当前基础速度，
     * 内侧轮按比例减速，不再在OUT1/2/7/8处突然切换另一套速度。
     */
    diffTarget = g_steeringCommand *
        motorOutputLimit * LINE_LINEAR_TURN_GAIN;
    g_diffCommand = slewFloat(g_diffCommand, diffTarget,
        diffSlew);
    turnAmount = absoluteFloat(g_diffCommand);
    if (turnAmount > motorOutputLimit) {
        turnAmount = motorOutputLimit;
    }
    if (g_diffCommand > 0.0f) {
        leftTarget = (int16_t) motorOutputLimit;
        rightTarget = (int16_t) (motorOutputLimit - turnAmount);
    } else if (g_diffCommand < 0.0f) {
        leftTarget = (int16_t) (motorOutputLimit - turnAmount);
        rightTarget = (int16_t) motorOutputLimit;
    } else {
        leftTarget = (int16_t) motorOutputLimit;
        rightTarget = (int16_t) motorOutputLimit;
    }

    leftTarget = (int16_t) (
        ((int32_t) leftTarget * MOTOR_LEFT_GAIN_PERCENT) / 100);
    rightTarget = (int16_t) (
        ((int32_t) rightTarget * MOTOR_RIGHT_GAIN_PERCENT) / 100);
    if (leftTarget < 0) leftTarget = 0;
    if (rightTarget < 0) rightTarget = 0;
    /*
     * 最终安全限幅放在左右增益补偿之后，保证任何参数组合都不能让
     * 实际 PWM 超过当前工况允许的速度。
     */
    if (leftTarget > (int16_t) motorOutputLimit) {
        leftTarget = (int16_t) motorOutputLimit;
    }
    if (rightTarget > (int16_t) motorOutputLimit) {
        rightTarget = (int16_t) motorOutputLimit;
    }

    WheelSpeedPI_Update((float) leftTarget, (float) rightTarget);
    return observation;
}

void LineFollow_Stop(void)
{
    g_runState = LINE_STATE_WAIT_START;
    g_startLineSamples = 0U;
    g_startLineConfirmed = false;
    g_lostElapsedMs = 0U;
    g_markerDetectSamples = 0U;
    g_filteredError = 0.0f;
    g_previousFilteredError = 0.0f;
    g_errorFilterInitialized = false;
    g_speedCommand = 0.0f;
    g_diffCommand = 0.0f;
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

float LineFollow_GetError(void)
{
    return g_filteredError;
}

uint16_t LineFollow_GetLeftSpeedCounts(void)
{
    return WheelSpeedPI_GetLeftDelta();
}

uint16_t LineFollow_GetRightSpeedCounts(void)
{
    return WheelSpeedPI_GetRightDelta();
}

int16_t LineFollow_GetLeftCommand(void)
{
    return WheelSpeedPI_GetLeftOutput();
}

int16_t LineFollow_GetRightCommand(void)
{
    return WheelSpeedPI_GetRightOutput();
}

bool LineFollow_IsSpeedLoopArmed(void)
{
    return WheelSpeedPI_IsEnabled();
}

bool LineFollow_IsStartConfirmed(void)
{
    return g_startLineConfirmed;
}
