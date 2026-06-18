/******************************************************************************
 * @file    EEPROM_Cfg.h
 * @author  Ahmed Abdelrhman
 * @brief   Public/private interface and configuration declarations for EEPROM Driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 * @note    Final GitHub-ready cleanup: comments, spacing, and readability only.
 *          Application behavior and logic are intentionally unchanged.
 ******************************************************************************/

/*
 * EEPROM_Cfg.h
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 *
 *  EEPROM driver configuration file.
 *
 *  This file contains user configurable parameters used by the EEPROM driver.
 */

#ifndef EEPROM_EEPROM_CFG_H_
#define EEPROM_EEPROM_CFG_H_


/******************************************************************************************
 *                              EEPROM_SLAVE_ADDR
 *
 *  EEPROM I2C slave address.
 *
 *  Note:
 *    STM32 HAL expects shifted 8-bit address format.
 ******************************************************************************************/
#define EEPROM_SLAVE_ADDR  0xA0


/******************************************************************************************
 *                          EEMPROM_MAX_TIMEOUT
 *
 *  Maximum timeout used by EEPROM I2C operations.
 *
 *  Unit:
 *    milliseconds
 ******************************************************************************************/
#define EEMPROM_MAX_TIMEOUT  5000

#endif /* EEPROM_EEPROM_CFG_H_ */
