#ifndef CONTROL_PID_H
#define CONTROL_PID_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float previousError;
    float filteredDerivative;
    float integralLimit;
    float outputLimit;
    bool initialized;
} ControlPID;

void PID_Init(ControlPID *pid, float kp, float ki, float kd,
    float integralLimit, float outputLimit);
void PID_Reset(ControlPID *pid);
float PID_Update(ControlPID *pid, float setpoint, float measurement, float dt);

#endif
