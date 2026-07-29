#ifndef BSP_BUZZER_H
#define BSP_BUZZER_H

#include <stdint.h>

void BSP_Buzzer_Init(void);
void BSP_Buzzer_Update(uint32_t nowMs);
void BSP_Buzzer_PlayStartup(uint32_t nowMs);
void BSP_Buzzer_PlayArmed(uint32_t nowMs);
void BSP_Buzzer_PlayComplete(uint32_t nowMs);

#endif
