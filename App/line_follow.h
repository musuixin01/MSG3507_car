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

void LineFollow_Init(void);
void LineFollow_CalibrateSensors(void);
LineObservation LineFollow_Read(void);
LineObservation LineFollow_Run(uint8_t baseSpeed);
void LineFollow_Stop(void);
uint8_t LineFollow_GetPattern(void);
uint8_t LineFollow_GetRawHighPattern(void);
bool LineFollow_IsVisible(void);
float LineFollow_GetError(void);
uint16_t LineFollow_GetLeftSpeedCounts(void);
uint16_t LineFollow_GetRightSpeedCounts(void);
int16_t LineFollow_GetLeftCommand(void);
int16_t LineFollow_GetRightCommand(void);
bool LineFollow_IsSpeedLoopArmed(void);
bool LineFollow_IsStartConfirmed(void);

#endif
