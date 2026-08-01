#include "wheel_speed_pi.h"

#include "app_config.h"
#include "bsp_encoder.h"
#include "bsp_motor.h"

typedef struct {
    float integral;
    float output;
} WheelPIState;

static WheelPIState g_leftPI;
static WheelPIState g_rightPI;
static int32_t g_previousLeft;
static uint16_t g_previousRight;
static uint8_t g_sampleDivider;

volatile uint16_t g_wheelSpeedLeftDelta;
volatile uint16_t g_wheelSpeedRightDelta;
volatile uint16_t g_wheelSpeedLeftTargetX10;
volatile uint16_t g_wheelSpeedRightTargetX10;
volatile uint16_t g_wheelSpeedLeftOutputX10;
volatile uint16_t g_wheelSpeedRightOutputX10;

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

static uint16_t absoluteDelta32(int32_t value)
{
    if (value < 0) value = -value;
    return (value > 65535) ? 65535U : (uint16_t) value;
}

static int32_t signedDelta16(uint16_t current, uint16_t previous)
{
    uint16_t delta = (uint16_t) (current - previous);

    if (delta <= 0x7FFFU) {
        return (int32_t) delta;
    }
    return (int32_t) delta - 65536;
}

static float updateOneWheel(WheelPIState *state, float targetPercent,
    uint16_t measuredCounts, float referenceCounts)
{
    float measuredPercent;
    float error;
    float candidateIntegral;
    float dynamicOutputLimit;
    float output;
    float outputSlewStep;

    targetPercent = clampFloat(targetPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    if (targetPercent <= 0.0f) {
        state->integral = 0.0f;
        state->output = 0.0f;
        return 0.0f;
    }

    measuredPercent = ((float) measuredCounts *
        SPEED_PI_REFERENCE_PWM_PERCENT) / referenceCounts;
    error = targetPercent - measuredPercent;
    dynamicOutputLimit = clampFloat(
        targetPercent + SPEED_PI_OUTPUT_HEADROOM_PERCENT,
        0.0f, LINE_MOTOR_HARD_LIMIT_PERCENT);
    candidateIntegral = clampFloat(
        state->integral + error,
        -SPEED_PI_INTEGRAL_LIMIT, SPEED_PI_INTEGRAL_LIMIT);
    output = targetPercent +
        (SPEED_PI_KP * error) +
        (SPEED_PI_KI * candidateIntegral);
    output = clampFloat(output, 0.0f, dynamicOutputLimit);
    outputSlewStep = (output < state->output) ?
        SPEED_PI_OUTPUT_DECEL_SLEW_PER_SAMPLE :
        SPEED_PI_OUTPUT_ACCEL_SLEW_PER_SAMPLE;
    output = slewFloat(state->output, output,
        outputSlewStep);
    output = clampFloat(output, 0.0f, dynamicOutputLimit);

    if (((output < dynamicOutputLimit) &&
         (output > 0.0f)) ||
        ((output >= dynamicOutputLimit) && (error < 0.0f)) ||
        ((output <= 0.0f) && (error > 0.0f))) {
        state->integral = candidateIntegral;
    }
    state->output = output;
    return output;
}

void WheelSpeedPI_Init(void)
{
    WheelSpeedPI_Reset();
}

void WheelSpeedPI_Reset(void)
{
    g_leftPI.integral = 0.0f;
    g_leftPI.output = 0.0f;
    g_rightPI.integral = 0.0f;
    g_rightPI.output = 0.0f;
    g_previousLeft = BSP_Encoder_GetLeft();
    g_previousRight = BSP_Encoder_GetRight();
    g_sampleDivider = 0U;
    g_wheelSpeedLeftDelta = 0U;
    g_wheelSpeedRightDelta = 0U;
    g_wheelSpeedLeftTargetX10 = 0U;
    g_wheelSpeedRightTargetX10 = 0U;
    g_wheelSpeedLeftOutputX10 = 0U;
    g_wheelSpeedRightOutputX10 = 0U;
    BSP_Motor_StopAll();
}

void WheelSpeedPI_Update(float leftTargetPercent,
    float rightTargetPercent)
{
    int32_t leftNow;
    uint16_t rightNow;
    int32_t leftDifference;
    int32_t rightDifference;
    float leftOutput;
    float rightOutput;

    leftTargetPercent = clampFloat(leftTargetPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    rightTargetPercent = clampFloat(rightTargetPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    g_wheelSpeedLeftTargetX10 =
        (uint16_t) (leftTargetPercent * 10.0f);
    g_wheelSpeedRightTargetX10 =
        (uint16_t) (rightTargetPercent * 10.0f);

    if (++g_sampleDivider < SPEED_PI_SAMPLE_DIVIDER) {
        return;
    }
    g_sampleDivider = 0U;

    leftNow = BSP_Encoder_GetLeft();
    rightNow = BSP_Encoder_GetRight();
    leftDifference = leftNow - g_previousLeft;
    rightDifference = signedDelta16(rightNow, g_previousRight);
    g_previousLeft = leftNow;
    g_previousRight = rightNow;
    g_wheelSpeedLeftDelta = absoluteDelta32(leftDifference);
    g_wheelSpeedRightDelta = absoluteDelta32(rightDifference);

#if SPEED_PI_ENABLE
    leftOutput = updateOneWheel(&g_leftPI, leftTargetPercent,
        g_wheelSpeedLeftDelta, SPEED_PI_LEFT_REFERENCE_COUNTS);
    rightOutput = updateOneWheel(&g_rightPI, rightTargetPercent,
        g_wheelSpeedRightDelta, SPEED_PI_RIGHT_REFERENCE_COUNTS);
#else
    leftOutput = clampFloat(leftTargetPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    rightOutput = clampFloat(rightTargetPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);
    g_leftPI.output = leftOutput;
    g_rightPI.output = rightOutput;
#endif

    g_wheelSpeedLeftOutputX10 = (uint16_t) (leftOutput * 10.0f);
    g_wheelSpeedRightOutputX10 = (uint16_t) (rightOutput * 10.0f);
    BSP_Motor_Set(BSP_MOTOR_LEFT, (int8_t) leftOutput);
    BSP_Motor_Set(BSP_MOTOR_RIGHT, (int8_t) rightOutput);
}

void WheelSpeedPI_RunCalibration(float pwmPercent)
{
    int32_t leftNow;
    uint16_t rightNow;
    int32_t leftDifference;
    int32_t rightDifference;

    pwmPercent = clampFloat(pwmPercent, 0.0f,
        LINE_MOTOR_HARD_LIMIT_PERCENT);

    BSP_Motor_Set(BSP_MOTOR_LEFT, (int8_t) pwmPercent);
    BSP_Motor_Set(BSP_MOTOR_RIGHT, (int8_t) pwmPercent);
    g_leftPI.output = pwmPercent;
    g_rightPI.output = pwmPercent;

    if (++g_sampleDivider < SPEED_PI_SAMPLE_DIVIDER) {
        return;
    }
    g_sampleDivider = 0U;

    leftNow = BSP_Encoder_GetLeft();
    rightNow = BSP_Encoder_GetRight();
    leftDifference = leftNow - g_previousLeft;
    rightDifference = signedDelta16(rightNow, g_previousRight);
    g_previousLeft = leftNow;
    g_previousRight = rightNow;
    g_wheelSpeedLeftDelta = absoluteDelta32(leftDifference);
    g_wheelSpeedRightDelta = absoluteDelta32(rightDifference);
}
