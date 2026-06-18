/******************************************************************************
 * @file    EEPROM.c
 * @author  Ahmed Abdelrhman
 * @brief   Implementation file for EEPROM Driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 * @note    Final GitHub-ready cleanup: comments, spacing, and readability only.
 *          Application behavior and logic are intentionally unchanged.
 ******************************************************************************/

/*
 * EEPROM.c
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 *
 *  EEPROM driver implementation.
 *
 *  This file provides the implementation of the external EEPROM driver.
 *
 *  Responsibilities:
 *    - use MCAL I2C interface for EEPROM communication
 *    - check EEPROM device availability on I2C bus
 *    - write single bytes into EEPROM memory
 *    - read single bytes from EEPROM memory
 *    - write full pages into EEPROM
 *    - read full pages from EEPROM
 *    - erase EEPROM memory locations
 *    - perform generic multi-byte read/write operations
 *
 *  Communication:
 *    - EEPROM communication is performed over I2C3
 *    - STM32 HAL I2C memory APIs are used
 *
 *  EEPROM Addressing:
 *    - EEPROM uses 16-bit memory addressing
 *    - Memory is divided into pages
 *    - Each page contains EEPROM_PAGE_SIZE bytes
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2c_ex.h"
#include "I2C.h"

#include "EEPROM_Prv.h"
#include "EEPROM_Cfg.h"
#include "EEPROM.h"

/******************************************************************************************
 *                                  EEPROM_Init()
 *
 *  Initialize EEPROM driver.
 *
 *  Description:
 *    - Initializes MCAL I2C driver.
 *    - Verifies EEPROM device presence on the I2C bus.
 *
 *  Internal Behavior:
 *    - Calls I2C_Init().
 *    - Uses MCAL I2C device-ready wrapper to verify EEPROM response.
 *
 *  Returns:
 *    EEPROM_Init_Success: EEPROM initialized successfully.
 *    EEPROM_Init_Failed : EEPROM initialization failed.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Init(void)
{
    /* Default return assumes initialization failed */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Init_Failed;

    /* HAL status used to evaluate I2C operation */
    HAL_StatusTypeDef I2C_St;

    /* Initialize MCAL I2C driver */
    I2C_Init();

    /* Check whether EEPROM slave is responding on the I2C bus */
    I2C_St = I2C_IsDeviceReady(EEPROM_SLAVE_ADDR, EEPROM_I2C_DEVICE_READY_TRIAL, EEMPROM_MAX_TIMEOUT);

    if (I2C_St == HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Init_Success;
    }
    else
    {
        EEPROM_Err_St = EEPROM_Init_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Write_Byte()
 *
 *  Write single byte into EEPROM memory.
 *
 *  Parameters:
 *    EEPROM_Mem_Addr:
 *        Target EEPROM memory address.
 *
 *    byte_val:
 *        Data byte to be written.
 *
 *  Internal Behavior:
 *    - Validates memory address.
 *    - Uses MCAL I2C memory write wrapper with 16-bit memory addressing.
 *
 *  Returns:
 *    EEPROM_Write_Success          : Write completed successfully.
 *    EEPROM_Write_Failed           : I2C write operation failed.
 *    EEPROM_Invalid_Mem_Addr_Arg   : Invalid EEPROM address.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Write_Byte(uint16_t EEPROM_Mem_Addr, uint8_t byte_val)
{
    /* Default return assumes write success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

    /* Validate EEPROM memory address */
    if (EEPROM_Mem_Addr >= EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Mem_Addr_Arg;
    }

    /* Write one byte into EEPROM memory */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Write(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, &byte_val, EEPROM_BYTE_LEN, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Write_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Read_Byte()
 *
 *  Read single byte from EEPROM memory.
 *
 *  Parameters:
 *    EEPROM_Mem_Addr:
 *        EEPROM memory address to read from.
 *
 *    read_val:
 *        Pointer where received byte will be stored.
 *
 *  Internal Behavior:
 *    - Validates address and pointer.
 *    - Uses MCAL I2C memory read wrapper with 16-bit addressing.
 *
 *  Returns:
 *    EEPROM_Read_Success           : Read completed successfully.
 *    EEPROM_Read_Failed            : I2C read operation failed.
 *    EEPROM_Invalid_Mem_Addr_Arg   : Invalid EEPROM address.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Read_Byte(uint16_t EEPROM_Mem_Addr, uint8_t *read_val)
{
    /* Default return assumes read success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

    /* Validate EEPROM memory address */
    if (EEPROM_Mem_Addr >= EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Mem_Addr_Arg;
    }

    /* Validate output pointer */
    if (read_val == NULL)
    {
        return EEPROM_Invalid_Data_Arg;
    }

    /* Read one byte from EEPROM memory */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Read(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, read_val, EEPROM_BYTE_LEN, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Read_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Write_Page()
 *
 *  Write multiple bytes into a specific EEPROM page.
 *
 *  Parameters:
 *    Page_Num:
 *        EEPROM page number.
 *
 *    data:
 *        Pointer to data buffer.
 *
 *    lenth:
 *        Number of bytes to write.
 *
 *  Internal Behavior:
 *    - Calculates EEPROM memory address from page number.
 *    - Writes data sequentially into EEPROM.
 *
 *  Returns:
 *    EEPROM_Write_Success          : Page write successful.
 *    EEPROM_Write_Failed           : I2C write failed.
 *    EEPROM_Invalid_Page_Num_Arg   : Invalid page number.
 *    EEPROM_Invalid_Page_Len_Arg   : Invalid page length.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Write_Page(uint16_t Page_Num, uint8_t *data, uint8_t lenth)
{
    /* Default return assumes write success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

    /* Validate page number */
    if (Page_Num >= EEPROM_TOTAL_PAGES)
    {
        return EEPROM_Invalid_Page_Num_Arg;
    }

    /* Validate page length */
    if (lenth > EEPROM_PAGE_SIZE)
    {
        return EEPROM_Invalid_Page_Len_Arg;
    }

    /* Validate data pointer */
    if (data == NULL)
    {
        return EEPROM_Invalid_Data_Arg;
    }

    /* Convert page number into EEPROM memory address */
    uint16_t EEPROM_Mem_Addr = Page_Num * EEPROM_PAGE_SIZE;

    /* Write page data into EEPROM */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Write(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Write_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                                  EEPROM_Write()
 *
 *  Generic multi-byte EEPROM write.
 *
 *  Parameters:
 *    EEPROM_Mem_Addr:
 *        Start EEPROM memory address.
 *
 *    data:
 *        Pointer to data buffer.
 *
 *    lenth:
 *        Number of bytes to write.
 *
 *  Internal Behavior:
 *    - Validates memory range.
 *    - Performs continuous EEPROM write operation.
 *
 *  Returns:
 *    EEPROM_Write_Success          : Write completed successfully.
 *    EEPROM_Write_Failed           : I2C write failed.
 *    EEPROM_Invalid_Mem_Addr_Arg   : Invalid EEPROM address.
 *    EEPROM_Invalid_Page_Len_Arg   : Invalid write range.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Write(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth)
{
    /* Default return assumes write success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Write_Success;

    /* Validate EEPROM memory address */
    if (EEPROM_Mem_Addr > EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Mem_Addr_Arg;
    }

    /* Validate write range */
    if ((EEPROM_Mem_Addr + lenth) > EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Page_Len_Arg;
    }

    /* Validate data pointer */
    if (data == NULL)
    {
        return EEPROM_Invalid_Data_Arg;
    }

    /* Write multiple bytes into EEPROM */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Write(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Write_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                                  EEPROM_Read()
 *
 *  Generic multi-byte EEPROM read.
 *
 *  Parameters:
 *    EEPROM_Mem_Addr:
 *        Start EEPROM memory address.
 *
 *    data:
 *        Pointer to destination buffer.
 *
 *    lenth:
 *        Number of bytes to read.
 *
 *  Returns:
 *    EEPROM_Read_Success           : Read completed successfully.
 *    EEPROM_Read_Failed            : I2C read failed.
 *    EEPROM_Invalid_Mem_Addr_Arg   : Invalid EEPROM address.
 *    EEPROM_Invalid_Page_Len_Arg   : Invalid read range.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Read(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth)
{
    /* Default return assumes read success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

    /* Validate EEPROM memory address */
    if (EEPROM_Mem_Addr >= EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Mem_Addr_Arg;
    }

    /* Validate memory range */
    if ((EEPROM_Mem_Addr + lenth) > EEPROM_MAX_ADDR)
    {
        return EEPROM_Invalid_Page_Len_Arg;
    }

    /* Validate data pointer */
    if (data == NULL)
    {
        return EEPROM_Invalid_Data_Arg;
    }

    /* Read multiple bytes from EEPROM */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Read(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Read_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Read_Page()
 *
 *  Read bytes from a specific EEPROM page.
 *
 *  Parameters:
 *    Page_Num:
 *        EEPROM page number.
 *
 *    data:
 *        Pointer to destination buffer.
 *
 *    lenth:
 *        Number of bytes to read.
 *
 *  Returns:
 *    EEPROM_Read_Success           : Page read successful.
 *    EEPROM_Read_Failed            : I2C read failed.
 *    EEPROM_Invalid_Page_Num_Arg   : Invalid page number.
 *    EEPROM_Invalid_Page_Len_Arg   : Invalid page length.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Read_Page(uint16_t Page_Num, uint8_t *data, uint8_t lenth)
{
    /* Default return assumes read success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Read_Success;

    /* Validate page number */
    if (Page_Num >= EEPROM_TOTAL_PAGES)
    {
        return EEPROM_Invalid_Page_Num_Arg;
    }

    /* Validate requested length */
    if (lenth > EEPROM_PAGE_SIZE)
    {
        return EEPROM_Invalid_Page_Len_Arg;
    }

    /* Validate data pointer */
    if (data == NULL)
    {
        return EEPROM_Invalid_Data_Arg;
    }

    /* Convert page number into EEPROM memory address */
    uint16_t EEPROM_Mem_Addr = Page_Num * EEPROM_PAGE_SIZE;

    /* Read page data from EEPROM */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Read(EEPROM_SLAVE_ADDR, EEPROM_Mem_Addr, I2C_MEMADD_SIZE_16BIT, data, lenth, EEMPROM_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        EEPROM_Err_St = EEPROM_Read_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Erase_Byte()
 *
 *  Erase single EEPROM byte.
 *
 *  Description:
 *    EEPROM erased state is typically 0xFF.
 *
 *  Parameters:
 *    EEPROM_Mem_Addr:
 *        EEPROM memory address to erase.
 *
 *  Returns:
 *    EEPROM_Erase_Success : Byte erased successfully.
 *    EEPROM_Erase_Failed  : Erase operation failed.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Erase_Byte(uint16_t EEPROM_Mem_Addr)
{
    /* Default return assumes erase success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Erase_Success;

    /* Erase byte by writing 0xFF */
    EEPROM_Err_St = EEPROM_Write_Byte(EEPROM_Mem_Addr, 0xFF);

    if (EEPROM_Err_St == EEPROM_Write_Success)
    {
        EEPROM_Err_St = EEPROM_Erase_Success;
    }
    else
    {
        EEPROM_Err_St = EEPROM_Erase_Failed;
    }

    return EEPROM_Err_St;
}


/******************************************************************************************
 *                              EEPROM_Erase_Page()
 *
 *  Erase full EEPROM page.
 *
 *  Description:
 *    - Fills temporary buffer with 0xFF.
 *    - Writes the full buffer into selected EEPROM page.
 *
 *  Parameters:
 *    Page_Num:
 *        EEPROM page number to erase.
 *
 *  Returns:
 *    EEPROM_Erase_Success : Page erased successfully.
 *    EEPROM_Erase_Failed  : Page erase failed.
 ******************************************************************************************/
EEPROM_Err_St_t EEPROM_Erase_Page(uint16_t Page_Num)
{
    /* Default return assumes erase success */
    EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Erase_Success;

    /* Temporary erase buffer */
    uint8_t buffer[EEPROM_PAGE_SIZE];

    /* Fill page buffer with erased state value */
    for (uint8_t i = 0; i < EEPROM_PAGE_SIZE; i++)
    {
        buffer[i] = 0xFF;
    }

    /* Write erase pattern into EEPROM page */
    EEPROM_Err_St = EEPROM_Write_Page(Page_Num, buffer, EEPROM_PAGE_SIZE);

    if (EEPROM_Err_St == EEPROM_Write_Success)
    {
        EEPROM_Err_St = EEPROM_Erase_Success;
    }
    else
    {
        EEPROM_Err_St = EEPROM_Erase_Failed;
    }

    return EEPROM_Err_St;
}
