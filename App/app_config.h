#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* All competition-specific calibration values are centralized here. */
#define CONTROL_PERIOD_MS              (5U)
#define LINE_BASE_SPEED_TEST          (16U)
#define LINE_BASE_SPEED_FAST          (60U)
#define LINE_BASE_SPEED_BALANCE       (20U)
/* 循迹状态下任何参数组合都不能突破此最终PWM上限。 */
#define LINE_MOTOR_HARD_LIMIT_PERCENT (64.0f)
#define LINE_TURN_SPEED_SHARP          (28U)
/*
 * 连续弯道禁止内轮停转：
 * 内轮目标速度至少保留当前弯道基础速度的 35%，并尽量不低于 4%。
 * 若当前基础速度本身低于 4%（例如起步斜坡），则不强行抬高到 4%。
 */
#define LINE_INNER_WHEEL_MIN_RATIO       (0.25f)
#define LINE_INNER_WHEEL_MIN_PERCENT     (5.0f)
#define LINE_ERROR_FILTER_ALPHA          (0.35f)
#define LINE_ERROR_FULL_SCALE            (30.0f)
#define LINE_ERROR_DEADBAND                (0.035f)
/*
 * 逻辑传感器权重从左到右为 -30 ... +30。
 * 正误差表示黑线位于车体右侧，必须让左轮快、右轮慢，使车向右回线。
 * line_follow.c 中正差速正是“左轮快、右轮慢”，这里不能再次反相。
 */
#define LINE_STEER_DIRECTION               (+1)
#define LINE_STEER_CENTER_KP                (4.0f)
#define LINE_STEER_CENTER_KD               (16.0f)
#define LINE_STEER_CURVE_KP                (24.0f)
#define LINE_STEER_CURVE_KD                (10.0f)
/* 微分项使用相邻控制拍误差增量，并带35%低通滤波。 */
#define LINE_STEER_D_FILTER_ALPHA           (0.25f)
#define LINE_STEER_OUTPUT_LIMIT         (30.0f)
/*
 * 入弯快速降低两轮共同基础速度，出弯缓慢恢复，防止边缘检测时
 * 外侧轮仍保持直线高速。
 */
#define LINE_SPEED_DECEL_PERCENT_PER_TICK      (1.20f)
#define LINE_EDGE_SPEED_DECEL_PERCENT_PER_TICK (3.00f)
#define LINE_SPEED_ACCEL_PERCENT_PER_TICK      (0.25f)
/*
 * 逻辑位图 bit0..bit7 对应最左到最右传感器。
 * 0xC3 = 左侧两路或右侧两路，任一路触发都进入边缘限速。
 */
#define LINE_LEFT_EDGE_SENSOR_MASK        (0x03U)
#define LINE_RIGHT_EDGE_SENSOR_MASK       (0xC0U)
#define LINE_LEFT_PRETURN_SENSOR_MASK      (0x04U)
#define LINE_RIGHT_PRETURN_SENSOR_MASK     (0x20U)
#define LINE_CENTER_LEFT_SENSOR_MASK       (0x08U)
#define LINE_CENTER_RIGHT_SENSOR_MASK      (0x10U)
#define LINE_CENTER_CONFIRM_SAMPLES           (3U)
#define LINE_PRETURN_MIN_NORMALIZED_ERROR    (0.30f)
#define LINE_EDGE_MIN_NORMALIZED_ERROR       (0.80f)
#define LINE_PRETURN_MIN_CURVE_RATIO          (0.45f)
#define LINE_STRAIGHT_CAPTURE_ERROR_THRESHOLD (0.040f)
#define LINE_STRAIGHT_MIN_STEERING_PERCENT    (1.10f)
#define LINE_PRETURN_MIN_STEERING_PERCENT     (8.0f)
#define LINE_EDGE_MIN_STEERING_PERCENT       (22.0f)
/* PID 转向量转换为内侧轮减速量，并单独限制差速变化速度。 */
#define LINE_DIFF_SLEW_PERCENT_PER_TICK        (0.35f)
#define LINE_PRETURN_DIFF_SLEW_PERCENT_PER_TICK (0.85f)
#define LINE_EDGE_DIFF_SLEW_PERCENT_PER_TICK   (2.00f)
#define LINE_DIFF_RELEASE_SLEW_PERCENT_PER_TICK (0.75f)
/*
 * 丢线找回参数（控制周期为 5 ms）：
 * 连续 5 次全白才进入搜索；搜索两轮只能继承原目标或减速，禁止加速。
 * 找回后先用受限速度和转向运行 200 ms，再恢复正常循迹。
 */
#define LINE_LOST_CONFIRM_SAMPLES            (5U)
#define LINE_LOST_DIRECTION_MIN_ERROR        (0.15f)
#define LINE_LOST_DIRECTION_MEMORY_MS        (300U)
#define LINE_LOST_SEARCH_OUTER_PERCENT      (10U)
#define LINE_LOST_SEARCH_INNER_PERCENT       (5U)
#define LINE_LOST_DECEL_PERCENT_PER_TICK     (1.00f)
#define LINE_LOST_SEARCH_TIMEOUT_MS       (1200U)
#define LINE_REACQUIRE_CONFIRM_SAMPLES        (3U)
#define LINE_REACQUIRE_BLEND_MS             (200U)
#define LINE_REACQUIRE_SPEED_PERCENT         (12U)
#define LINE_REACQUIRE_STEER_LIMIT            (6.0f)
/*
 * 数字循迹模块：白底输出高电平，检测到黑线输出低电平。
 * 若 OLED 的 RAW 位图实测相反，只修改此宏，不要改循迹算法。
 */
#define TRACK_ACTIVE_LOW                 (1)
#define TRACK_AUTO_POLARITY              (0)
#define TRACK_SENSOR_ORDER_REVERSED      (1)
#define LINE_START_CONFIRM_SAMPLES        (6U)
#define LINE_START_MAX_ERROR              (3.0f)
/*
 * A 点横线识别：
 * 先连续确认已经离开起点横线，再允许把下一次横线识别为终点。
 * 最短圈时只用于屏蔽起点附近抖动，不再依赖固定 6 秒盲区。
 */
#define A_MARKER_CONFIRM_SAMPLES        (1U)
#define A_MARKER_CLEAR_SAMPLES          (8U)
/* 鍙敤浜庣‘淇濊蒋鍚姩闃舵瀹屽叏椹剁1.8 cm瀹界殑璧风偣绾裤€?*/
#define A_MARKER_ARM_MIN_MS           (1000U)
#define A_MARKER_MIN_LAP_MS          (6000U)
#define A_MARKER_MIN_ACTIVE_SENSORS      (8U)
#define A_MARKER_APPROACH_SAMPLES         (6U)
#define A_MARKER_APPROACH_MAX_ACTIVE      (4U)
#define A_MARKER_APPROACH_MAX_ABS_ERROR   (5.0f)
/* 传感器层必须连续全黑，单帧全黑不会清除循迹误差。 */
#define A_MARKER_DETECT_SAMPLES          (4U)
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
#define SPEED_PI_KP                        (0.25f)
#define SPEED_PI_KI                        (0.015f)
#define SPEED_PI_INTEGRAL_LIMIT           (30.0f)
/* PI最多只比当前目标多补偿2% PWM，边缘10%目标不会冲到高占空比。 */
#define SPEED_PI_OUTPUT_HEADROOM_PERCENT   (2.0f)
#define SPEED_PI_OUTPUT_ACCEL_SLEW_PER_SAMPLE (1.50f)
#define SPEED_PI_OUTPUT_DECEL_SLEW_PER_SAMPLE (6.00f)

/*
 * 编译期安全检查：急弯、边缘和丢线速度禁止高于最低直线基础速度。
 * 参数误配时直接停止编译，避免生成可能冲出赛道的程序。
 */
#if LINE_LOST_SEARCH_OUTER_PERCENT > LINE_TURN_SPEED_SHARP
#error "lost-line outer speed exceeds sharp-turn speed"
#endif
#if LINE_LOST_SEARCH_INNER_PERCENT > LINE_LOST_SEARCH_OUTER_PERCENT
#error "lost-line inner speed exceeds outer speed"
#endif
#if LINE_REACQUIRE_SPEED_PERCENT > LINE_TURN_SPEED_SHARP
#error "line reacquire speed exceeds sharp-turn speed"
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
