#include "vision_protocol.h"
#include "app_config.h"
#include "communication.h"

/*
 * K210 sends ASCII frames: X:+123\n
 * Position is signed millimetres from beam centre, valid range -125..+125.
 */
static int16_t g_positionMm;
static uint32_t g_lastFrameMs;
static int16_t g_accumulator;
static int8_t g_sign;
static bool g_inFrame;
static bool g_hasDigit;

void Vision_Init(void)
{
    g_positionMm = 0;
    g_lastFrameMs = 0;
    g_accumulator = 0;
    g_sign = 1;
    g_inFrame = false;
    g_hasDigit = false;
}

void Vision_Poll(uint32_t nowMs)
{
    uint8_t byte;

    while (K210_ReadByte(&byte)) {
        if (byte == 'X') {
            g_accumulator = 0;
            g_sign = 1;
            g_inFrame = true;
            g_hasDigit = false;
        } else if (g_inFrame && (byte == '-')) {
            g_sign = -1;
        } else if (g_inFrame && (byte >= '0') && (byte <= '9')) {
            if (g_accumulator < 1000) {
                g_accumulator =
                    (int16_t) ((g_accumulator * 10) + (byte - '0'));
                g_hasDigit = true;
            }
        } else if (g_inFrame && ((byte == '\n') || (byte == '\r'))) {
            int16_t value = (int16_t) (g_sign * g_accumulator);
            if (g_hasDigit && (value >= -125) && (value <= 125)) {
                g_positionMm = value;
                g_lastFrameMs = nowMs;
            }
            g_inFrame = false;
        }
    }
}

bool Vision_GetBallPosition(int16_t *positionMm, uint32_t nowMs)
{
    if ((positionMm == NULL) ||
        ((nowMs - g_lastFrameMs) > BALL_MEASUREMENT_TIMEOUT_MS)) {
        return false;
    }
    *positionMm = g_positionMm;
    return true;
}
