#include "user_interface.h"
#include "app_controller.h"
#include "bsp_buzzer.h"
#include "line_follow.h"
#include "mpu6050.h"
#include "oled.h"
#include "ti_msp_dl_config.h"

#define BUTTON_DEBOUNCE_MS   (30U)
#define BUTTON_LONG_PRESS_MS (1500U)
#define DISPLAY_PERIOD_MS    (200U)
#define START_RELEASE_DELAY_MS (500U)

static bool g_rawPressed;
static bool g_stablePressed;
static bool g_longPressHandled;
static uint32_t g_rawChangedMs;
static uint32_t g_pressStartedMs;
static uint32_t g_lastDisplayMs;
static uint32_t g_startAtMs;
static bool g_startArmed;
static bool g_startScheduled;

static bool buttonPressed(void)
{
    return (DL_GPIO_readPins(
        USER_BUTTON_PORT, USER_BUTTON_KEY_PIN) & USER_BUTTON_KEY_PIN) == 0U;
}

static void selectNextMode(void)
{
    int32_t mode = (int32_t) AppController_GetMode() + 1;
    if (mode > APP_MODE_LAP_POSITION) {
        mode = APP_MODE_VIDEO;
    }
    AppController_SelectMode((AppMode) mode);
}

void UserInterface_Init(void)
{
    g_rawPressed = buttonPressed();
    g_stablePressed = g_rawPressed;
    g_longPressHandled = false;
    g_rawChangedMs = 0;
    g_pressStartedMs = 0;
    g_lastDisplayMs = 0;
    g_startAtMs = 0;
    g_startArmed = false;
    g_startScheduled = false;
    OLED_Init();
    OLED_ShowMenu((uint8_t) AppController_GetMode(), 0U, false);
}

void UserInterface_Update(uint32_t nowMs)
{
    bool currentPressed = buttonPressed();

    if (currentPressed != g_rawPressed) {
        g_rawPressed = currentPressed;
        g_rawChangedMs = nowMs;
    }

    if ((g_stablePressed != g_rawPressed) &&
        ((nowMs - g_rawChangedMs) >= BUTTON_DEBOUNCE_MS)) {
        g_stablePressed = g_rawPressed;
        if (g_stablePressed) {
            g_pressStartedMs = nowMs;
            g_longPressHandled = false;
        } else {
            /* A click is generated on release only if long press did not fire. */
            if (g_startArmed) {
                g_startArmed = false;
                g_startScheduled = true;
                g_startAtMs = nowMs + START_RELEASE_DELAY_MS;
            } else if (!g_longPressHandled &&
                !AppController_IsRunning() && !g_startScheduled) {
                selectNextMode();
            }
        }
    }

    if (g_stablePressed && !g_longPressHandled &&
        ((nowMs - g_pressStartedMs) >= BUTTON_LONG_PRESS_MS)) {
        g_longPressHandled = true;
        if (AppController_IsRunning()) {
            AppController_Stop(nowMs);
        } else {
            /*
             * Long press only arms the selected mode. Actual motion starts
             * after the button is released plus a short safety pause.
             */
            g_startArmed = true;
            BSP_Buzzer_PlayArmed(nowMs);
        }
    }

    if (g_startScheduled &&
        ((int32_t) (nowMs - g_startAtMs) >= 0)) {
        g_startScheduled = false;
        AppController_Start(nowMs);
    }

    if (!AppController_IsRunning() &&
        ((AppController_GetMode() == APP_MODE_VIDEO) ||
         (AppController_GetMode() == APP_MODE_LAP_FAST))) {
        (void) LineFollow_Read();
    }

    if ((nowMs - g_lastDisplayMs) >= DISPLAY_PERIOD_MS) {
        g_lastDisplayMs = nowMs;
        OLED_ShowMenu((uint8_t) AppController_GetMode(),
            AppController_GetElapsedMs(nowMs),
            AppController_IsRunning());
        OLED_ShowImuStatus(
            MPU6050_IsConnected(), MPU6050_IsCalibrated());
        /*
         * Keep the five sensor bits visible in modes 1/2 even while stopped,
         * so polarity and physical sensor order can be checked safely.
         */
        if ((AppController_GetMode() == APP_MODE_VIDEO) ||
            (AppController_GetMode() == APP_MODE_LAP_FAST)) {
            OLED_ShowTrackPattern(
                LineFollow_GetRawHighPattern(),
                LineFollow_GetPattern(), LineFollow_IsVisible());
        }
    }
}
