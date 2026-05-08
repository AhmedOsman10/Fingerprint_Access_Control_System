/*
 * RELAY_Cfg.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 *
 *  Configuration file for the Relay driver.
 *
 *  This file contains hardware-specific relay configuration.
 *  The relay driver implementation should use this file instead of hardcoded
 *  GPIO ports and pins.
 */

#ifndef RELAY_RELAY_CFG_H_
#define RELAY_RELAY_CFG_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

/******************************************************************************************
 *                                  RELAY_MAX_NUM
 *
 *  Maximum number of relays configured in the driver.
 *
 *  RELAY_MAX_NUM: Number of relay entries available in RELAY_Config[].
 ******************************************************************************************/
#define RELAY_MAX_NUM   1


/******************************************************************************************
 *                                  RELAY_Config_t
 *
 *  Relay GPIO configuration structure.
 *
 *  Fields:
 *    Port_Num: GPIO port connected to the relay control pin.
 *    Pin_Num:  GPIO pin connected to the relay control input.
 ******************************************************************************************/
typedef struct RELAY_Config_S
{
	GPIO_TypeDef *Port_Num;
	uint32_t      Pin_Num;
}RELAY_Config_t;


/******************************************************************************************
 *                              GPIO Pin Helper Macros
 *
 *  These aliases map directly to STM32 HAL GPIO pin definitions.
 *
 *  Application / Configuration Use:
 *    Use RELAY_PIN_x macros inside RELAY_Config[] instead of raw numbers.
 ******************************************************************************************/
#define RELAY_PIN_0   GPIO_PIN_0
#define RELAY_PIN_1   GPIO_PIN_1
#define RELAY_PIN_2   GPIO_PIN_2
#define RELAY_PIN_3   GPIO_PIN_3
#define RELAY_PIN_4   GPIO_PIN_4
#define RELAY_PIN_5   GPIO_PIN_5
#define RELAY_PIN_6   GPIO_PIN_6
#define RELAY_PIN_7   GPIO_PIN_7
#define RELAY_PIN_8   GPIO_PIN_8
#define RELAY_PIN_9   GPIO_PIN_9
#define RELAY_PIN_10  GPIO_PIN_10
#define RELAY_PIN_11  GPIO_PIN_11
#define RELAY_PIN_12  GPIO_PIN_12
#define RELAY_PIN_13  GPIO_PIN_13
#define RELAY_PIN_14  GPIO_PIN_14
#define RELAY_PIN_15  GPIO_PIN_15


/******************************************************************************************
 *                              GPIO Port Helper Macros
 *
 *  These aliases map directly to STM32 GPIO port base addresses.
 *
 *  Configuration Use:
 *    Use RELAY_PORT_x macros inside RELAY_Config[] for readability.
 ******************************************************************************************/
#define RELAY_PORT_A	GPIOA
#define RELAY_PORT_B	GPIOB
#define RELAY_PORT_C	GPIOC
#define RELAY_PORT_D	GPIOD
#define RELAY_PORT_E	GPIOE
#define RELAY_PORT_F	GPIOF
#define RELAY_PORT_G	GPIOG
#define RELAY_PORT_H	GPIOH


/******************************************************************************************
 *                                  RELAY_Config[]
 *
 *  Relay hardware configuration table.
 *
 *  Each entry maps one logical relay number to one GPIO port/pin pair.
 *
 *  Current Configuration:
 *    RELAY_NUM_1:
 *      Port: RELAY_PORT_A
 *      Pin : RELAY_PIN_9
 ******************************************************************************************/
const RELAY_Config_t  RELAY_Config[RELAY_MAX_NUM] = {{.Port_Num = RELAY_PORT_A, .Pin_Num  = RELAY_PIN_9}, };


#endif /* RELAY_RELAY_CFG_H_ */
