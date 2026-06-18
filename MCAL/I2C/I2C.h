/******************************************************************************
 * @file    I2C.h
 * @author  Ahmed Abdelrhman
 * @brief   Public interface for the MCAL I2C driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This file exposes the I2C APIs used by higher-level modules. The driver owns
 * the I2C3 HAL handle and provides simple wrappers for common I2C operations.
 *
 * Code behavior is unchanged; only comments and formatting were cleaned.
 ******************************************************************************/

#ifndef MCAL_I2C_I2C_H_
#define MCAL_I2C_I2C_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/******************************************************************************
 * External I2C Handle
 *
 * hi2c3 is defined in I2C.c. It is kept extern here because STM32 HAL callbacks
 * and some project modules may need access to the same configured handle.
 ******************************************************************************/
extern I2C_HandleTypeDef hi2c3;

/******************************************************************************
 * @brief  Initialize the I2C3 peripheral and its GPIO pins.
 ******************************************************************************/
void I2C_Init(void);

/******************************************************************************
 * @brief  Check whether a slave device acknowledges on the I2C bus.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  Trials      Number of acknowledge polling attempts.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_IsDeviceReady(uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);

/******************************************************************************
 * @brief  Write data to an internal register/memory address of an I2C device.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  MemAddress  Internal memory/register address inside the slave device.
 * @param  MemAddSize  Memory address size.
 * @param  pData       Pointer to the transmit buffer.
 * @param  Size        Number of bytes to write.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_Mem_Write(uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

/******************************************************************************
 * @brief  Read data from an internal register/memory address of an I2C device.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  MemAddress  Internal memory/register address inside the slave device.
 * @param  MemAddSize  Memory address size.
 * @param  pData       Pointer to the receive buffer.
 * @param  Size        Number of bytes to read.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_Mem_Read(uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout);

#endif /* MCAL_I2C_I2C_H_ */
