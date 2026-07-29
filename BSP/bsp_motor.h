#ifndef BSP_MOTOR_H
#define BSP_MOTOR_H

#include <stdint.h>

typedef enum {
    BSP_MOTOR_LEFT = 0,
    BSP_MOTOR_RIGHT
} BSP_Motor;

void BSP_Motor_Init(void);
void BSP_Motor_Set(BSP_Motor motor, int8_t percent);
void BSP_Motor_StopAll(void);
void BSP_Motor_BrakeAll(void);

#endif
