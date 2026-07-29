#include "ball_control.h"
#include "app_config.h"
#include "bsp_servo.h"
#include "control_pid.h"

#define BALL_LIMIT_MM            (115)
#define BALL_STABLE_ERROR_MM      (10)
#define BALL_STABLE_SAMPLES      (100U)
#define BALANCE_SERVO_CENTER_US (1500)

static ControlPID g_ballPid;
static int16_t g_targetMm;
static uint16_t g_stableSamples;

void BallControl_Init(void)
{
    /*
     * Initial values for a 5 ms loop. Final values must be identified on the
     * real beam because linkage ratio, servo direction and friction vary.
     */
    PID_Init(&g_ballPid, 8.0f, 0.35f, 1.2f, 80.0f, 650.0f);
    g_targetMm = 0;
    g_stableSamples = 0;
}

void BallControl_SetTargetMm(int16_t positionMm)
{
    if (positionMm > BALL_LIMIT_MM) positionMm = BALL_LIMIT_MM;
    if (positionMm < -BALL_LIMIT_MM) positionMm = -BALL_LIMIT_MM;
    if (positionMm != g_targetMm) {
        g_targetMm = positionMm;
        g_stableSamples = 0;
        PID_Reset(&g_ballPid);
    }
}

void BallControl_Update(int16_t measuredMm, bool measurementValid)
{
    float correction;
    int32_t pulseUs;
    int16_t error;

    if (!measurementValid) {
        g_stableSamples = 0;
        BSP_Servo_SetPulseUs(BSP_SERVO_CAMERA_PAN, BALANCE_SERVO_CENTER_US);
        return;
    }
    correction = PID_Update(
        &g_ballPid, (float) g_targetMm, (float) measuredMm, 0.005f);
    pulseUs = BALANCE_SERVO_CENTER_US +
        (BALANCE_SERVO_DIRECTION * (int32_t) correction);
    BSP_Servo_SetPulseUs(BSP_SERVO_CAMERA_PAN, (uint16_t) pulseUs);

    error = g_targetMm - measuredMm;
    if ((error >= -BALL_STABLE_ERROR_MM) &&
        (error <= BALL_STABLE_ERROR_MM)) {
        if (g_stableSamples < BALL_STABLE_SAMPLES) {
            g_stableSamples++;
        }
    } else {
        g_stableSamples = 0;
    }
}

int16_t BallControl_GetTargetMm(void)
{
    return g_targetMm;
}

bool BallControl_IsStable(void)
{
    return g_stableSamples >= BALL_STABLE_SAMPLES;
}
