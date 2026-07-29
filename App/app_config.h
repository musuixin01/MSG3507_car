#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* All competition-specific calibration values are centralized here. */
#define CONTROL_PERIOD_MS              (5U)
#define LINE_BASE_SPEED_TEST          (16U)
#define LINE_BASE_SPEED_FAST          (26U)
#define LINE_BASE_SPEED_BALANCE       (20U)
/* 循迹状态下任何参数组合都不能突破此最终PWM上限。 */
#define LINE_MOTOR_HARD_LIMIT_PERCENT (30.0f)
#define LINE_TURN_SPEED_SHARP          (5U)
#define LINE_ERROR_FILTER_ALPHA       (0.20f)
#define LINE_ERROR_FULL_SCALE          (7.0f)
#define LINE_STEER_ERROR_SCALE         (2.8f)
#define LINE_ERROR_DEADBAND             (0.15f)
#define LINE_LOOKAHEAD_GAIN             (0.60f)
#define LINE_STEER_SPEED_REDUCTION_GAIN (0.45f)
/*
 * Sensor convention: OUT1 is the -7 (left) side and OUT8 is the +7
 * (right) side. Positive error must speed up the left wheel.
 */
#define LINE_STEER_DIRECTION               (-1)
#define LINE_STEER_KP                   (22.0f)
/* 微分项用于抑制急弯后的摆动，内部带 30% 低通滤波。 */
#define LINE_STEER_KD                   (0.035f)
#define LINE_STEER_OUTPUT_LIMIT         (30.0f)
#define LINE_STEER_SLEW_PERCENT_PER_TICK (0.60f)
/* 大弯允许内侧轮接近停止，提高半径 0.5 m 弯道的转向能力。 */
#define LINE_STEER_MAX_SPEED_RATIO        (0.92f)
/*
 * 入弯快速降低两轮共同基础速度，出弯缓慢恢复，防止边缘检测时
 * 外侧轮仍保持直线高速。
 */
#define LINE_SPEED_DECEL_PERCENT_PER_TICK (2.00f)
#define LINE_SPEED_ACCEL_PERCENT_PER_TICK (0.18f)
#define LINE_EDGE_SPEED_LIMIT_PERCENT      (5U)
/*
 * 逻辑位图 bit0..bit7 对应最左到最右传感器。
 * 0xC3 = 左侧两路或右侧两路，任一路触发都进入边缘限速。
 */
#define LINE_EDGE_SENSOR_MASK             (0xC3U)
#define LINE_LEFT_EDGE_SENSOR_MASK        (0x03U)
#define LINE_RIGHT_EDGE_SENSOR_MASK       (0xC0U)
#define LINE_LINEAR_TURN_GAIN              (1.00f)
#define LINE_LINEAR_STEER_SLEW_PER_TICK    (0.08f)
/* PID 转向量转换为内侧轮减速量，并单独限制差速变化速度。 */
#define LINE_DIFF_STEER_GAIN              (1.20f)
#define LINE_DIFF_SLEW_PERCENT_PER_TICK   (0.45f)
#define LINE_EDGE_DIFF_SLEW_PERCENT_PER_TICK (0.20f)
#define LINE_LOST_SPEED_SLEW_PERCENT_PER_TICK (0.60f)
/*
 * 数字循迹模块：白底输出高电平，检测到黑线输出低电平。
 * 若 OLED 的 RAW 位图实测相反，只修改此宏，不要改循迹算法。
 */
#define TRACK_ACTIVE_LOW                 (1)
#define TRACK_AUTO_POLARITY              (0)
#define TRACK_SENSOR_ORDER_REVERSED      (1)
#define LINE_START_CONFIRM_SAMPLES        (6U)
#define LINE_START_MAX_ERROR              (2.0f)
/*
 * A 点横线识别：
 * 先连续确认已经离开起点横线，再允许把下一次横线识别为终点。
 * 最短圈时只用于屏蔽起点附近抖动，不再依赖固定 6 秒盲区。
 */
#define A_MARKER_MIN_LAP_MS          (6000U)
#define A_MARKER_CONFIRM_SAMPLES        (3U)
#define A_MARKER_CLEAR_SAMPLES          (8U)
#define A_MARKER_MIN_ACTIVE_SENSORS      (8U)
/* 传感器层必须连续全黑，单帧全黑不会清除循迹误差。 */
#define A_MARKER_DETECT_SAMPLES          (5U)
#define MOTOR_LEFT_GAIN_PERCENT       (100U)
#define MOTOR_RIGHT_GAIN_PERCENT      (100U)
#define MOTOR_LEFT_DIRECTION_REVERSED    (1)
#define MOTOR_RIGHT_DIRECTION_REVERSED   (1)

/*
 * 左右轮速度 PI 标定：
 * 1. 保持 SPEED_PI_ENABLE=0，架空车轮以参考 PWM 运行；
 * 2. 读取 OLED/调试器中的左右 20 ms 编码器增量；
 * 3. 分别填入 LEFT/RIGHT_REFERENCE_COUNTS；
 * 4. 将 SPEED_PI_ENABLE 改为 1。
 */
#define SPEED_PI_ENABLE                    (1)
/* 标定完成后改为0，模式1恢复普通低速循迹。 */
#define SPEED_PI_CALIBRATION_MODE           (0)
#define SPEED_PI_SAMPLE_DIVIDER            (4U)
#define SPEED_PI_REFERENCE_PWM_PERCENT    (16.0f)
#define SPEED_PI_LEFT_REFERENCE_COUNTS   (144.0f)
#define SPEED_PI_RIGHT_REFERENCE_COUNTS  (144.0f)
#define SPEED_PI_KP                        (0.35f)
#define SPEED_PI_KI                        (0.04f)
#define SPEED_PI_INTEGRAL_LIMIT           (30.0f)
/* PI最多只比当前目标多补偿2% PWM，边缘10%目标不会冲到高占空比。 */
#define SPEED_PI_OUTPUT_HEADROOM_PERCENT   (2.0f)
#define LINE_LOST_HOLD_MS                (25U)
#define LINE_LOST_HOLD_SPEED_PERCENT        (8)
#define LINE_LOST_SEARCH_BASE_PERCENT      (10)
#define LINE_LOST_SEARCH_CORRECTION_PERCENT (14)
#define LINE_LOST_STEER_SLEW_PERCENT_PER_TICK (1.20f)
#define LINE_LOST_STOP_MS              (3500U)

/*
 * 编译期安全检查：急弯、边缘和丢线速度禁止高于最低直线基础速度。
 * 参数误配时直接停止编译，避免生成可能冲出赛道的程序。
 */
#if LINE_TURN_SPEED_SHARP > LINE_BASE_SPEED_TEST
#error "LINE_TURN_SPEED_SHARP exceeds straight speed"
#endif
#if LINE_EDGE_SPEED_LIMIT_PERCENT > LINE_BASE_SPEED_TEST
#error "LINE_EDGE_SPEED_LIMIT_PERCENT exceeds straight speed"
#endif
#if LINE_LOST_HOLD_SPEED_PERCENT > LINE_BASE_SPEED_TEST
#error "LINE_LOST_HOLD_SPEED_PERCENT exceeds straight speed"
#endif
#if LINE_LOST_SEARCH_BASE_PERCENT > LINE_BASE_SPEED_TEST
#error "LINE_LOST_SEARCH_BASE_PERCENT exceeds straight speed"
#endif
#define AB_TARGET_ENCODER_COUNTS     (7200U)
#define TASK3_STAGE_TIMEOUT_MS       (2500U)
#define BALL_MEASUREMENT_TIMEOUT_MS   (100U)
#define TASK6_TARGET_MM                 (50)

/*
 * Set to 1 if increasing servo pulse moves the ball toward +X.
 * If the first bench test moves away from the target, invert it.
 */
#define BALANCE_SERVO_DIRECTION          (1)

#endif
