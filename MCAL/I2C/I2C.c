/******************************************************************************
 * @file    I2C.c
 * @author  Ahmed Abdelrhman
 * @brief   MCAL I2C driver implementation.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This file initializes and exposes the I2C3 peripheral used by higher-level
 * modules in the project, such as the external RTC and EEPROM drivers.
 *
 * Current hardware mapping:
 *  - I2C3_SCL : PA8
 *  - I2C3_SDA : PC9
 *
 * Notes:
 *  - This driver is based on STM32 HAL I2C services.
 *  - Higher layers should call the wrapper APIs in this file instead of
 *    accessing the HAL handle directly where possible.
 *  - Code behavior is unchanged; only comments and formatting were cleaned.
 ******************************************************************************/

#include "I2C.h"

/******************************************************************************
 * Global I2C Handle
 *
 * hi2c3 is the HAL handle used to control the I2C3 peripheral. It is defined
 * here because this driver owns the I2C3 configuration and initialization.
 ******************************************************************************/
I2C_HandleTypeDef hi2c3;

/******************************************************************************
 * @brief  Initialize the I2C3 peripheral.
 *
 * This function configures I2C3 in standard 100 kHz mode using 7-bit addressing.
 * It replaces the CubeMX-generated MX_I2C3_Init() style logic and keeps the I2C
 * initialization inside the MCAL layer.
 *
 * Used by:
 *  - External RTC driver
 *  - EEPROM driver
 *  - Any other module connected to the same I2C bus
 ******************************************************************************/
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

    if (HAL_I2C_Init(&hi2c3) != HAL_OK)
    {
        /* Error handling can be added here if the project requires it later. */
    }
}

/******************************************************************************
 * @brief  Check whether an I2C device responds on the bus.
 *
 * This is a wrapper around HAL_I2C_IsDeviceReady(). It is useful during device
 * initialization to confirm that the slave device acknowledges its address.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  Trials      Number of acknowledge polling attempts.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_IsDeviceReady(uint16_t DevAddress, uint32_t Trials, uint32_t Timeout)
{
    return HAL_I2C_IsDeviceReady(&hi2c3, DevAddress, Trials, Timeout);
}

/******************************************************************************
 * @brief  Write data to a memory-mapped I2C device.
 *
 * This wrapper is used by external devices such as EEPROM and RTC chips that
 * expose internal registers or memory locations over I2C.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  MemAddress  Internal memory/register address inside the slave device.
 * @param  MemAddSize  Memory address size, such as I2C_MEMADD_SIZE_8BIT.
 * @param  pData       Pointer to the data buffer to transmit.
 * @param  Size        Number of bytes to write.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_Mem_Write(uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    return HAL_I2C_Mem_Write(&hi2c3, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
}

/******************************************************************************
 * @brief  Read data from a memory-mapped I2C device.
 *
 * This wrapper is used by external devices such as EEPROM and RTC chips that
 * expose internal registers or memory locations over I2C.
 *
 * @param  DevAddress  I2C slave address in HAL format.
 * @param  MemAddress  Internal memory/register address inside the slave device.
 * @param  MemAddSize  Memory address size, such as I2C_MEMADD_SIZE_8BIT.
 * @param  pData       Pointer to the buffer where received data will be stored.
 * @param  Size        Number of bytes to read.
 * @param  Timeout     Timeout value in milliseconds.
 *
 * @return HAL status.
 ******************************************************************************/
HAL_StatusTypeDef I2C_Mem_Read(uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    return HAL_I2C_Mem_Read(&hi2c3, DevAddress, MemAddress, MemAddSize, pData, Size, Timeout);
}

/******************************************************************************
 * @brief  Initialize low-level clocks and GPIO pins for I2C3.
 *
 * This function is called automatically by HAL_I2C_Init(). It enables the GPIO
 * clocks, enables the I2C3 peripheral clock, and configures the pins in
 * alternate-function open-drain mode.
 *
 * Pin mapping:
 *  - PC9 -> I2C3_SDA
 *  - PA8 -> I2C3_SCL
 *
 * @param  hi2c  Pointer to the HAL I2C handle being initialized.
 ******************************************************************************/
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hi2c->Instance == I2C3)
    {
        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_I2C3_CLK_ENABLE();

        /* Configure PC9 as I2C3 SDA. */
        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* Configure PA8 as I2C3 SCL. */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}
