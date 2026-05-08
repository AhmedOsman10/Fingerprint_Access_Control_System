/*
 * EEPROM.c
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 */


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2c_ex.h"


#include "EEPROM_Prv.h"
#include "EEPROM_Cfg.h"
#include "EEPROM.h"

extern I2C_HandleTypeDef hi2c3;

EEPROM_Err_St_t EEPROM_Init(void)
{
	/* Default return assumes initialization will succeed */
	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Init_Failed;

	/* HAL status used to evaluate I2C operation */
	HAL_StatusTypeDef I2C_St;

	/* Initialize I2C3 peripheral and related GPIO configuration */
	MX_I2C3_Init();

	/* Check whether RTC slave is responding on the I2C bus */
	I2C_St = HAL_I2C_IsDeviceReady(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_I2C_DEVICE_READY_TRIAL, EEMPROM_MAX_TIMEOUT);

	if(I2C_St == HAL_OK)
	{
		EEPROM_Err_St = EEPROM_Init_Success;
	}
	else
	{
		EEPROM_Err_St = EEPROM_Init_Failed;
	}

	return EEPROM_Err_St;
}

EEPROM_Err_St_t EEPROM_Write_Byte(uint16_t EEPROM_Mem_Addr , uint8_t byte_val )
{

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

		HAL_StatusTypeDef i2c_st =
				HAL_I2C_Mem_Write(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, &byte_val, EEPROM_BYTE_LEN, EEMPROM_MAX_TIMEOUT);

		if(i2c_st != HAL_OK)
		{
			EEPROM_Err_St = EEPROM_Write_Failed;
		}


		return EEPROM_Err_St;
}


EEPROM_Err_St_t  EEPROM_Read_Byte(uint16_t EEPROM_Mem_Addr , uint8_t *read_val)
{
	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

		HAL_StatusTypeDef i2c_st = HAL_I2C_Mem_Read(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, read_val, EEPROM_BYTE_LEN, EEMPROM_MAX_TIMEOUT);

		if(i2c_st != HAL_OK)
		{
			/* Read failed */
			EEPROM_Err_St = EEPROM_Read_Success;
		}

		return EEPROM_Err_St;
}



