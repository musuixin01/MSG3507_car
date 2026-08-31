#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* ======================== 控制周期 ======================== */
#define CONTROL_PERIOD_MS                     (5U)

/* ======================== 五路循迹 ======================== */
#define LINE_SENSOR_COUNT                     (5U)
#define TRACK_ACTIVE_LOW                      (1)

/* 调试阶段先使用低速；直线可靠后再逐级提高快速模式速度。 */
#define LINE_BASE_SPEED_TEST                 (16U)
#define LINE_BASE_SPEED_FAST                 (30U)
#define LINE_BASE_SPEED_BALANCE              (20U)
#define LINE_MOTOR_HARD_LIMIT_PERCENT        (64.0f)
#define LINE_CURVE_SPEED                     (18.0f)
#define LINE_PRETURN_SPEED                   (24.0f)

/* 位置误差权重为 -4、-2、0、2、4；正误差表示黑线在车体右侧。 */
#define LINE_ERROR_FILTER_ALPHA               (0.30f)
#define LINE_ERROR_DEADBAND                   (0.08f)
#define LINE_D_FILTER_ALPHA                   (0.25f)
#define LINE_PD_KP                            (3.20f)
#define LINE_PD_KD                            (4.50f)
#define LINE_STEERING_LIMIT                  (20.0f)
#define LINE_PRETURN_MIN_STEERING             (7.0f)
#define LINE_EDGE_MIN_STEERING               (16.0f)
#define LINE_DIRECTION_MEMORY_MIN_ERROR       (0.50f)

/* OUT1/OUT5 为大弯边缘，OUT2/OUT4 为预弯，OUT3 为直线中心。 */
#define LINE_LEFT_EDGE_MASK                   (0x01U)
#define LINE_LEFT_PRETURN_MASK                (0x02U)
#define LINE_RIGHT_PRETURN_MASK               (0x08U)
#define LINE_RIGHT_EDGE_MASK                  (0x10U)

#define LINE_NORMAL_STEER_SLEW                (2.0f)
#define LINE_EDGE_STEER_SLEW                  (5.0f)
#define LINE_SPEED_ACCEL_PER_TICK             (0.50f)
#define LINE_SPEED_DECEL_PER_TICK             (2.00f)
#define LINE_EDGE_SPEED_DECEL_PER_TICK        (4.00f)
#define LINE_INNER_WHEEL_MIN_RATIO            (0.30f)
#define LINE_INNER_WHEEL_MIN_SPEED            (4.0f)

/* 启动、横线和丢线安全参数。 */
#define LINE_START_CONFIRM_SAMPLES            (4U)
#define LINE_START_MAX_ABS_ERROR              (1.0f)
#define LINE_MARKER_CONFIRM_SAMPLES           (4U)
#define LINE_LOST_CONFIRM_SAMPLES             (5U)
#define LINE_LOST_OUTER_SPEED                (10U)
#define LINE_LOST_INNER_SPEED                 (5U)
#define LINE_LOST_DECEL_PER_TICK              (2.0f)
#define LINE_LOST_TIMEOUT_MS               (1200U)
#define LINE_REACQUIRE_SPEED                 (12U)

/* 直线基线必须先保持两轮目标一致，再依据稳定实测单独标定。 */
#define MOTOR_LEFT_GAIN_PERCENT             (100U)
#define MOTOR_RIGHT_GAIN_PERCENT            (100U)
#define MOTOR_LEFT_DIRECTION_REVERSED         (1)
#define MOTOR_RIGHT_DIRECTION_REVERSED        (1)

/* ======================== 左右轮速度 PI ======================== */
/* 当前参考计数未经重新标定，保持关闭。 */
#define SPEED_PI_ENABLE                       (0)
#define SPEED_PI_CALIBRATION_MODE             (0)
#define SPEED_PI_SAMPLE_DIVIDER               (4U)
#define SPEED_PI_REFERENCE_PWM_PERCENT       (16.0f)
#define SPEED_PI_LEFT_REFERENCE_COUNTS      (144.0f)
#define SPEED_PI_RIGHT_REFERENCE_COUNTS     (144.0f)
#define SPEED_PI_KP                           (0.25f)
#define SPEED_PI_KI                           (0.015f)
#define SPEED_PI_INTEGRAL_LIMIT              (30.0f)
#define SPEED_PI_OUTPUT_HEADROOM_PERCENT      (2.0f)
#define SPEED_PI_OUTPUT_ACCEL_SLEW_PER_SAMPLE (1.50f)
#define SPEED_PI_OUTPUT_DECEL_SLEW_PER_SAMPLE (6.00f)

/* ======================== A 点停车 ======================== */
#define A_MARKER_CONFIRM_SAMPLES              (1U)
#define A_MARKER_CLEAR_SAMPLES                (8U)
#define A_MARKER_MIN_LAP_MS                 (6000U)

/* ======================== 赛题其他任务 ======================== */
#define AB_TARGET_ENCODER_COUNTS            (7200U)
#define BALL_MEASUREMENT_TIMEOUT_MS          (100U)
#define TASK6_TARGET_MM                        (50)

#define TASK3_CENTER_PULSE_US                (1720U)
#define TASK3_DOWN_TO_POSITIVE_5CM_PULSE_US (1400U)
#define TASK3_UP_TO_NEGATIVE_5CM_PULSE_US   (1800U)
#define TASK3_NEGATIVE_HOLD_PULSE_US \
    (TASK3_UP_TO_NEGATIVE_5CM_PULSE_US)
#define TASK3_CENTER_SETTLE_MS                (550U)
#define TASK3_DOWN_TO_POSITIVE_5CM_MS         (900U)
#define TASK3_RETURN_TO_CENTER_MS             (650U)
#define TASK3_UP_TO_NEGATIVE_5CM_MS          (1800U)
#define TASK3_NEGATIVE_SETTLE_MS              (850U)
#define TASK3_SERVO_SLEW_US_PER_TICK            (2U)

#if ((TASK3_CENTER_SETTLE_MS + TASK3_DOWN_TO_POSITIVE_5CM_MS + \
      TASK3_RETURN_TO_CENTER_MS + TASK3_UP_TO_NEGATIVE_5CM_MS + \
      TASK3_NEGATIVE_SETTLE_MS) > 5000U)
#error "task 3 open-loop duration exceeds 5 seconds"
#endif

#define BALANCE_SERVO_DIRECTION                (1)

/* ======================== 编译期安全检查 ======================== */
#if LINE_SENSOR_COUNT != 5U
#error "this line-follow implementation requires five sensors"
#endif

#if LINE_LOST_INNER_SPEED > LINE_LOST_OUTER_SPEED
#error "lost-line inner speed exceeds outer speed"
#endif

#endif
