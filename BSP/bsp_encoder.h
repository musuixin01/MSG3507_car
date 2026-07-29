#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#include <stdint.h>

void BSP_Encoder_Init(void);
int32_t BSP_Encoder_GetLeft(void);
uint16_t BSP_Encoder_GetRight(void);
void BSP_Encoder_Reset(void);

#endif
