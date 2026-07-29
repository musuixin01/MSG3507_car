#include "bsp_motor.h"
#include "app_config.h"
#include "ti_msp_dl_config.h"

#define MOTOR_PWM_PERIOD_TICKS (1600U)

static void setDuty(uint32_t channel, uint8_t percent)
{
    uint32_t compare;

    if (percent > 100U) {
        percent = 100U;
    }
    /*
     * 本工程 SysConfig 的边沿对齐PWM为反向比较值：
     * duty=0% 时生成 compare=period，duty=100% 时 compare=0。
     * 因此必须使用 period-duty，才能保证软件百分比与实际速度同向。
     */
    compare = MOTOR_PWM_PERIOD_TICKS -
        ((MOTOR_PWM_PERIOD_TICKS * (uint32_t) percent) / 100U);
    DL_TimerG_setCaptureCompareValue(MOTOR_PWM_INST, compare, channel);
}

void BSP_Motor_Init(void)
{
    BSP_Motor_StopAll();
}

void BSP_Motor_Set(BSP_Motor motor, int8_t percent)
{
    uint8_t duty;
    bool forward;

    if (percent > 100) {
        percent = 100;
    } else if (percent < -100) {
        percent = -100;
    }
    forward = percent >= 0;
    duty = (uint8_t) (forward ? percent : -percent);

    if (motor == BSP_MOTOR_LEFT) {
        if (MOTOR_LEFT_DIRECTION_REVERSED) {
            forward = !forward;
        }
        DL_GPIO_writePinsVal(MOTOR_DIR_AIN1_PORT, MOTOR_DIR_AIN1_PIN,
            forward ? MOTOR_DIR_AIN1_PIN : 0U);
        DL_GPIO_writePinsVal(MOTOR_DIR_AIN2_PORT, MOTOR_DIR_AIN2_PIN,
            forward ? 0U : MOTOR_DIR_AIN2_PIN);
        setDuty(DL_TIMER_CC_0_INDEX, duty);
    } else {
        if (MOTOR_RIGHT_DIRECTION_REVERSED) {
            forward = !forward;
        }
        DL_GPIO_writePinsVal(MOTOR_DIR_BIN1_PORT, MOTOR_DIR_BIN1_PIN,
            forward ? MOTOR_DIR_BIN1_PIN : 0U);
        DL_GPIO_writePinsVal(MOTOR_DIR_BIN2_PORT, MOTOR_DIR_BIN2_PIN,
            forward ? 0U : MOTOR_DIR_BIN2_PIN);
        setDuty(DL_TIMER_CC_1_INDEX, duty);
    }
}

void BSP_Motor_StopAll(void)
{
    setDuty(DL_TIMER_CC_0_INDEX, 0U);
    setDuty(DL_TIMER_CC_1_INDEX, 0U);
    DL_GPIO_clearPins(MOTOR_DIR_AIN1_PORT, MOTOR_DIR_AIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_AIN2_PORT, MOTOR_DIR_AIN2_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_BIN1_PORT, MOTOR_DIR_BIN1_PIN);
    DL_GPIO_clearPins(MOTOR_DIR_BIN2_PORT, MOTOR_DIR_BIN2_PIN);
}

void BSP_Motor_BrakeAll(void)
{
    /*
     * TB6612 short-brake mode: IN1=IN2=H and PWM=H.
     * Used at the A marker to reduce coasting distance.
     */
    DL_GPIO_setPins(MOTOR_DIR_AIN1_PORT, MOTOR_DIR_AIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR_AIN2_PORT, MOTOR_DIR_AIN2_PIN);
    DL_GPIO_setPins(MOTOR_DIR_BIN1_PORT, MOTOR_DIR_BIN1_PIN);
    DL_GPIO_setPins(MOTOR_DIR_BIN2_PORT, MOTOR_DIR_BIN2_PIN);
    setDuty(DL_TIMER_CC_0_INDEX, 100U);
    setDuty(DL_TIMER_CC_1_INDEX, 100U);
}
