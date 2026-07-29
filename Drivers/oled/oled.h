#ifndef OLED_H
#define OLED_H

#include <stdbool.h>
#include <stdint.h>

void OLED_Init(void);
void OLED_Clear(void);
void OLED_Update(void);
void OLED_SetPixel(uint8_t x, uint8_t y, bool enabled);
void OLED_ShowStatus(uint8_t mode, uint32_t elapsedMs, bool running);
void OLED_ShowMenu(uint8_t mode, uint32_t elapsedMs, bool running);
void OLED_ShowTrackPattern(
    uint8_t rawHighPattern, uint8_t pattern, bool lineVisible);
void OLED_ShowImuStatus(bool connected, bool calibrated);

#endif
