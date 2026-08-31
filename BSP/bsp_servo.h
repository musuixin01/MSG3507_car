#ifndef BSP_SERVO_H
#define BSP_SERVO_H

#include <stdint.h>

typedef enum {
    BSP_SERVO_CAMERA_PAN = 0
} BSP_Servo;

void BSP_Servo_Init(void);
void BSP_Servo_SetPulseUs(BSP_Servo servo, uint16_t pulseUs);
void BSP_Servo_SetAngle(BSP_Servo servo, uint8_t angle);

#endif
