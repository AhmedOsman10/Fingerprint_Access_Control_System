/******************************************************************************
 * @file    I2C_Cfg.c
 * @author  Ahmed Abdelrhman
 * @brief   I2C peripheral configuration table.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This file stores the GPIO and peripheral mapping for the available I2C
 * instances. The table keeps hardware configuration separated from driver logic.
 *
 * Code behavior is unchanged; only comments and formatting were cleaned.
 ******************************************************************************/

#include "I2C_Cfg.h"

/******************************************************************************
 * I2C Configuration Table
 *
 * Each table entry describes:
 *  - I2C peripheral instance
 *  - SCL GPIO port, pin, and alternate function
 *  - SDA GPIO port, pin, and alternate function
 *  - Bus clock speed
 *
 * Current project usage:
 *  - I2C3 is the active bus used by the RTC/EEPROM hardware.
 ******************************************************************************/
const I2C_Config_t I2C_Config[I2C_MAX_NUM] =
{
    /**************************************************************************
     * I2C1 Configuration
     *
     * Status:
     *  - Not currently used in this project.
     *
     * Default pin mapping:
     *  - SCL : PB6
     *  - SDA : PB7
     *************************************************************************/
    {
        .Instance   = I2C1,
        .SCL_Port   = GPIOB,
        .SCL_Pin    = GPIO_PIN_6,
        .SCL_AF     = GPIO_AF4_I2C1,
        .SDA_Port   = GPIOB,
        .SDA_Pin    = GPIO_PIN_7,
        .SDA_AF     = GPIO_AF4_I2C1,
        .ClockSpeed = 100000
    },

    /**************************************************************************
     * I2C2 Configuration
     *
     * Status:
     *  - Not currently used in this project.
     *
     * Default pin mapping:
     *  - SCL : PB10
     *  - SDA : PB11
     *************************************************************************/
    {
        .Instance   = I2C2,
        .SCL_Port   = GPIOB,
        .SCL_Pin    = GPIO_PIN_10,
        .SCL_AF     = GPIO_AF4_I2C2,
        .SDA_Port   = GPIOB,
        .SDA_Pin    = GPIO_PIN_11,
        .SDA_AF     = GPIO_AF4_I2C2,
        .ClockSpeed = 100000
    },

    /**************************************************************************
     * I2C3 Configuration
     *
     * Status:
     *  - Active bus used by the project.
     *
     * Hardware mapping:
     *  - SCL : PA8
     *  - SDA : PC9
     *
     * Connected devices:
     *  - External RTC module
     *  - EEPROM module
     *
     * Bus speed:
     *  - 100 kHz standard mode
     *************************************************************************/
    {
        .Instance   = I2C3,
        .SCL_Port   = GPIOA,
        .SCL_Pin    = GPIO_PIN_8,
        .SCL_AF     = GPIO_AF4_I2C3,
        .SDA_Port   = GPIOC,
        .SDA_Pin    = GPIO_PIN_9,
        .SDA_AF     = GPIO_AF4_I2C3,
        .ClockSpeed = 100000
    }
};
