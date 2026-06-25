/******************************************************************************
 * @file    EEPROM.h
 * @author  Ahmed Abdelrhman
 * @brief   Public/private interface and configuration declarations for EEPROM Driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 * @note    Final GitHub-ready cleanup: comments, spacing, and readability only.
 *          Application behavior and logic are intentionally unchanged.
 ******************************************************************************/

/*
 * EEPROM.h
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 *
 *  Public API for the EEPROM driver.
 *
 *  This module provides external EEPROM memory services.
 *
 *  Features:
 *    - single byte read/write
 *    - page read/write
 *    - generic multi-byte read/write
 *    - EEPROM erase operations
 *
 *  Integration:
 *    - must be initialized once using EEPROM_Init()
 *    - APIs can be used by:
 *        - application layer
 *        - tasks
 *        - cyclic functions
 */

#ifndef EEPROM_EEPROM_H_
#define EEPROM_EEPROM_H_


/******************************************************************************************
 *                              EEPROM_Err_St_t
 *
 *  Return type used by EEPROM APIs.
 *
 *  Values:
 *    EEPROM_Init_Success           : EEPROM initialized successfully.
 *    EEPROM_Init_Failed            : EEPROM initialization failed.
 *
 *    EEPROM_Write_Success          : Write operation successful.
 *    EEPROM_Write_Failed           : Write operation failed.
 *
 *    EEPROM_Read_Success           : Read operation successful.
 *    EEPROM_Read_Failed            : Read operation failed.
 *
 *    EEPROM_Erase_Success          : Erase operation successful.
 *    EEPROM_Erase_Failed           : Erase operation failed.
 *
 *    EEPROM_Invalid_Page_Num_Arg   : Invalid page number argument.
 *    EEPROM_Invalid_Mem_Addr_Arg   : Invalid EEPROM address.
 *    EEPROM_Invalid_Page_Len_Arg   : Invalid data length.
 *    EEPROM_Invalid_Data_Arg       : NULL pointer argument.
 ******************************************************************************************/
typedef enum EEPROM_Err_St_e
{

    EEPROM_Init_Success,
    EEPROM_Init_Failed,

    EEPROM_Write_Success,
    EEPROM_Write_Failed,

    EEPROM_Read_Success,
    EEPROM_Read_Failed,

    EEPROM_Erase_Success,
    EEPROM_Erase_Failed,

    EEPROM_Invalid_Page_Num_Arg,
    EEPROM_Invalid_Mem_Addr_Arg,
    EEPROM_Invalid_Page_Len_Arg,
    EEPROM_Invalid_Data_Arg,

} EEPROM_Err_St_t;


/* Initialize EEPROM driver */
EEPROM_Err_St_t EEPROM_Init(void);

/* Single byte operations */
EEPROM_Err_St_t EEPROM_Write_Byte(uint16_t EEPROM_Mem_Addr, uint8_t byte_val);
EEPROM_Err_St_t EEPROM_Read_Byte(uint16_t EEPROM_Mem_Addr, uint8_t *read_val);

/* Page operations */
EEPROM_Err_St_t EEPROM_Write_Page(uint16_t Page_Num, uint8_t *data, uint8_t lenth);
EEPROM_Err_St_t EEPROM_Read_Page(uint16_t Page_Num, uint8_t *data, uint8_t lenth);

/* Erase operations */
EEPROM_Err_St_t EEPROM_Erase_All(void);
EEPROM_Err_St_t EEPROM_Erase_Page(uint16_t Page_Num);
EEPROM_Err_St_t EEPROM_Erase_Byte(uint16_t EEPROM_Mem_Addr);

/* Generic multi-byte operations */
EEPROM_Err_St_t EEPROM_Write(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth);
EEPROM_Err_St_t EEPROM_Read(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth);

#endif /* EEPROM_EEPROM_H_ */
