#include "bsp_buzzer.h"
#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint16_t frequencyHz;
    uint16_t durationMs;
    uint16_t gapMs;
} BuzzerNote;

static const BuzzerNote g_startupMelody[] = {
    {523U, 90U, 25U},
    {659U, 90U, 25U},
    {784U, 120U, 30U},
    {1047U, 220U, 0U}
};

static const BuzzerNote g_armedMelody[] = {
    {880U, 80U, 25U},
    {1175U, 120U, 0U}
};

static const BuzzerNote g_completeMelody[] = {
    {659U, 90U, 20U},
    {784U, 90U, 20U},
    {988U, 120U, 25U},
    {1319U, 260U, 0U}
};

static const BuzzerNote *g_melody;
static size_t g_noteCount;
static size_t g_noteIndex;
static uint32_t g_nextChangeMs;
static bool g_inGap;
static bool g_active;

static void stopTone(void)
{
    DL_TimerG_stopCounter(BUZZER_PWM_INST);
    DL_TimerG_setTimerCount(BUZZER_PWM_INST, 0U);
}

static void startTone(uint16_t frequencyHz)
{
    uint32_t periodTicks;

    if (frequencyHz == 0U) {
        stopTone();
        return;
    }
    periodTicks = BUZZER_PWM_INST_CLK_FREQ / frequencyHz;
    if (periodTicks > 65535U) periodTicks = 65535U;
    if (periodTicks < 2U) periodTicks = 2U;
    DL_TimerG_stopCounter(BUZZER_PWM_INST);
    DL_TimerG_setLoadValue(BUZZER_PWM_INST, periodTicks);
    DL_TimerG_setCaptureCompareValue(BUZZER_PWM_INST,
        periodTicks / 2U, DL_TIMER_CC_1_INDEX);
    DL_TimerG_setTimerCount(BUZZER_PWM_INST, 0U);
    DL_TimerG_startCounter(BUZZER_PWM_INST);
}

static void playMelody(const BuzzerNote *melody,
    size_t noteCount, uint32_t nowMs)
{
    g_melody = melody;
    g_noteCount = noteCount;
    g_noteIndex = 0U;
    g_inGap = false;
    g_active = noteCount != 0U;
    if (g_active) {
        startTone(g_melody[0].frequencyHz);
        g_nextChangeMs = nowMs + g_melody[0].durationMs;
    }
}

void BSP_Buzzer_Init(void)
{
    g_melody = NULL;
    g_noteCount = 0U;
    g_noteIndex = 0U;
    g_nextChangeMs = 0U;
    g_inGap = false;
    g_active = false;
    stopTone();
}

void BSP_Buzzer_Update(uint32_t nowMs)
{
    if (!g_active || ((int32_t) (nowMs - g_nextChangeMs) < 0)) {
        return;
    }

    if (!g_inGap && (g_melody[g_noteIndex].gapMs != 0U)) {
        stopTone();
        g_inGap = true;
        g_nextChangeMs = nowMs + g_melody[g_noteIndex].gapMs;
        return;
    }

    g_noteIndex++;
    g_inGap = false;
    if (g_noteIndex >= g_noteCount) {
        stopTone();
        g_active = false;
        return;
    }
    startTone(g_melody[g_noteIndex].frequencyHz);
    g_nextChangeMs = nowMs + g_melody[g_noteIndex].durationMs;
}

void BSP_Buzzer_PlayStartup(uint32_t nowMs)
{
    playMelody(g_startupMelody,
        sizeof(g_startupMelody) / sizeof(g_startupMelody[0]), nowMs);
}

void BSP_Buzzer_PlayArmed(uint32_t nowMs)
{
    playMelody(g_armedMelody,
        sizeof(g_armedMelody) / sizeof(g_armedMelody[0]), nowMs);
}

void BSP_Buzzer_PlayComplete(uint32_t nowMs)
{
    playMelody(g_completeMelody,
        sizeof(g_completeMelody) / sizeof(g_completeMelody[0]), nowMs);
}
