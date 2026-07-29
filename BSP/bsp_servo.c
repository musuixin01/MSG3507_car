#include "bsp_servo.h"
#include "ti_msp_dl_config.h"

#define SERVO_PERIOD_TICKS (64000U)
#define SERVO_MIN_US         (500U)
#define SERVO_MAX_US        (2500U)

void BSP_Servo_Init(void)
{
    BSP_Servo_SetPulseUs(BSP_SERVO_CAMERA_PAN, 1500U);
    BSP_Servo_SetPulseUs(BSP_SERVO_CAMERA_TILT, 1500U);
    BSP_Servo_SetPulseUs(BSP_SERVO_SONAR_PAN, 1500U);
    BSP_Servo_SetPulseUs(BSP_SERVO_SONAR_TILT, 1500U);
}

void BSP_Servo_SetPulseUs(BSP_Servo servo, uint16_t pulseUs)
{
    uint32_t pulseTicks;

    if (pulseUs < SERVO_MIN_US) {
        pulseUs = SERVO_MIN_US;
    } else if (pulseUs > SERVO_MAX_US) {
        pulseUs = SERVO_MAX_US;
    }
    pulseTicks = ((uint32_t) pulseUs * 16U) / 5U;
    DL_TimerA_setCaptureCompareValue(SERVO_PWM_INST,
        SERVO_PERIOD_TICKS - pulseTicks, (uint32_t) servo);
}

void BSP_Servo_SetAngle(BSP_Servo servo, uint8_t angle)
{
    uint16_t pulseUs;

    if (angle > 180U) {
        angle = 180U;
    }
    pulseUs = (uint16_t) (SERVO_MIN_US +
        (((uint32_t) angle * (SERVO_MAX_US - SERVO_MIN_US)) / 180U));
    BSP_Servo_SetPulseUs(servo, pulseUs);
}
