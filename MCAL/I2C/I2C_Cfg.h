/******************************************************************************
 * @file    I2C_Cfg.h
 * @author  Ahmed Abdelrhman
 * @brief   Configuration declarations for the MCAL I2C driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This file defines the configuration structure and instance indexes used by
 * the I2C driver configuration table.
 *
 * Code behavior is unchanged; only comments and formatting were cleaned.
 ******************************************************************************/

#ifndef MCAL_I2C_I2C_CFG_H_
#define MCAL_I2C_I2C_CFG_H_

#include "stm32f4xx_hal.h"

/******************************************************************************
 * Number of supported I2C instances in the configuration table.
 ******************************************************************************/
#define I2C_MAX_NUM     3

/******************************************************************************
 * I2C instance indexes.
 *
 * These indexes are used to access the I2C_Config[] table.
 ******************************************************************************/
#define I2C_NUM_1       0
#define I2C_NUM_2       1
#define I2C_NUM_3       2

/******************************************************************************
 * @brief I2C hardware configuration structure.
 *
 * This structure stores the peripheral instance, GPIO mapping, alternate
 * function selection, and bus speed for one I2C peripheral.
 ******************************************************************************/
typedef struct
{
    I2C_TypeDef *Instance;

    GPIO_TypeDef *SCL_Port;
    uint16_t      SCL_Pin;
    uint32_t      SCL_AF;

    GPIO_TypeDef *SDA_Port;
    uint16_t      SDA_Pin;
    uint32_t      SDA_AF;

    uint32_t ClockSpeed;

} I2C_Config_t;

/******************************************************************************
 * Global I2C configuration table.
 ******************************************************************************/
extern const I2C_Config_t I2C_Config[I2C_MAX_NUM];

#endif /* MCAL_I2C_I2C_CFG_H_ */
