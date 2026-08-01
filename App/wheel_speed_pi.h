#ifndef WHEEL_SPEED_PI_H
#define WHEEL_SPEED_PI_H

#include <stdint.h>

/* 供 Keil Watch 直接观察，volatile 可避免优化后无法求值。 */
extern volatile uint16_t g_wheelSpeedLeftDelta;
extern volatile uint16_t g_wheelSpeedRightDelta;
extern volatile uint16_t g_wheelSpeedLeftTargetX10;
extern volatile uint16_t g_wheelSpeedRightTargetX10;
extern volatile uint16_t g_wheelSpeedLeftOutputX10;
extern volatile uint16_t g_wheelSpeedRightOutputX10;

/*
 * 左右轮独立速度 PI。
 * targetPercent 表示相对于标定基准速度的百分比，不直接等同于 PWM。
 */
void WheelSpeedPI_Init(void);
void WheelSpeedPI_Reset(void);
void WheelSpeedPI_Update(float leftTargetPercent,
    float rightTargetPercent);
void WheelSpeedPI_RunCalibration(float pwmPercent);

#endif
