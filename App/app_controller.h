#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    APP_MODE_VIDEO = 1,
    APP_MODE_LAP_FAST,
    APP_MODE_BALL_STATIC,
    APP_MODE_AB_BALANCE,
    APP_MODE_LAP_CENTER,
    APP_MODE_LAP_POSITION
} AppMode;

void AppController_Init(void);
void AppController_Update(uint32_t nowMs);
void AppController_SelectMode(AppMode mode);
void AppController_Start(uint32_t nowMs);
void AppController_Stop(uint32_t nowMs);
void AppController_Abort(void);
bool AppController_IsRunning(void);
uint32_t AppController_GetElapsedMs(uint32_t nowMs);
AppMode AppController_GetMode(void);

#endif
