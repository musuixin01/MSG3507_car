#ifndef VISION_PROTOCOL_H
#define VISION_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

void Vision_Init(void);
void Vision_Poll(uint32_t nowMs);
bool Vision_GetBallPosition(int16_t *positionMm, uint32_t nowMs);

#endif
