#ifndef BALL_CONTROL_H
#define BALL_CONTROL_H

#include <stdbool.h>
#include <stdint.h>

void BallControl_Init(void);
void BallControl_SetTargetMm(int16_t positionMm);
void BallControl_Update(int16_t measuredMm, bool measurementValid);
int16_t BallControl_GetTargetMm(void);
bool BallControl_IsStable(void);

#endif
