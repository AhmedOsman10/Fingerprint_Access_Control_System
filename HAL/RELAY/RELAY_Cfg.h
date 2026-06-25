/******************************************************************************
 * @file    RELAY_Cfg.h
 * @author  Ahmed Abdelrhman
 * @brief   Relay driver configuration file.
 *
 * @details
 * This file contains the relay count, relay GPIO configuration structure,
 * helper pin/port macros, and the relay configuration table used by RELAY.c.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef RELAY_RELAY_CFG_H_
#define RELAY_RELAY_CFG_H_

#include "stm32f4xx_hal.h"

/* Total number of relay channels configured in this project. */
#define RELAY_MAX_NUM    1

/******************************************************************************
 * @brief Relay GPIO configuration structure.
 *
 * @details
 * Each relay channel has:
 * - Port_Num: GPIO port used to drive the relay.
 * - Pin_Num : GPIO pin connected to the relay driver circuit.
 ******************************************************************************/
typedef struct RELAY_Config_S
{
    /* GPIO port connected to the relay input/driver. */
    GPIO_TypeDef *Port_Num;

    /* GPIO pin connected to the relay input/driver. */
    uint32_t Pin_Num;

} RELAY_Config_t;

/******************************************************************************
 * GPIO pin helper macros.
 *
 * These aliases map directly to STM32 HAL GPIO pin definitions. They make the
 * relay configuration table easier to read and keep the driver style consistent
 * with other project drivers.
 ******************************************************************************/
#define RELAY_PIN_0     GPIO_PIN_0
#define RELAY_PIN_1     GPIO_PIN_1
#define RELAY_PIN_2     GPIO_PIN_2
#define RELAY_PIN_3     GPIO_PIN_3
#define RELAY_PIN_4     GPIO_PIN_4
#define RELAY_PIN_5     GPIO_PIN_5
#define RELAY_PIN_6     GPIO_PIN_6
#define RELAY_PIN_7     GPIO_PIN_7
#define RELAY_PIN_8     GPIO_PIN_8
#define RELAY_PIN_9     GPIO_PIN_9
#define RELAY_PIN_10    GPIO_PIN_10
#define RELAY_PIN_11    GPIO_PIN_11
#define RELAY_PIN_12    GPIO_PIN_12
#define RELAY_PIN_13    GPIO_PIN_13
#define RELAY_PIN_14    GPIO_PIN_14
#define RELAY_PIN_15    GPIO_PIN_15

/******************************************************************************
 * GPIO port helper macros.
 *
 * These aliases map directly to STM32 GPIO port base addresses.
 ******************************************************************************/
#define RELAY_PORT_A    GPIOA
#define RELAY_PORT_B    GPIOB
#define RELAY_PORT_C    GPIOC
#define RELAY_PORT_D    GPIOD
#define RELAY_PORT_E    GPIOE
#define RELAY_PORT_F    GPIOF
#define RELAY_PORT_G    GPIOG
#define RELAY_PORT_H    GPIOH

/******************************************************************************
 * Relay configuration table.
 *
 * RELAY_NUM_1 is configured on:
 * - GPIO port: GPIOA
 * - GPIO pin : PA9
 *
 * The application accesses this relay using RELAY_NUM_1.
 ******************************************************************************/
const RELAY_Config_t RELAY_Config[RELAY_MAX_NUM] = {
    {
        .Port_Num = RELAY_PORT_A,
        .Pin_Num  = RELAY_PIN_9
    }
};

#endif /* RELAY_RELAY_CFG_H_ */
