/*
 * RELAY_Cfg.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 */

#ifndef RELAY_RELAY_CFG_H_
#define RELAY_RELAY_CFG_H_


#define RELAY_MAX_NUM   1


typedef struct RELAY_Config_S{
	GPIO_TypeDef *Port_Num;
	uint32_t      Pin_Num;
}RELAY_Config_t;

/* =========================================================================================
 *                                GPIO Pin Helper Macros
 * =========================================================================================
 *
 * These aliases allow configuration tables to be written in a consistent style:
 *   .Tx_Pin = USART_PIN_2, .Tx_Port = USART_PORT_A, etc.
 *
 * They map 1:1 to HAL GPIO pin defines (GPIO_PIN_x).
 */
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

/* =========================================================================================
 *                                GPIO Port Helper Macros
 * =========================================================================================
 *
 * These aliases map to STM32 GPIO port base addresses (GPIOA..GPIOH).
 */
#define RELAY_PORT_A	GPIOA
#define RELAY_PORT_B	GPIOB
#define RELAY_PORT_C	GPIOC
#define RELAY_PORT_D	GPIOD
#define RELAY_PORT_E	GPIOE
#define RELAY_PORT_F	GPIOF
#define RELAY_PORT_G	GPIOG
#define RELAY_PORT_H	GPIOH

const RELAY_Config_t  RELAY_Config[RELAY_MAX_NUM] = {{.Port_Num = RELAY_PORT_A, .Pin_Num = RELAY_PIN_9}, };


#endif /* RELAY_RELAY_CFG_H_ */
