/*
 * I2C.h
 *
 *  Created on: 17 Jun 2026
 *      Author: Ahmed
 *
 *  MCAL I2C driver interface.
 *
 *  This driver owns the STM32 I2C peripheral initialization and exposes
 *  simple wrapper APIs for higher-level drivers such as EEPROM and RTC.
 */

#ifndef MCAL_I2C_I2C_H_
#define MCAL_I2C_I2C_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_i2c.h"

/* Public I2C handle.
 *
 * Higher layers should not initialize this handle directly.
 * They may use the MCAL I2C APIs below instead.
 */
extern I2C_HandleTypeDef hi2c3;

void I2C_Init(void);
HAL_StatusTypeDef I2C_IsDeviceReady(uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);
HAL_StatusTypeDef I2C_Mem_Write(uint16_t DevAddress,
                                uint16_t MemAddress,
                                uint16_t MemAddSize,
                                uint8_t *pData,
                                uint16_t Size,
                                uint32_t Timeout);
HAL_StatusTypeDef I2C_Mem_Read(uint16_t DevAddress,
                               uint16_t MemAddress,
                               uint16_t MemAddSize,
                               uint8_t *pData,
                               uint16_t Size,
                               uint32_t Timeout);

#endif /* MCAL_I2C_I2C_H_ */
