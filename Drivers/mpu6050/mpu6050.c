#include "mpu6050.h"

#include <math.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

#define MPU6050_ADDRESS_LOW             (0x68U)
#define MPU6050_ADDRESS_HIGH            (0x69U)

#define MPU6050_REG_SMPLRT_DIV          (0x19U)
#define MPU6050_REG_CONFIG              (0x1AU)
#define MPU6050_REG_GYRO_CONFIG         (0x1BU)
#define MPU6050_REG_ACCEL_CONFIG        (0x1CU)
#define MPU6050_REG_INT_PIN_CFG         (0x37U)
#define MPU6050_REG_INT_ENABLE          (0x38U)
#define MPU6050_REG_ACCEL_XOUT_H        (0x3BU)
#define MPU6050_REG_SIGNAL_PATH_RESET   (0x68U)
#define MPU6050_REG_USER_CTRL           (0x6AU)
#define MPU6050_REG_PWR_MGMT_1          (0x6BU)
#define MPU6050_REG_PWR_MGMT_2          (0x6CU)
#define MPU6050_REG_WHO_AM_I            (0x75U)

#define MPU6050_WHO_AM_I_MASK           (0x7EU)
#define MPU6050_WHO_AM_I_VALUE          (0x68U)
#define MPU6050_I2C_TIMEOUT_LOOPS        (200000UL)
#define MPU6050_DEFAULT_CAL_SAMPLES      (200U)
#define MPU6050_ACCEL_SCALE_4G           (8192.0f)
#define MPU6050_GYRO_SCALE_500DPS        (65.5f)
#define MPU6050_RAD_TO_DEG               (57.2957795f)
#define MPU6050_COMPLEMENTARY_ALPHA      (0.98f)

static uint8_t g_address;
static bool g_connected;
static bool g_calibrated;
static bool g_attitudeStarted;
static volatile bool g_dataReady;
static uint8_t g_readErrorCount;
static uint16_t g_calibrationTarget;
static uint16_t g_calibrationCount;
static int32_t g_gyroSumX;
static int32_t g_gyroSumY;
static int32_t g_gyroSumZ;
static float g_gyroBiasX;
static float g_gyroBiasY;
static float g_gyroBiasZ;
static MPU6050_Data g_raw;
static MPU6050_Motion g_motion;

static void recoverTransfer(void)
{
    DL_I2C_flushControllerTXFIFO(MPU6050_INST);
    while (!DL_I2C_isControllerRXFIFOEmpty(MPU6050_INST)) {
        (void) DL_I2C_receiveControllerData(MPU6050_INST);
    }
    DL_I2C_resetControllerTransfer(MPU6050_INST);
}

static bool waitUntilIdle(void)
{
    uint32_t timeout = MPU6050_I2C_TIMEOUT_LOOPS;

    while ((DL_I2C_getControllerStatus(MPU6050_INST) &
            DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) {
        if (--timeout == 0U) {
            recoverTransfer();
            return false;
        }
    }
    return true;
}

static bool waitUntilBusReleased(void)
{
    uint32_t timeout = MPU6050_I2C_TIMEOUT_LOOPS;

    while ((DL_I2C_getControllerStatus(MPU6050_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY_BUS) != 0U) {
        if (--timeout == 0U) {
            recoverTransfer();
            return false;
        }
    }

    if ((DL_I2C_getControllerStatus(MPU6050_INST) &
         DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        recoverTransfer();
        return false;
    }
    return true;
}

static bool writeRegisterAt(uint8_t address, uint8_t reg, uint8_t value)
{
    uint8_t tx[2];

    tx[0] = reg;
    tx[1] = value;
    DL_I2C_flushControllerTXFIFO(MPU6050_INST);
    if (!waitUntilIdle()) return false;
    if (DL_I2C_fillControllerTXFIFO(MPU6050_INST, tx, 2U) != 2U) {
        recoverTransfer();
        return false;
    }
    DL_I2C_startControllerTransfer(MPU6050_INST, address,
        DL_I2C_CONTROLLER_DIRECTION_TX, 2U);
    return waitUntilBusReleased();
}

static bool readRegistersAt(
    uint8_t address, uint8_t reg, uint8_t *data, uint8_t length)
{
    uint8_t index;

    if ((data == NULL) || (length == 0U)) return false;

    DL_I2C_flushControllerTXFIFO(MPU6050_INST);
    if (!waitUntilIdle()) return false;
    if (DL_I2C_fillControllerTXFIFO(MPU6050_INST, &reg, 1U) != 1U) {
        recoverTransfer();
        return false;
    }
    DL_I2C_startControllerTransfer(MPU6050_INST, address,
        DL_I2C_CONTROLLER_DIRECTION_TX, 1U);
    if (!waitUntilBusReleased() || !waitUntilIdle()) return false;

    DL_I2C_startControllerTransfer(MPU6050_INST, address,
        DL_I2C_CONTROLLER_DIRECTION_RX, length);
    for (index = 0U; index < length; index++) {
        uint32_t timeout = MPU6050_I2C_TIMEOUT_LOOPS;
        while (DL_I2C_isControllerRXFIFOEmpty(MPU6050_INST)) {
            if (--timeout == 0U) {
                recoverTransfer();
                return false;
            }
        }
        data[index] = DL_I2C_receiveControllerData(MPU6050_INST);
    }
    return waitUntilBusReleased();
}

static int16_t makeInt16(uint8_t high, uint8_t low)
{
    return (int16_t) (((uint16_t) high << 8) | low);
}

static bool stationaryForCalibration(const MPU6050_Motion *motion)
{
    float accelMagnitudeSquared =
        motion->accelX_g * motion->accelX_g +
        motion->accelY_g * motion->accelY_g +
        motion->accelZ_g * motion->accelZ_g;

    return (accelMagnitudeSquared > 0.7225f) &&
           (accelMagnitudeSquared < 1.3225f) &&
           (fabsf(motion->gyroX_dps) < 10.0f) &&
           (fabsf(motion->gyroY_dps) < 10.0f) &&
           (fabsf(motion->gyroZ_dps) < 10.0f);
}

bool MPU6050_Init(void)
{
    uint8_t whoAmI = 0U;
    uint8_t address;

    g_address = 0U;
    g_connected = false;
    g_calibrated = false;
    g_attitudeStarted = false;
    g_dataReady = false;
    g_readErrorCount = 0U;
    g_calibrationTarget = 0U;
    g_calibrationCount = 0U;
    g_gyroBiasX = 0.0f;
    g_gyroBiasY = 0.0f;
    g_gyroBiasZ = 0.0f;

    for (address = MPU6050_ADDRESS_LOW;
         address <= MPU6050_ADDRESS_HIGH; address++) {
        if (readRegistersAt(address, MPU6050_REG_WHO_AM_I, &whoAmI, 1U) &&
            ((whoAmI & MPU6050_WHO_AM_I_MASK) ==
             MPU6050_WHO_AM_I_VALUE)) {
            g_address = address;
            break;
        }
    }
    if (g_address == 0U) return false;

    /*
     * 200 Hz sample rate, 44 Hz DLPF, accelerometer +/-4 g,
     * gyroscope +/-500 dps, active-low data-ready pulse on PB4.
     */
    if (!writeRegisterAt(g_address, MPU6050_REG_PWR_MGMT_1, 0x01U) ||
        !writeRegisterAt(g_address, MPU6050_REG_PWR_MGMT_2, 0x00U) ||
        !writeRegisterAt(g_address, MPU6050_REG_USER_CTRL, 0x00U) ||
        !writeRegisterAt(g_address, MPU6050_REG_SIGNAL_PATH_RESET, 0x07U)) {
        return false;
    }
    delay_cycles(CPUCLK_FREQ / 100U);

    if (!writeRegisterAt(g_address, MPU6050_REG_SMPLRT_DIV, 0x04U) ||
        !writeRegisterAt(g_address, MPU6050_REG_CONFIG, 0x03U) ||
        !writeRegisterAt(g_address, MPU6050_REG_GYRO_CONFIG, 0x08U) ||
        !writeRegisterAt(g_address, MPU6050_REG_ACCEL_CONFIG, 0x08U) ||
        !writeRegisterAt(g_address, MPU6050_REG_INT_PIN_CFG, 0x80U) ||
        !writeRegisterAt(g_address, MPU6050_REG_INT_ENABLE, 0x01U)) {
        return false;
    }

    g_connected = true;
    g_readErrorCount = 0U;
    MPU6050_StartGyroCalibration(MPU6050_DEFAULT_CAL_SAMPLES);
    return true;
}

bool MPU6050_Read(MPU6050_Data *data)
{
    uint8_t buffer[14];

    if (!g_connected || (data == NULL) ||
        !readRegistersAt(
            g_address, MPU6050_REG_ACCEL_XOUT_H, buffer, 14U)) {
        return false;
    }

    data->accelX = makeInt16(buffer[0], buffer[1]);
    data->accelY = makeInt16(buffer[2], buffer[3]);
    data->accelZ = makeInt16(buffer[4], buffer[5]);
    data->temperature = makeInt16(buffer[6], buffer[7]);
    data->gyroX = makeInt16(buffer[8], buffer[9]);
    data->gyroY = makeInt16(buffer[10], buffer[11]);
    data->gyroZ = makeInt16(buffer[12], buffer[13]);
    return true;
}

bool MPU6050_Update(float deltaSeconds)
{
    float accelRoll;
    float accelPitch;

    if (!g_connected) return false;

    /*
     * PB4 normally releases this flag at 200 Hz. Reading is also allowed
     * without the flag, so a disconnected INT wire cannot stop I2C data.
     */
    g_dataReady = false;
    if (!MPU6050_Read(&g_raw)) {
        if (++g_readErrorCount >= 3U) {
            g_connected = false;
        }
        return false;
    }
    g_readErrorCount = 0U;

    g_motion.accelX_g = (float) g_raw.accelX / MPU6050_ACCEL_SCALE_4G;
    g_motion.accelY_g = (float) g_raw.accelY / MPU6050_ACCEL_SCALE_4G;
    g_motion.accelZ_g = (float) g_raw.accelZ / MPU6050_ACCEL_SCALE_4G;
    g_motion.gyroX_dps =
        ((float) g_raw.gyroX - g_gyroBiasX) / MPU6050_GYRO_SCALE_500DPS;
    g_motion.gyroY_dps =
        ((float) g_raw.gyroY - g_gyroBiasY) / MPU6050_GYRO_SCALE_500DPS;
    g_motion.gyroZ_dps =
        ((float) g_raw.gyroZ - g_gyroBiasZ) / MPU6050_GYRO_SCALE_500DPS;
    g_motion.temperature_c =
        ((float) g_raw.temperature / 340.0f) + 36.53f;

    if (!g_calibrated && (g_calibrationTarget != 0U) &&
        stationaryForCalibration(&g_motion)) {
        g_gyroSumX += g_raw.gyroX;
        g_gyroSumY += g_raw.gyroY;
        g_gyroSumZ += g_raw.gyroZ;
        g_calibrationCount++;
        if (g_calibrationCount >= g_calibrationTarget) {
            g_gyroBiasX = (float) g_gyroSumX / g_calibrationCount;
            g_gyroBiasY = (float) g_gyroSumY / g_calibrationCount;
            g_gyroBiasZ = (float) g_gyroSumZ / g_calibrationCount;
            g_calibrated = true;
        }
    }

    accelRoll = atan2f(g_motion.accelY_g, g_motion.accelZ_g) *
        MPU6050_RAD_TO_DEG;
    accelPitch = atan2f(-g_motion.accelX_g,
        sqrtf(g_motion.accelY_g * g_motion.accelY_g +
              g_motion.accelZ_g * g_motion.accelZ_g)) *
        MPU6050_RAD_TO_DEG;

    if (!g_attitudeStarted || (deltaSeconds <= 0.0f) ||
        (deltaSeconds > 0.1f)) {
        g_motion.roll_deg = accelRoll;
        g_motion.pitch_deg = accelPitch;
        g_attitudeStarted = true;
    } else {
        g_motion.roll_deg = MPU6050_COMPLEMENTARY_ALPHA *
            (g_motion.roll_deg + g_motion.gyroX_dps * deltaSeconds) +
            (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * accelRoll;
        g_motion.pitch_deg = MPU6050_COMPLEMENTARY_ALPHA *
            (g_motion.pitch_deg + g_motion.gyroY_dps * deltaSeconds) +
            (1.0f - MPU6050_COMPLEMENTARY_ALPHA) * accelPitch;
    }
    return true;
}

void MPU6050_StartGyroCalibration(uint16_t sampleCount)
{
    g_calibrationTarget = (sampleCount == 0U) ?
        MPU6050_DEFAULT_CAL_SAMPLES : sampleCount;
    g_calibrationCount = 0U;
    g_gyroSumX = 0;
    g_gyroSumY = 0;
    g_gyroSumZ = 0;
    g_gyroBiasX = 0.0f;
    g_gyroBiasY = 0.0f;
    g_gyroBiasZ = 0.0f;
    g_calibrated = false;
}

void MPU6050_NotifyDataReadyIRQ(void)
{
    g_dataReady = true;
}

bool MPU6050_IsConnected(void)
{
    return g_connected;
}

bool MPU6050_IsCalibrated(void)
{
    return g_calibrated;
}

uint8_t MPU6050_GetAddress(void)
{
    return g_address;
}

const MPU6050_Data *MPU6050_GetRawData(void)
{
    return &g_raw;
}

const MPU6050_Motion *MPU6050_GetMotion(void)
{
    return &g_motion;
}
