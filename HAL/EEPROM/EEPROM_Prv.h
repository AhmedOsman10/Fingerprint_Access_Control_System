/*
 * EEPROM_Prv.h
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 *
 *  Private definitions for EEPROM driver.
 *
 *  This file contains:
 *    - internal driver macros
 *    - EEPROM memory layout information
 *    - internal helper declarations
 *
 *  Note:
 *    This file is intended only for internal EEPROM driver usage.
 */

#ifndef EEPROM_EEPROM_PRV_H_
#define EEPROM_EEPROM_PRV_H_


/******************************************************************************************
 *                              EEPROM_MAX_ADDR
 *
 *  Maximum valid EEPROM memory address.
 *
 *  EEPROM memory range: 0x0000 -> 0x3FFF
 ******************************************************************************************/
#define EEPROM_MAX_ADDR   0x3FFF


/******************************************************************************************
 *                              EEPROM_PAGE_SIZE
 *
 *  EEPROM page size in bytes.
 *  Each EEPROM page contains 64 bytes.
 ******************************************************************************************/
#define EEPROM_PAGE_SIZE    64


/******************************************************************************************
 *                              EEPROM_TOTAL_PAGES
 *
 *  Total number of EEPROM pages.
 ******************************************************************************************/
#define EEPROM_TOTAL_PAGES  256


/******************************************************************************************
 *                      EEPROM_I2C_DEVICE_READY_TRIAL
 *
 *  Number of trials used while checking EEPROM device availability.
 ******************************************************************************************/
#define EEPROM_I2C_DEVICE_READY_TRIAL  3


/******************************************************************************************
 *                              EEPROM_BYTE_LEN
 *
 *  Length of single byte transfer.
 ******************************************************************************************/
#define EEPROM_BYTE_LEN   1


/******************************************************************************************
 *                              MX_I2C3_Init()
 *
 *  Internal helper used to initialize I2C3 peripheral.
 *
 *  Internal Use:
 *    Used only by EEPROM driver.
 ******************************************************************************************/
void MX_I2C3_Init(void);


/******************************************************************************************
 *                          HAL_I2C_MspInit()
 *
 *  STM32 HAL MSP initialization callback.
 *
 *  Responsibilities:
 *    - configure GPIO pins
 *    - enable clocks
 *    - configure NVIC if needed
 ******************************************************************************************/
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c);

#endif /* EEPROM_EEPROM_PRV_H_ */
