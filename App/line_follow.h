#ifndef LINE_FOLLOW_H
#define LINE_FOLLOW_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float error;
    uint8_t pattern;
    bool lineVisible;
    bool marker;
} LineObservation;

/* Keil Watch 调试变量。 */
extern volatile uint8_t g_lineRunStateDebug;
extern volatile uint8_t g_linePatternDebug;
extern volatile uint8_t g_lineEdgeDebug;
extern volatile uint8_t g_lineActiveCountDebug;
extern volatile uint8_t g_lineMarkerDebug;
extern volatile int16_t g_lineErrorX1000Debug;
extern volatile int16_t g_lineSteeringX10Debug;
extern volatile uint16_t g_lineSpeedCommandX10Debug;
extern volatile uint16_t g_lineLostElapsedMs;
extern volatile uint16_t g_lineLostLeftCommandX10;
extern volatile uint16_t g_lineLostRightCommandX10;

void LineFollow_Init(void);
void LineFollow_CalibrateSensors(void);
LineObservation LineFollow_Read(void);
LineObservation LineFollow_Run(uint8_t requestedBaseSpeed);
void LineFollow_Stop(void);
uint8_t LineFollow_GetPattern(void);
uint8_t LineFollow_GetRawHighPattern(void);
bool LineFollow_IsVisible(void);

#endif
