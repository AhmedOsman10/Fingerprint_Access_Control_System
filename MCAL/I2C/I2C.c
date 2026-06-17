/*
 * I2C.c
 *
 *  Created on: 17 Jun 2026
 *      Author: Ahmed
 *
 *  MCAL I2C driver implementation.
 *
 *  Responsibilities:
 *    - Own I2C3 handle definition
 *    - Initialize I2C3 peripheral and GPIO pins
 *    - Provide memory read/write APIs for HAL drivers
 *
 *  Current hardware mapping:
 *    - I2C3_SDA -> PC9
 *    - I2C3_SCL -> PA8
 */

#include "I2C.h"

/* Global I2C3 handle owned by MCAL I2C driver. */
I2C_HandleTypeDef hi2c3;

/******************************************************************************************
 *                                      I2C_Init()
 *
 *  Initialize I2C3 peripheral.
 *
 *  Notes:
 *    - This is the previous MX_I2C3_Init() logic moved from RTC driver to MCAL.
 *    - It is safe for EEPROM_Init() and RTC_Init() to call this before use.
 ******************************************************************************************/
void I2C_Init(void)
{
    hi2c3.Instance = I2C3;
    hi2c3.Init.ClockSpeed = 100000;
    hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if(HAL_I2C_Init(&hi2c3) != HAL_OK)
    {
        /* Error handler can be added later if required. */
    }
}

HAL_StatusTypeDef I2C_IsDeviceReady(uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
    return HAL_I2C_IsDeviceReady(&hi2c3, DevAddress, Trials, Timeout);
}

HAL_StatusTypeDef I2C_Mem_Write(uint16_t DevAddress,
                                uint16_t MemAddress,
                                uint16_t MemAddSize,
                                uint8_t *pData,
                                uint16_t Size,
                                uint32_t Timeout)
{
    return HAL_I2C_Mem_Write(&hi2c3, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
}

HAL_StatusTypeDef I2C_Mem_Read(uint16_t DevAddress,
                               uint16_t MemAddress,
                               uint16_t MemAddSize,
                               uint8_t *pData,
                               uint16_t Size,
                               uint32_t Timeout)
{
    return HAL_I2C_Mem_Read(&hi2c3, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
}

/******************************************************************************************
 *                                  HAL_I2C_MspInit()
 *
 *  Low-level GPIO and clock initialization for I2C3.
 *
 *  This function is called internally by HAL_I2C_Init().
 ******************************************************************************************/
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if(hi2c->Instance == I2C3)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_I2C3_CLK_ENABLE();

        /* PC9 -> I2C3_SDA */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* PA8 -> I2C3_SCL */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
