#include "app_controller.h"
#include "app_config.h"
#include "ball_control.h"
#include "bsp_buzzer.h"
#include "bsp_encoder.h"
#include "bsp_motor.h"
#include "bsp_servo.h"
#include "line_follow.h"
#include "vision_protocol.h"
#include "wheel_speed_pi.h"

typedef enum {
    RUN_IDLE,
    RUN_ACTIVE,
    RUN_COMPLETE,
    RUN_FAULT
} RunState;

static AppMode g_mode;
static RunState g_state;
static uint32_t g_startMs;
static uint32_t g_finishMs;
static uint32_t g_stageMs;
static uint8_t g_task3Stage;
static bool g_markerLatched;
static uint8_t g_markerSamples;
static uint8_t g_markerClearSamples;
static bool g_finishMarkerArmed;
static uint16_t g_encoderStart;

static uint16_t encoderDelta(void)
{
    return (uint16_t) (BSP_Encoder_GetRight() - g_encoderStart);
}

static void complete(uint32_t nowMs)
{
    if ((g_mode == APP_MODE_LAP_FAST) ||
        (g_mode == APP_MODE_AB_BALANCE) ||
        (g_mode == APP_MODE_LAP_CENTER) ||
        (g_mode == APP_MODE_LAP_POSITION)) {
        BSP_Motor_BrakeAll();
    } else {
        LineFollow_Stop();
    }
    g_finishMs = nowMs;
    g_state = RUN_COMPLETE;
    BSP_Buzzer_PlayComplete(nowMs);
}

void AppController_Init(void)
{
    BSP_Motor_Init();
    BSP_Servo_Init();
    BSP_Encoder_Init();
    LineFollow_Init();
    BallControl_Init();
    Vision_Init();
    g_mode = APP_MODE_VIDEO;
    g_state = RUN_IDLE;
}

void AppController_SelectMode(AppMode mode)
{
    if ((g_state != RUN_ACTIVE) &&
        (mode >= APP_MODE_VIDEO) && (mode <= APP_MODE_LAP_POSITION)) {
        g_mode = mode;
    }
}

void AppController_Start(uint32_t nowMs)
{
    if (g_state == RUN_ACTIVE) return;
    BSP_Encoder_Reset();
    g_encoderStart = BSP_Encoder_GetRight();
    g_startMs = nowMs;
    g_finishMs = nowMs;
    g_stageMs = nowMs;
    g_task3Stage = 0;
    g_markerLatched = false;
    g_markerSamples = 0;
    g_markerClearSamples = 0;
    g_finishMarkerArmed = false;
    LineFollow_CalibrateSensors();
    BallControl_Init();
    BallControl_SetTargetMm(
        (g_mode == APP_MODE_LAP_POSITION) ? TASK6_TARGET_MM : 0);
    g_state = RUN_ACTIVE;
}

void AppController_Abort(void)
{
    LineFollow_Stop();
    BSP_Servo_SetPulseUs(BSP_SERVO_CAMERA_PAN, 1500U);
    g_state = RUN_IDLE;
}

void AppController_Stop(uint32_t nowMs)
{
    if (g_state == RUN_ACTIVE) {
        LineFollow_Stop();
        g_finishMs = nowMs;
        g_state = RUN_COMPLETE;
    }
}

static void updateBall(uint32_t nowMs)
{
    int16_t positionMm;
    bool valid;
    Vision_Poll(nowMs);
    valid = Vision_GetBallPosition(&positionMm, nowMs);
    BallControl_Update(positionMm, valid);
}

static void updateLap(uint32_t nowMs, bool balanceEnabled)
{
    LineObservation line;

    if (balanceEnabled) updateBall(nowMs);
    line = LineFollow_Run(balanceEnabled ?
        LINE_BASE_SPEED_BALANCE : LINE_BASE_SPEED_FAST);

    /*
     * 起点和终点是同一条横线。必须先确认车头已经完全离开起点横线，
     * 才允许下一次横线触发停车，避免用固定时间盲区造成漏停或误停。
     */
    if (!g_finishMarkerArmed) {
        g_markerSamples = 0U;
        if (!line.marker && line.lineVisible) {
            if (g_markerClearSamples < A_MARKER_CLEAR_SAMPLES) {
                g_markerClearSamples++;
            }
            if (g_markerClearSamples >= A_MARKER_CLEAR_SAMPLES) {
                g_finishMarkerArmed = true;
                g_markerLatched = false;
            }
        } else {
            g_markerClearSamples = 0U;
        }
        return;
    }

    if (!line.marker) {
        g_markerLatched = false;
        g_markerSamples = 0U;
    } else if (g_markerSamples < A_MARKER_CONFIRM_SAMPLES) {
        g_markerSamples++;
    }

    if (!g_markerLatched &&
        (g_markerSamples >= A_MARKER_CONFIRM_SAMPLES) &&
        ((nowMs - g_startMs) >= A_MARKER_MIN_LAP_MS)) {
        g_markerLatched = true;
        complete(nowMs);
    }
}

static void updateTask3(uint32_t nowMs)
{
    updateBall(nowMs);
    if (g_task3Stage == 0U) {
        BallControl_SetTargetMm(50);
        g_task3Stage = 1U;
        g_stageMs = nowMs;
    } else if ((g_task3Stage == 1U) &&
        (BallControl_IsStable() ||
         ((nowMs - g_stageMs) >= TASK3_STAGE_TIMEOUT_MS))) {
        BallControl_SetTargetMm(-50);
        g_task3Stage = 2U;
        g_stageMs = nowMs;
    } else if ((g_task3Stage == 2U) &&
        BallControl_IsStable()) {
        complete(nowMs);
    } else if ((nowMs - g_startMs) > 5000U) {
        complete(nowMs);
    }
}

void AppController_Update(uint32_t nowMs)
{
    if (g_state != RUN_ACTIVE) {
        Vision_Poll(nowMs);
        return;
    }

    switch (g_mode) {
        case APP_MODE_VIDEO:
#if SPEED_PI_CALIBRATION_MODE
            /*
             * 模式1临时作为编码器标定模式：
             * 不依赖黑线确认，两轮固定参考PWM并更新20 ms计数。
             */
            Vision_Poll(nowMs);
            WheelSpeedPI_RunCalibration(
                SPEED_PI_REFERENCE_PWM_PERCENT);
#else
            /*
             * Mode 1 is the safe commissioning mode: continuous line
             * following at reduced speed, without automatic A-line stop.
             * Long press again to stop and unlock the mode.
             */
            Vision_Poll(nowMs);
            (void) LineFollow_Run(LINE_BASE_SPEED_TEST);
#endif
            break;
        case APP_MODE_LAP_FAST:
            updateLap(nowMs, false);
            break;
        case APP_MODE_BALL_STATIC:
            updateTask3(nowMs);
            break;
        case APP_MODE_AB_BALANCE:
            updateBall(nowMs);
            (void) LineFollow_Run(LINE_BASE_SPEED_BALANCE);
            if (encoderDelta() >= AB_TARGET_ENCODER_COUNTS) complete(nowMs);
            break;
        case APP_MODE_LAP_CENTER:
        case APP_MODE_LAP_POSITION:
            updateLap(nowMs, true);
            break;
        default:
            g_state = RUN_FAULT;
            AppController_Abort();
            break;
    }
}

bool AppController_IsRunning(void)
{
    return g_state == RUN_ACTIVE;
}

uint32_t AppController_GetElapsedMs(uint32_t nowMs)
{
    return ((g_state == RUN_ACTIVE) ? nowMs : g_finishMs) - g_startMs;
}

AppMode AppController_GetMode(void)
{
    return g_mode;
}
