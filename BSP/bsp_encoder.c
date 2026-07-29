#include "bsp_encoder.h"
#include "mpu6050.h"
#include "ti_msp_dl_config.h"

static volatile int32_t g_leftCount;
static volatile uint8_t g_leftPreviousState;

void BSP_Encoder_Init(void)
{
    g_leftCount = 0;
    g_leftPreviousState =
        (uint8_t) ((DL_GPIO_readPins(
            ENCODER_L_PORT, ENCODER_L_LEFT_A_PIN) != 0U) << 1);
    g_leftPreviousState |= (uint8_t) (DL_GPIO_readPins(
        ENCODER_L_PORT, ENCODER_L_LEFT_B_PIN) != 0U);
    NVIC_ClearPendingIRQ(ENCODER_L_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_L_INT_IRQN);
    DL_TimerG_startCounter(ENCODER_R_INST);
}

int32_t BSP_Encoder_GetLeft(void)
{
    return g_leftCount;
}

uint16_t BSP_Encoder_GetRight(void)
{
    return (uint16_t) DL_TimerG_getTimerCount(ENCODER_R_INST);
}

void BSP_Encoder_Reset(void)
{
    g_leftCount = 0;
    g_leftPreviousState =
        (uint8_t) ((DL_GPIO_readPins(
            ENCODER_L_PORT, ENCODER_L_LEFT_A_PIN) != 0U) << 1);
    g_leftPreviousState |= (uint8_t) (DL_GPIO_readPins(
        ENCODER_L_PORT, ENCODER_L_LEFT_B_PIN) != 0U);
    DL_TimerG_setTimerCount(ENCODER_R_INST, 0U);
}

void GROUP1_IRQHandler(void)
{
    static const int8_t transition[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };
    uint32_t pendingA = DL_GPIO_getPendingInterrupt(GPIOA);
    uint32_t pendingB = DL_GPIO_getPendingInterrupt(GPIOB);

    if (pendingB == MPU_INT_SIGNAL_IIDX) {
        MPU6050_NotifyDataReadyIRQ();
    }

    if ((pendingA == ENCODER_L_LEFT_A_IIDX) ||
        (pendingA == ENCODER_L_LEFT_B_IIDX)) {
        uint8_t currentState =
            (uint8_t) ((DL_GPIO_readPins(
                ENCODER_L_PORT, ENCODER_L_LEFT_A_PIN) != 0U) << 1);
        currentState |= (uint8_t) (DL_GPIO_readPins(
            ENCODER_L_PORT, ENCODER_L_LEFT_B_PIN) != 0U);
        g_leftCount += transition[
            (g_leftPreviousState << 2) | currentState];
        g_leftPreviousState = currentState;
    }
}
