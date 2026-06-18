/******************************************************************************
 * @file    I2C_Prv.h
 * @author  Ahmed Abdelrhman
 * @brief   Private declarations for the MCAL I2C driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This file is intended for driver-internal macros and private function
 * prototypes. It is not meant to be included by application-level modules.
 *
 * Code behavior is unchanged; only comments and formatting were cleaned.
 ******************************************************************************/

#ifndef MCAL_I2C_I2C_PRV_H_
#define MCAL_I2C_I2C_PRV_H_

#include "I2C_Cfg.h"

/******************************************************************************
 * Default I2C operation settings.
 ******************************************************************************/
#define I2C_TIMEOUT                 3000
#define I2C_READY_TRIALS            3

/******************************************************************************
 * Memory address size helper.
 *
 * Used when communicating with devices that use 8-bit internal register or
 * memory addresses.
 ******************************************************************************/
#define I2C_MEM_ADDR_SIZE_8BIT      I2C_MEMADD_SIZE_8BIT

/******************************************************************************
 * Private function prototypes.
 *
 * Static driver helper functions should be declared here when used by I2C.c.
 ******************************************************************************/
static void I2C_EnableClock(uint8_t I2C_Num);
static void I2C_GPIO_Init(uint8_t I2C_Num);

#endif /* MCAL_I2C_I2C_PRV_H_ */
