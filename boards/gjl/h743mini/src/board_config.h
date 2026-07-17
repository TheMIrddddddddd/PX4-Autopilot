/****************************************************************************
 *
 *   Copyright (c) 2022 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file board_config.h
 *
 * PX4FMU-v6c internal definitions
 */

#pragma once

/****************************************************************************************************
 * Included Files
 ****************************************************************************************************/

#include <px4_platform_common/px4_config.h>
#include <nuttx/compiler.h>
#include <stdint.h>


#include <stm32_gpio.h>

/****************************************************************************************************
 * Definitions
 ****************************************************************************************************/

#undef TRACE_PINS

/* Legacy PX4IO serial configuration.
 *
 * CONFIG_DRIVERS_PX4IO is disabled on H743mini. Keep these definitions for the
 * inherited board code, but USART6 PC6/PC7 is reserved for later RCIN/CRSF
 * planning rather than an active PX4IO link.
 */

//#define BOARD_USES_PX4IO_VERSION       2
#define PX4IO_SERIAL_DEVICE            "/dev/ttyS4"
#define PX4IO_SERIAL_TX_GPIO           GPIO_USART6_TX
#define PX4IO_SERIAL_RX_GPIO           GPIO_USART6_RX
#define PX4IO_SERIAL_BASE              STM32_USART6_BASE
#define PX4IO_SERIAL_VECTOR            STM32_IRQ_USART6
#define PX4IO_SERIAL_TX_DMAMAP         DMAMAP_USART6_TX
#define PX4IO_SERIAL_RX_DMAMAP         DMAMAP_USART6_RX
#define PX4IO_SERIAL_RCC_REG           STM32_RCC_APB2ENR
#define PX4IO_SERIAL_RCC_EN            RCC_APB2ENR_USART6EN
#define PX4IO_SERIAL_CLOCK             STM32_PCLK2_FREQUENCY
#define PX4IO_SERIAL_BITRATE           1500000               /* 1.5Mbps -> max rate for IO */

/* PX4FMU GPIOs ***********************************************************************************/

/* USART6 PC6/PC7 is used for ELRS/CRSF receiver input. */
#define RC_SERIAL_PORT                         "/dev/ttyS5"
#define BOARD_SUPPORTS_RC_SERIAL_PORT_OUTPUT

/* LEDs are driven with push pull Anodes to 3.3V */

#define GPIO_nLED_RED        /* PI11 */  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTI|GPIO_PIN11)
#define GPIO_nLED_BLUE       /* PI8  */  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTI|GPIO_PIN8)
#define GPIO_nLED_GREEN      /* PC13  */  (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN13)

#define BOARD_HAS_CONTROL_STATUS_LEDS      1
#define BOARD_OVERLOAD_LED     LED_RED
#define BOARD_ARMED_STATE_LED  LED_BLUE
#define BOARD_ARMED_LED        LED_GREEN

/*
 * ADC channels
 *
 * These are the channel numbers of the ADCs of the microcontroller that
 * can be used by the Px4 Firmware in the adc driver
 */

/* ADC defines to be used in sensors.cpp to read from a particular channel */

#define ADC1_CH(n)                  (n)

/* N.B. there is no offset mapping needed for ADC3 because */
#define ADC3_CH(n)                  (n)

/* We are only use ADC3 for REV/VER. */

/* Define GPIO pins used as ADC N.B. Channel numbers must match below  */

#define PX4_ADC_GPIO  \
	/* PC4  */  GPIO_ADC12_INP4,   \
	/* PC5  */  GPIO_ADC12_INP8,   \
	/* PC0  */  GPIO_ADC123_INP10, \
	/* PC1  */  GPIO_ADC123_INP11, \
	/* PA4  */  GPIO_ADC12_INP18   \

/* Define Channel numbers must match above GPIO pin IN(n)*/
#define ADC_BATTERY1_CURRENT_CHANNEL            /* PC4 */  ADC1_CH(4)
//#define ADC_BATTERY2_VOLTAGE_CHANNEL            /* PB1 */  ADC1_CH(5)
#define ADC_BATTERY1_VOLTAGE_CHANNEL            /* PC5 */  ADC1_CH(8)
#define ADC_HW_REV_SENSE_CHANNEL                /* PC0 */  ADC3_CH(10)
#define ADC_HW_VER_SENSE_CHANNEL                /* PC1 */  ADC3_CH(11)
/* PA2 is reserved for USART2_TX / GPS. Do not initialize it as ADC. */
//#define ADC_BATTERY2_CURRENT_CHANNEL            /* PA2 */  ADC1_CH(14)
#define ADC_SCALED_V5_CHANNEL                   /* PA4 */  ADC1_CH(18)

#define ADC_CHANNELS \
	((1 << ADC_BATTERY1_CURRENT_CHANNEL) | \
	 (1 << ADC_SCALED_V5_CHANNEL       ))

#define HW_REV_VER_ADC_BASE STM32_ADC3_BASE

#define SYSTEM_ADC_BASE STM32_ADC1_BASE

/* HW has to large of R termination on ADC todo:change when HW value is chosen */
#define BOARD_ADC_OPEN_CIRCUIT_V     (5.6f)

/* HW Version and Revision drive signals Default to 1 to detect */
#define BOARD_HAS_HW_VERSIONING

#define GPIO_HW_VER_REV_DRIVE  /* PE12 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTE|GPIO_PIN12)
#define GPIO_HW_REV_SENSE      /* PC0 */  GPIO_ADC123_INP10
#define GPIO_HW_VER_SENSE      /* PC1 */  GPIO_ADC123_INP11
#define HW_INFO_INIT           {'V','6','C','x', 'x',0}
#define HW_INFO_INIT_VER       3 /* Offset in above string of the VER */
#define HW_INFO_INIT_REV       4 /* Offset in above string of the REV */

#define BOARD_NUM_SPI_CFG_HW_VERSIONS 3 // Rev 0, 10 and Mini Sensor sets
//                 Base/FMUM
#define V6C00   HW_VER_REV(0x0,0x0) // FMUV6C,                 Rev 0  I2C4 External but with Internal devices
#define V6C01   HW_VER_REV(0x0,0x1) // FMUV6C,                 Rev 1  I2C4 Internal I2C2 External
#define V6C10   HW_VER_REV(0x1,0x0) // NO PX4IO,               Rev 0  I2C4 External but with Internal devices
#define V6C11   HW_VER_REV(0x1,0x1) // NO PX4IO,               Rev 1  I2C4 Internal I2C2 External
#define V6C21   HW_VER_REV(0x2,0x1) // FMUV6CMini,             Rev 1  I2C4 Internal I2C2 External


/* HEATER
 * PWM in future
 */
#define GPIO_HEATER_OUTPUT   /* PB9  T17CH1 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN9)
#define HEATER_OUTPUT_EN(on_true)	       px4_arch_gpiowrite(GPIO_HEATER_OUTPUT, (on_true))

/* PWM
 */
#define DIRECT_PWM_OUTPUT_CHANNELS   8


/* Power supply control and monitoring GPIOs
 *
 * H743mini does not carry over the full FMUv6C power-switch circuit. Keep
 * legacy definitions documented here, but do not initialize or drive pins that
 * are already assigned to real H743mini peripherals:
 *
 * - PA15: legacy Brick1 valid input, reserved for TIM2_CH1 / M1 PWM.
 * - PC10/PC11: legacy high-power 5V enable/OC, used by SDMMC1 D2/D3.
 * - PB2: legacy sensors 3V3 enable, used by GD25Q128 QSPI_CLK.
 */

#define GPIO_nPOWER_IN_A                /* PA15  */ (GPIO_INPUT|GPIO_PULLUP|GPIO_PORTA|GPIO_PIN15)
#define GPIO_nPOWER_IN_C                /* PE15  */ (GPIO_INPUT|GPIO_PULLUP|GPIO_PORTE|GPIO_PIN15)

#define GPIO_nVDD_BRICK1_VALID          GPIO_nPOWER_IN_A /* Brick 1 Is Chosen */
#define BOARD_NUMBER_BRICKS             2
#define GPIO_nVDD_USB_VALID             GPIO_nPOWER_IN_C /* USB     Is Chosen */

#define GPIO_VDD_5V_PERIPH_nEN          /* PE2  */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTE|GPIO_PIN2)
#define GPIO_VDD_5V_PERIPH_nOC          /* PE3  */ (GPIO_INPUT |GPIO_FLOAT|GPIO_PORTE|GPIO_PIN3)
#define GPIO_VDD_5V_HIPOWER_nEN         /* PC10 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_SET|GPIO_PORTC|GPIO_PIN10)
#define GPIO_VDD_5V_HIPOWER_nOC         /* PC11 */ (GPIO_INPUT |GPIO_FLOAT|GPIO_PORTC|GPIO_PIN11)
#define GPIO_VDD_3V3_SENSORS_EN         /* PB2  */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN2)

/* Define True logic Power Control in arch agnostic form */

#define VDD_5V_PERIPH_EN(on_true)          px4_arch_gpiowrite(GPIO_VDD_5V_PERIPH_nEN, !(on_true))
#define VDD_5V_HIPOWER_EN(on_true)         do { (void)(on_true); } while (0)
#define VDD_3V3_SENSORS_EN(on_true)        do { (void)(on_true); } while (0)

/* Tone alarm output */

#define TONE_ALARM_TIMER        12  /* Timer 12 */
#define TONE_ALARM_CHANNEL      1   /* PB14 GPIO_TIM12_CH1OUT_1 */

#define GPIO_BUZZER_1           /* PB14 */ (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_2MHz|GPIO_OUTPUT_CLEAR|GPIO_PORTB|GPIO_PIN14)

#define GPIO_TONE_ALARM_IDLE    GPIO_BUZZER_1
#define GPIO_TONE_ALARM         GPIO_TIM12_CH1OUT_1

/* USB OTG FS
 *
 * PA11 OTG_FS_DM
 * PA12 OTG_FS_DP
 *
 * h743mini does not route USB VBUS to PA9. PA9 is USART1_TX on this board.
 */
#define BOARD_USB_VBUS_SENSE_DISABLED  1
#define GPIO_OTGFS_VBUS         /* PA9 */ (GPIO_INPUT|GPIO_PULLDOWN|GPIO_SPEED_100MHz|GPIO_PORTA|GPIO_PIN9)

/* High-resolution timer */
#define HRT_TIMER               8  /* use timer8 for the HRT */
#define HRT_TIMER_CHANNEL       3  /* use capture/compare channel 3 */

/* PWM input driver. Use FMU AUX5 pins attached to timer4 channel 3 */
#define PWMIN_TIMER                       4
#define PWMIN_TIMER_CHANNEL    /* T4C3 */ 3
#define GPIO_PWM_IN            /* PD14 */ GPIO_TIM4_CH3IN_2

#define SDIO_SLOTNO                    0  /* Only one slot */
#define SDIO_MINOR                     0

/* SD card bringup does not work if performed on the IDLE thread because it
 * will cause waiting.  Use either:
 *
 *  CONFIG_LIB_BOARDCTL=y, OR
 *  CONFIG_BOARD_INITIALIZE=y && CONFIG_BOARD_INITTHREAD=y
 */

#if defined(CONFIG_BOARD_INITIALIZE) && !defined(CONFIG_LIB_BOARDCTL) && \
   !defined(CONFIG_BOARD_INITTHREAD)
#  warning SDIO initialization cannot be perfomed on the IDLE thread
#endif

/* No USB VBUS sense GPIO is available on h743mini. */

/* FMUv6C never powers off the Servo rail */

#define BOARD_ADC_SERVO_VALID     (1)

#define BOARD_ADC_BRICK1_VALID  (1)
#define BOARD_ADC_BRICK2_VALID  (1)
#define BOARD_ADC_PERIPH_5V_OC  (!px4_arch_gpioread(GPIO_VDD_5V_PERIPH_nOC))
#define BOARD_ADC_HIPOWER_5V_OC (0)


/* This board provides a DMA pool and APIs */
#define BOARD_DMA_ALLOC_POOL_SIZE 16384

/* This board provides the board_on_reset interface */

#define BOARD_HAS_ON_RESET 1

/* Do not initialize these FMUv6C legacy / unavailable pins on H743mini:
 * - GPIO_nPOWER_IN_A: PA15 is reserved for TIM2_CH1 / M1 PWM.
 * - GPIO_VDD_5V_HIPOWER_nEN/nOC: PC10/PC11 are SDMMC1 D2/D3.
 * - GPIO_VDD_3V3_SENSORS_EN: PB2 is GD25Q128 QSPI_CLK.
 * - GPIO_CAN1_TX/RX and GPIO_CAN2_TX/RX: CAN transceiver routing is not confirmed.
 *
 * Screened CAN entries from PX4_GPIO_INIT_LIST:
 * - GPIO_CAN1_TX
 * - GPIO_CAN1_RX
 * - GPIO_CAN2_TX
 * - GPIO_CAN2_RX
 */
#define PX4_GPIO_INIT_LIST { \
		PX4_ADC_GPIO,                     \
		GPIO_HW_VER_REV_DRIVE,            \
		GPIO_HEATER_OUTPUT,               \
		GPIO_nPOWER_IN_C,                 \
		GPIO_VDD_5V_PERIPH_nEN,           \
		GPIO_VDD_5V_PERIPH_nOC,           \
	}

#define BOARD_ENABLE_CONSOLE_BUFFER

#define PX4_I2C_BUS_MTD      2

#define BOARD_OVERRIDE_I2C_DEVICE_EXTERNAL


#define BOARD_NUM_IO_TIMERS 5

__BEGIN_DECLS

/****************************************************************************************************
 * Public Types
 ****************************************************************************************************/

/****************************************************************************************************
 * Public data
 ****************************************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************************************
 * Public Functions
 ****************************************************************************************************/

/****************************************************************************
 * Name: stm32_sdio_initialize
 *
 * Description:
 *   Initialize SDIO-based MMC/SD card support
 *
 ****************************************************************************/

int stm32_sdio_initialize(void);

/****************************************************************************************************
 * Name: stm32_spiinitialize
 *
 * Description:
 *   Called to configure SPI chip select GPIO pins for the PX4FMU board.
 *
 ****************************************************************************************************/

extern void stm32_spiinitialize(void);

extern void stm32_usbinitialize(void);

extern void board_peripheral_reset(int ms);

#include <px4_platform_common/board_common.h>

#endif /* __ASSEMBLY__ */

__END_DECLS
