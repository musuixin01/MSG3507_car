/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for MOTOR_PWM */
#define MOTOR_PWM_INST                                                     TIMG0
#define MOTOR_PWM_INST_IRQHandler                               TIMG0_IRQHandler
#define MOTOR_PWM_INST_INT_IRQN                                 (TIMG0_INT_IRQn)
#define MOTOR_PWM_INST_CLK_FREQ                                         32000000
/* GPIO defines for channel 0 */
#define GPIO_MOTOR_PWM_C0_PORT                                             GPIOA
#define GPIO_MOTOR_PWM_C0_PIN                                     DL_GPIO_PIN_12
#define GPIO_MOTOR_PWM_C0_IOMUX                                  (IOMUX_PINCM34)
#define GPIO_MOTOR_PWM_C0_IOMUX_FUNC                 IOMUX_PINCM34_PF_TIMG0_CCP0
#define GPIO_MOTOR_PWM_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_MOTOR_PWM_C1_PORT                                             GPIOA
#define GPIO_MOTOR_PWM_C1_PIN                                     DL_GPIO_PIN_13
#define GPIO_MOTOR_PWM_C1_IOMUX                                  (IOMUX_PINCM35)
#define GPIO_MOTOR_PWM_C1_IOMUX_FUNC                 IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_MOTOR_PWM_C1_IDX                                DL_TIMER_CC_1_INDEX

/* Defines for BUZZER_PWM */
#define BUZZER_PWM_INST                                                    TIMG7
#define BUZZER_PWM_INST_IRQHandler                              TIMG7_IRQHandler
#define BUZZER_PWM_INST_INT_IRQN                                (TIMG7_INT_IRQn)
#define BUZZER_PWM_INST_CLK_FREQ                                        32000000
/* GPIO defines for channel 1 */
#define GPIO_BUZZER_PWM_C1_PORT                                            GPIOA
#define GPIO_BUZZER_PWM_C1_PIN                                     DL_GPIO_PIN_7
#define GPIO_BUZZER_PWM_C1_IOMUX                                 (IOMUX_PINCM14)
#define GPIO_BUZZER_PWM_C1_IOMUX_FUNC                IOMUX_PINCM14_PF_TIMG7_CCP1
#define GPIO_BUZZER_PWM_C1_IDX                               DL_TIMER_CC_1_INDEX

/* Defines for SERVO_PWM */
#define SERVO_PWM_INST                                                     TIMA0
#define SERVO_PWM_INST_IRQHandler                               TIMA0_IRQHandler
#define SERVO_PWM_INST_INT_IRQN                                 (TIMA0_INT_IRQn)
#define SERVO_PWM_INST_CLK_FREQ                                          3200000
/* GPIO defines for channel 0 */
#define GPIO_SERVO_PWM_C0_PORT                                             GPIOA
#define GPIO_SERVO_PWM_C0_PIN                                     DL_GPIO_PIN_21
#define GPIO_SERVO_PWM_C0_IOMUX                                  (IOMUX_PINCM46)
#define GPIO_SERVO_PWM_C0_IOMUX_FUNC                 IOMUX_PINCM46_PF_TIMA0_CCP0
#define GPIO_SERVO_PWM_C0_IDX                                DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_SERVO_PWM_C1_PORT                                             GPIOA
#define GPIO_SERVO_PWM_C1_PIN                                     DL_GPIO_PIN_22
#define GPIO_SERVO_PWM_C1_IOMUX                                  (IOMUX_PINCM47)
#define GPIO_SERVO_PWM_C1_IOMUX_FUNC                 IOMUX_PINCM47_PF_TIMA0_CCP1
#define GPIO_SERVO_PWM_C1_IDX                                DL_TIMER_CC_1_INDEX
/* GPIO defines for channel 2 */
#define GPIO_SERVO_PWM_C2_PORT                                             GPIOA
#define GPIO_SERVO_PWM_C2_PIN                                     DL_GPIO_PIN_15
#define GPIO_SERVO_PWM_C2_IOMUX                                  (IOMUX_PINCM37)
#define GPIO_SERVO_PWM_C2_IOMUX_FUNC                 IOMUX_PINCM37_PF_TIMA0_CCP2
#define GPIO_SERVO_PWM_C2_IDX                                DL_TIMER_CC_2_INDEX
/* GPIO defines for channel 3 */
#define GPIO_SERVO_PWM_C3_PORT                                             GPIOA
#define GPIO_SERVO_PWM_C3_PIN                                     DL_GPIO_PIN_17
#define GPIO_SERVO_PWM_C3_IOMUX                                  (IOMUX_PINCM39)
#define GPIO_SERVO_PWM_C3_IOMUX_FUNC                 IOMUX_PINCM39_PF_TIMA0_CCP3
#define GPIO_SERVO_PWM_C3_IDX                                DL_TIMER_CC_3_INDEX




/* Defines for ENCODER_R */
#define ENCODER_R_INST                                                     TIMG8
#define ENCODER_R_INST_IRQHandler                               TIMG8_IRQHandler
#define ENCODER_R_INST_INT_IRQN                                 (TIMG8_INT_IRQn)
/* Pin configuration defines for ENCODER_R PHA Pin */
#define GPIO_ENCODER_R_PHA_PORT                                            GPIOA
#define GPIO_ENCODER_R_PHA_PIN                                    DL_GPIO_PIN_26
#define GPIO_ENCODER_R_PHA_IOMUX                                 (IOMUX_PINCM59)
#define GPIO_ENCODER_R_PHA_IOMUX_FUNC                IOMUX_PINCM59_PF_TIMG8_CCP0
/* Pin configuration defines for ENCODER_R PHB Pin */
#define GPIO_ENCODER_R_PHB_PORT                                            GPIOA
#define GPIO_ENCODER_R_PHB_PIN                                    DL_GPIO_PIN_27
#define GPIO_ENCODER_R_PHB_IOMUX                                 (IOMUX_PINCM60)
#define GPIO_ENCODER_R_PHB_IOMUX_FUNC                IOMUX_PINCM60_PF_TIMG8_CCP1



/* Defines for MPU6050 */
#define MPU6050_INST                                                        I2C0
#define MPU6050_INST_IRQHandler                                  I2C0_IRQHandler
#define MPU6050_INST_INT_IRQN                                      I2C0_INT_IRQn
#define MPU6050_BUS_SPEED_HZ                                              400000
#define GPIO_MPU6050_SDA_PORT                                              GPIOA
#define GPIO_MPU6050_SDA_PIN                                       DL_GPIO_PIN_0
#define GPIO_MPU6050_IOMUX_SDA                                    (IOMUX_PINCM1)
#define GPIO_MPU6050_IOMUX_SDA_FUNC                     IOMUX_PINCM1_PF_I2C0_SDA
#define GPIO_MPU6050_SCL_PORT                                              GPIOA
#define GPIO_MPU6050_SCL_PIN                                       DL_GPIO_PIN_1
#define GPIO_MPU6050_IOMUX_SCL                                    (IOMUX_PINCM2)
#define GPIO_MPU6050_IOMUX_SCL_FUNC                     IOMUX_PINCM2_PF_I2C0_SCL


/* Defines for BLUETOOTH */
#define BLUETOOTH_INST                                                     UART1
#define BLUETOOTH_INST_FREQUENCY                                        32000000
#define BLUETOOTH_INST_IRQHandler                               UART1_IRQHandler
#define BLUETOOTH_INST_INT_IRQN                                   UART1_INT_IRQn
#define GPIO_BLUETOOTH_RX_PORT                                             GPIOB
#define GPIO_BLUETOOTH_TX_PORT                                             GPIOB
#define GPIO_BLUETOOTH_RX_PIN                                      DL_GPIO_PIN_7
#define GPIO_BLUETOOTH_TX_PIN                                      DL_GPIO_PIN_6
#define GPIO_BLUETOOTH_IOMUX_RX                                  (IOMUX_PINCM24)
#define GPIO_BLUETOOTH_IOMUX_TX                                  (IOMUX_PINCM23)
#define GPIO_BLUETOOTH_IOMUX_RX_FUNC                   IOMUX_PINCM24_PF_UART1_RX
#define GPIO_BLUETOOTH_IOMUX_TX_FUNC                   IOMUX_PINCM23_PF_UART1_TX
#define BLUETOOTH_BAUD_RATE                                               (9600)
#define BLUETOOTH_IBRD_32_MHZ_9600_BAUD                                    (208)
#define BLUETOOTH_FBRD_32_MHZ_9600_BAUD                                     (21)
/* Defines for K210 */
#define K210_INST                                                          UART2
#define K210_INST_FREQUENCY                                             32000000
#define K210_INST_IRQHandler                                    UART2_IRQHandler
#define K210_INST_INT_IRQN                                        UART2_INT_IRQn
#define GPIO_K210_RX_PORT                                                  GPIOA
#define GPIO_K210_TX_PORT                                                  GPIOA
#define GPIO_K210_RX_PIN                                          DL_GPIO_PIN_24
#define GPIO_K210_TX_PIN                                          DL_GPIO_PIN_23
#define GPIO_K210_IOMUX_RX                                       (IOMUX_PINCM54)
#define GPIO_K210_IOMUX_TX                                       (IOMUX_PINCM53)
#define GPIO_K210_IOMUX_RX_FUNC                        IOMUX_PINCM54_PF_UART2_RX
#define GPIO_K210_IOMUX_TX_FUNC                        IOMUX_PINCM53_PF_UART2_TX
#define K210_BAUD_RATE                                                  (115200)
#define K210_IBRD_32_MHZ_115200_BAUD                                        (17)
#define K210_FBRD_32_MHZ_115200_BAUD                                        (23)





/* Defines for AIN1: GPIOB.17 with pinCMx 43 on package pin 14 */
#define MOTOR_DIR_AIN1_PORT                                              (GPIOB)
#define MOTOR_DIR_AIN1_PIN                                      (DL_GPIO_PIN_17)
#define MOTOR_DIR_AIN1_IOMUX                                     (IOMUX_PINCM43)
/* Defines for AIN2: GPIOB.19 with pinCMx 45 on package pin 16 */
#define MOTOR_DIR_AIN2_PORT                                              (GPIOB)
#define MOTOR_DIR_AIN2_PIN                                      (DL_GPIO_PIN_19)
#define MOTOR_DIR_AIN2_IOMUX                                     (IOMUX_PINCM45)
/* Defines for BIN1: GPIOA.16 with pinCMx 38 on package pin 9 */
#define MOTOR_DIR_BIN1_PORT                                              (GPIOA)
#define MOTOR_DIR_BIN1_PIN                                      (DL_GPIO_PIN_16)
#define MOTOR_DIR_BIN1_IOMUX                                     (IOMUX_PINCM38)
/* Defines for BIN2: GPIOB.24 with pinCMx 52 on package pin 23 */
#define MOTOR_DIR_BIN2_PORT                                              (GPIOB)
#define MOTOR_DIR_BIN2_PIN                                      (DL_GPIO_PIN_24)
#define MOTOR_DIR_BIN2_IOMUX                                     (IOMUX_PINCM52)
/* Port definition for Pin Group OLED */
#define OLED_PORT                                                        (GPIOA)

/* Defines for SCL: GPIOA.31 with pinCMx 6 on package pin 39 */
#define OLED_SCL_PIN                                            (DL_GPIO_PIN_31)
#define OLED_SCL_IOMUX                                            (IOMUX_PINCM6)
/* Defines for SDA: GPIOA.28 with pinCMx 3 on package pin 35 */
#define OLED_SDA_PIN                                            (DL_GPIO_PIN_28)
#define OLED_SDA_IOMUX                                            (IOMUX_PINCM3)
/* Defines for T1: GPIOB.5 with pinCMx 18 on package pin 53 */
#define TRACK_SENSOR_T1_PORT                                             (GPIOB)
#define TRACK_SENSOR_T1_PIN                                      (DL_GPIO_PIN_5)
#define TRACK_SENSOR_T1_IOMUX                                    (IOMUX_PINCM18)
/* Defines for T2: GPIOB.15 with pinCMx 32 on package pin 3 */
#define TRACK_SENSOR_T2_PORT                                             (GPIOB)
#define TRACK_SENSOR_T2_PIN                                     (DL_GPIO_PIN_15)
#define TRACK_SENSOR_T2_IOMUX                                    (IOMUX_PINCM32)
/* Defines for T3: GPIOA.10 with pinCMx 21 on package pin 56 */
#define TRACK_SENSOR_T3_PORT                                             (GPIOA)
#define TRACK_SENSOR_T3_PIN                                     (DL_GPIO_PIN_10)
#define TRACK_SENSOR_T3_IOMUX                                    (IOMUX_PINCM21)
/* Defines for T4: GPIOB.16 with pinCMx 33 on package pin 4 */
#define TRACK_SENSOR_T4_PORT                                             (GPIOB)
#define TRACK_SENSOR_T4_PIN                                     (DL_GPIO_PIN_16)
#define TRACK_SENSOR_T4_IOMUX                                    (IOMUX_PINCM33)
/* Defines for T5: GPIOA.11 with pinCMx 22 on package pin 57 */
#define TRACK_SENSOR_T5_PORT                                             (GPIOA)
#define TRACK_SENSOR_T5_PIN                                     (DL_GPIO_PIN_11)
#define TRACK_SENSOR_T5_IOMUX                                    (IOMUX_PINCM22)
/* Port definition for Pin Group MPU_INT */
#define MPU_INT_PORT                                                     (GPIOB)

/* Defines for SIGNAL: GPIOB.4 with pinCMx 17 on package pin 52 */
// groups represented: ["MPU_INT"]
// pins affected: ["SIGNAL"]
#define GPIO_MULTIPLE_GPIOB_INT_IRQN                            (GPIOB_INT_IRQn)
#define GPIO_MULTIPLE_GPIOB_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define MPU_INT_SIGNAL_IIDX                                  (DL_GPIO_IIDX_DIO4)
#define MPU_INT_SIGNAL_PIN                                       (DL_GPIO_PIN_4)
#define MPU_INT_SIGNAL_IOMUX                                     (IOMUX_PINCM17)
/* Port definition for Pin Group USER_BUTTON */
#define USER_BUTTON_PORT                                                 (GPIOB)

/* Defines for KEY: GPIOB.21 with pinCMx 49 on package pin 20 */
#define USER_BUTTON_KEY_PIN                                     (DL_GPIO_PIN_21)
#define USER_BUTTON_KEY_IOMUX                                    (IOMUX_PINCM49)
/* Port definition for Pin Group ULTRASONIC */
#define ULTRASONIC_PORT                                                  (GPIOB)

/* Defines for ECHO: GPIOB.22 with pinCMx 50 on package pin 21 */
#define ULTRASONIC_ECHO_PIN                                     (DL_GPIO_PIN_22)
#define ULTRASONIC_ECHO_IOMUX                                    (IOMUX_PINCM50)
/* Port definition for Pin Group ENCODER_L */
#define ENCODER_L_PORT                                                   (GPIOA)

/* Defines for LEFT_A: GPIOA.25 with pinCMx 55 on package pin 26 */
// pins affected by this interrupt request:["LEFT_A","LEFT_B"]
#define ENCODER_L_INT_IRQN                                      (GPIOA_INT_IRQn)
#define ENCODER_L_INT_IIDX                      (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENCODER_L_LEFT_A_IIDX                               (DL_GPIO_IIDX_DIO25)
#define ENCODER_L_LEFT_A_PIN                                    (DL_GPIO_PIN_25)
#define ENCODER_L_LEFT_A_IOMUX                                   (IOMUX_PINCM55)
/* Defines for LEFT_B: GPIOA.14 with pinCMx 36 on package pin 7 */
#define ENCODER_L_LEFT_B_IIDX                               (DL_GPIO_IIDX_DIO14)
#define ENCODER_L_LEFT_B_PIN                                    (DL_GPIO_PIN_14)
#define ENCODER_L_LEFT_B_IOMUX                                   (IOMUX_PINCM36)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_MOTOR_PWM_init(void);
void SYSCFG_DL_BUZZER_PWM_init(void);
void SYSCFG_DL_SERVO_PWM_init(void);
void SYSCFG_DL_ENCODER_R_init(void);
void SYSCFG_DL_MPU6050_init(void);
void SYSCFG_DL_BLUETOOTH_init(void);
void SYSCFG_DL_K210_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */


