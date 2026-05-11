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
 *                              EEPROM_I2C_NUM
 *
 *  I2C handle used for EEPROM communication.
 *
 *  Current configuration:
 *    - I2C3 peripheral
 ******************************************************************************************/
#define EEPROM_I2C_NUM   &hi2c3


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
