/*
 * RTC.c
 *
 *  Created on: 27 Feb 2026
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

#include "RTC_Prv.h"
#include "RTC.h"

I2C_HandleTypeDef hi2c3;


RTC_Err_St_t RTC_Init(void)
{
	RTC_Err_St_t RTC_Err_St = RTC_Init_Success;
	HAL_StatusTypeDef I2C_St;

	MX_I2C3_Init();

	I2C_St = HAL_I2C_IsDeviceReady(&hi2c3, RTC_SLAVE_ADDR, RTC_I2C_DEVICE_READY_TRIAL, RTC_I2C_TIMEOUT);
	if(I2C_St == HAL_OK)
	{
		RTC_Err_St = RTC_Init_Success ;
	}
	else
	{
		RTC_Err_St = RTC_Init_Failed;
	}

	return  RTC_Err_St;
}


RTC_Err_St_t RTC_SetTime(RTC_Time_t *time)
{
	RTC_Err_St_t RTC_Err_St = RTC_SetTime_Success ;
	uint8_t Time[RTC_TIME_SIZE];

	Time[0] = DecToBCD(time->seconds) & 0x7f;
	Time[1] = DecToBCD(time->minutes);
	Time[2] = DecToBCD(time->hours);

	HAL_StatusTypeDef i2c_st =	HAL_I2C_Mem_Write(&hi2c3, RTC_SLAVE_ADDR, RTC_MEM_SECONDS_ADDR, I2C_MEMADD_SIZE_8BIT, Time , RTC_TIME_SIZE , RTC_MAX_TIMEOUT );
	if(i2c_st != HAL_OK)
	{
		RTC_Err_St = RTC_SetTime_Failed;
	}
	else
	{
		/* do nothing */
	}

	return RTC_Err_St;
}


RTC_Err_St_t RTC_SetDate(RTC_Date_t *date)
{
	RTC_Err_St_t RTC_Err_St = RTC_SetDate_Success;
	uint8_t Date[RTC_DATE_SIZE];

	Date[0] = DecToBCD(date->Day);
	Date[1] = DecToBCD(date->Date);
	Date[2] = DecToBCD(date->month);
	Date[3] = DecToBCD(date->year);

	HAL_StatusTypeDef i2c_st =	HAL_I2C_Mem_Write(&hi2c3,RTC_SLAVE_ADDR, RTC_MEM_DAY_ADDR, I2C_MEMADD_SIZE_8BIT, Date, RTC_DATE_SIZE , RTC_MAX_TIMEOUT);
	if(i2c_st != HAL_OK)
	{
		RTC_Err_St = RTC_SetDate_Failed;
	}
	else
	{
		/* do nothing */
	}

	return RTC_Err_St;
}


RTC_Err_St_t RTC_GetTime(RTC_Time_t *time)
{
	RTC_Err_St_t RTC_Err_St = RTC_GetTime_Success ;
	uint8_t Time[RTC_TIME_SIZE];

	HAL_StatusTypeDef i2c_st = HAL_I2C_Mem_Read(&hi2c3, RTC_SLAVE_ADDR, RTC_MEM_SECONDS_ADDR, I2C_MEMADD_SIZE_8BIT, Time, RTC_TIME_SIZE, RTC_MAX_TIMEOUT);
	if(i2c_st != HAL_OK)
	{
		RTC_Err_St = RTC_SetTime_Failed;
	}
	else
	{
		time->seconds = BCDToDec(Time[0]);
		time->minutes = BCDToDec(Time[1]);
		time->hours = BCDToDec(Time[2] ) & 0x3f ;
		/* do nothing */
	}

	return RTC_Err_St;
}


RTC_Err_St_t RTC_GetDate(RTC_Date_t *date)
{
	RTC_Err_St_t RTC_Err_St = RTC_GetDate_Success;
	uint8_t Date[RTC_DATE_SIZE];

	HAL_StatusTypeDef i2c_st = HAL_I2C_Mem_Read(&hi2c3, RTC_SLAVE_ADDR, RTC_MEM_DAY_ADDR, I2C_MEMADD_SIZE_8BIT, Date, RTC_DATE_SIZE, RTC_MAX_TIMEOUT);
	if(i2c_st != HAL_OK)
	{
		RTC_Err_St = RTC_GetDate_Failed;
	}
	else
	{
		date->Day = BCDToDec(Date[0]);
		date->Date = BCDToDec(Date[1]);
		date->month = BCDToDec(Date[2] )  & 0x1f;
		date->year = BCDToDec(Date[3]) + 2000;

	}

	return RTC_Err_St;
}


/**
 * @brief I2C3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C3_Init(void)
{

	/* USER CODE BEGIN I2C3_Init 0 */

	/* USER CODE END I2C3_Init 0 */

	/* USER CODE BEGIN I2C3_Init 1 */

	/* USER CODE END I2C3_Init 1 */
	hi2c3.Instance = I2C3;
	hi2c3.Init.ClockSpeed = 100000;
	hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c3.Init.OwnAddress1 = 0;
	hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c3.Init.OwnAddress2 = 0;
	hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c3) != HAL_OK)
	{
//		Error_Handler();
	}
	/* USER CODE BEGIN I2C3_Init 2 */

	/* USER CODE END I2C3_Init 2 */

}

/**
 * @brief I2C MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if(hi2c->Instance==I2C3)
	{
		/* USER CODE BEGIN I2C3_MspInit 0 */

		/* USER CODE END I2C3_MspInit 0 */

		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOA_CLK_ENABLE();
		/**I2C3 GPIO Configuration
    PC9     ------> I2C3_SDA
    PA8     ------> I2C3_SCL
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

		/* Peripheral clock enable */
		__HAL_RCC_I2C3_CLK_ENABLE();
		/* USER CODE BEGIN I2C3_MspInit 1 */

		/* USER CODE END I2C3_MspInit 1 */
	}

}


