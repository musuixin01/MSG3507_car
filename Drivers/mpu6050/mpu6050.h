#ifndef MPU6050_H
#define MPU6050_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
    int16_t temperature;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
} MPU6050_Data;

typedef struct {
    float accelX_g;
    float accelY_g;
    float accelZ_g;
    float gyroX_dps;
    float gyroY_dps;
    float gyroZ_dps;
    float temperature_c;
    float roll_deg;
    float pitch_deg;
} MPU6050_Motion;

/*
 * The driver automatically probes both legal MPU6050 addresses (0x68/0x69).
 * Call Update() every CONTROL_PERIOD_MS. The first stationary samples are
 * used to remove gyroscope zero bias without blocking the control loop.
 */
bool MPU6050_Init(void);
bool MPU6050_Read(MPU6050_Data *data);
bool MPU6050_Update(float deltaSeconds);
void MPU6050_StartGyroCalibration(uint16_t sampleCount);
void MPU6050_NotifyDataReadyIRQ(void);

bool MPU6050_IsConnected(void);
bool MPU6050_IsCalibrated(void);
uint8_t MPU6050_GetAddress(void);
const MPU6050_Data *MPU6050_GetRawData(void);
const MPU6050_Motion *MPU6050_GetMotion(void);

#endif
