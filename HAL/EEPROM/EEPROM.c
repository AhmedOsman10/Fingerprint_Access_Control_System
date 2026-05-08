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


EEPROM_Err_St_t EEPROM_Write_Page(uint16_t Page_Num ,uint8_t *data , uint8_t lenth)
{

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

	if(Page_Num >= EEPROM_TOTAL_PAGES)  return EEPRROM_Invalid_Page_Num_Arg;
	if(lenth >= EEPROM_PAGE_SIZE) return EEPRROM_Invalid_Page_Len_Arg;
	if(data == NULL) return EEPRROM_Invalid_Data_Arg;


	uint16_t EEPROM_Mem_Addr = Page_Num * EEPROM_PAGE_SIZE;

	HAL_StatusTypeDef i2c_st =
			HAL_I2C_Mem_Write(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

	if(i2c_st != HAL_OK)
	{
		EEPROM_Err_St = EEPROM_Write_Failed;
	}


	return EEPROM_Err_St;
}

EEPROM_Err_St_t EEPROM_Write(uint16_t EEPROM_Mem_Addr ,uint8_t *data , uint8_t lenth)
{

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

	if(EEPROM_Mem_Addr >= EEPROM_MAX_ADDR)  return EEPRROM_Invalid_Mem_Addr_Arg;
	if((EEPROM_Mem_Addr  + lenth) >= EEPROM_MAX_ADDR) return EEPRROM_Invalid_Page_Len_Arg;
	if(data == NULL) return EEPRROM_Invalid_Data_Arg;

	HAL_StatusTypeDef i2c_st =
			HAL_I2C_Mem_Write(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

	if(i2c_st != HAL_OK)
	{
		EEPROM_Err_St = EEPROM_Write_Failed;
	}


	return EEPROM_Err_St;
}
EEPROM_Err_St_t EEPROM_Read(uint16_t EEPROM_Mem_Addr , uint8_t *data , uint8_t lenth)
{

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

	if(EEPROM_Mem_Addr >= EEPROM_MAX_ADDR)  return EEPRROM_Invalid_Mem_Addr_Arg;
	if((EEPROM_Mem_Addr  + lenth) >= EEPROM_MAX_ADDR) return EEPRROM_Invalid_Page_Len_Arg;

	if(data == NULL) return EEPRROM_Invalid_Data_Arg;



	HAL_StatusTypeDef i2c_st = HAL_I2C_Mem_Read(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

	if(i2c_st != HAL_OK)
	{
		/* Read failed */
		EEPROM_Err_St = EEPROM_Read_Success;
	}

	return EEPROM_Err_St;
}


EEPROM_Err_St_t EEPROM_Read_Page(uint16_t Page_Num , uint8_t *data , uint8_t lenth)
{

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

	if(Page_Num >= EEPROM_TOTAL_PAGES)  return EEPRROM_Invalid_Page_Num_Arg;
	if(lenth >= EEPROM_PAGE_SIZE) return EEPRROM_Invalid_Page_Len_Arg;
	if(data == NULL) return EEPRROM_Invalid_Data_Arg;

	uint16_t EEPROM_Mem_Addr = Page_Num * EEPROM_PAGE_SIZE;


	HAL_StatusTypeDef i2c_st = HAL_I2C_Mem_Read(EEPROM_I2C_NUM, EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

	if(i2c_st != HAL_OK)
	{
		/* Read failed */
		EEPROM_Err_St = EEPROM_Read_Success;
	}

	return EEPROM_Err_St;
}


EEPROM_Err_St_t EEPROM_Erase_Byte(uint16_t EEPROM_Mem_Addr)
{
	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Erase_Success;

	EEPROM_Err_St = EEPROM_Write_Byte(EEPROM_Mem_Addr , 0xFF);

	if(EEPROM_Err_St == EEPROM_Write_Success)
	{
		EEPROM_Err_St = EEPROM_Erase_Success;
	}
	else
	{
		EEPROM_Err_St = EEPROM_Erase_Failed;
	}

	return EEPROM_Err_St;
}

EEPROM_Err_St_t EEPROM_Erase_Page(uint16_t Page_Num)
{
	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Erase_Success;


	uint8_t buffer[EEPROM_PAGE_SIZE];

	for(uint8_t i= 0 ; i < EEPROM_PAGE_SIZE; i++)
	{
		buffer[i] =0xff;
	}
	EEPROM_Err_St = EEPROM_Write_Page(Page_Num , buffer , EEPROM_PAGE_SIZE);

	if(EEPROM_Err_St == EEPROM_Write_Success)
	{
		EEPROM_Err_St = EEPROM_Erase_Success;
	}
	else
	{
		EEPROM_Err_St = EEPROM_Erase_Failed;
	}

	return EEPROM_Err_St;
}



EEPROM_Err_St_t EEPROM_Erase_All(void){

	EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Erase_Success;
	for(uint16_t page_num = 0 ; page_num < EEPROM_TOTAL_PAGES ; page_num++)
	{
		EEPROM_Err_St = EEPROM_Erase_Page(page_num);

		if(EEPROM_Err_St == EEPROM_Write_Success)
		{
			EEPROM_Err_St = EEPROM_Erase_Success;
		}
		else
		{
			EEPROM_Err_St = EEPROM_Erase_Failed;
			break;
		}
	}
	return EEPROM_Err_St;
}
