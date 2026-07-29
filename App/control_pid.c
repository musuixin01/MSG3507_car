#include "control_pid.h"

static float clampFloat(float value, float limit)
{
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

void PID_Init(ControlPID *pid, float kp, float ki, float kd,
    float integralLimit, float outputLimit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->integralLimit = integralLimit;
    pid->outputLimit = outputLimit;
    PID_Reset(pid);
}

void PID_Reset(ControlPID *pid)
{
    pid->integral = 0.0f;
    pid->previousError = 0.0f;
    pid->filteredDerivative = 0.0f;
    pid->initialized = false;
}

float PID_Update(ControlPID *pid, float setpoint, float measurement, float dt)
{
    float error = setpoint - measurement;
    float rawDerivative = 0.0f;
    float candidateIntegral;
    float candidateOutput;
    float output;

    if (dt <= 0.0f) {
        return 0.0f;
    }
    if (dt > 0.02f) {
        dt = 0.02f;
    }
    if (pid->initialized) {
        rawDerivative = (error - pid->previousError) / dt;
        /*
         * First-order derivative low-pass filter. Keeping only 30% of the
         * new derivative suppresses digital line-sensor edge impulses.
         */
        pid->filteredDerivative +=
            (rawDerivative - pid->filteredDerivative) * 0.30f;
    } else {
        pid->initialized = true;
        pid->filteredDerivative = 0.0f;
    }

    candidateIntegral = clampFloat(
        pid->integral + (error * dt), pid->integralLimit);
    candidateOutput = (pid->kp * error) +
        (pid->ki * candidateIntegral) +
        (pid->kd * pid->filteredDerivative);

    /*
     * Conditional integration: freeze the integrator while saturated unless
     * the current error drives the output back toward the linear region.
     */
    if (((candidateOutput < pid->outputLimit) &&
         (candidateOutput > -pid->outputLimit)) ||
        ((candidateOutput >= pid->outputLimit) && (error < 0.0f)) ||
        ((candidateOutput <= -pid->outputLimit) && (error > 0.0f))) {
        pid->integral = candidateIntegral;
    }

    output = (pid->kp * error) + (pid->ki * pid->integral) +
        (pid->kd * pid->filteredDerivative);
    pid->previousError = error;
    return clampFloat(output, pid->outputLimit);
}
