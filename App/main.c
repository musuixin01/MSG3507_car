#include "ti_msp_dl_config.h"
#include "app_config.h"
#include "app_controller.h"
#include "bsp_buzzer.h"
#include "mpu6050.h"
#include "user_interface.h"
static volatile uint32_t g_milliseconds;

void SysTick_Handler(void)
{
    g_milliseconds++;
}

int main(void)
{
    SYSCFG_DL_init();

    BSP_Buzzer_Init();
    AppController_Init();
    UserInterface_Init();
    (void) SysTick_Config(CPUCLK_FREQ / 1000U);
    (void) MPU6050_Init();
    BSP_Buzzer_PlayStartup(g_milliseconds);
    while (1) {
        static uint32_t lastControlMs;
        static uint32_t lastMpuRetryMs;
        uint32_t nowMs = g_milliseconds;

        if ((nowMs - lastControlMs) >= CONTROL_PERIOD_MS) {
            lastControlMs += CONTROL_PERIOD_MS;
            if (MPU6050_IsConnected()) {
                (void) MPU6050_Update(
                    (float) CONTROL_PERIOD_MS / 1000.0f);
            } else if (!AppController_IsRunning() &&
                ((nowMs - lastMpuRetryMs) >= 1000U)) {
                lastMpuRetryMs = nowMs;
                (void) MPU6050_Init();
            }
            AppController_Update(nowMs);
            UserInterface_Update(nowMs);
            BSP_Buzzer_Update(nowMs);
        } else {
            __WFI();
        }
    }
}
