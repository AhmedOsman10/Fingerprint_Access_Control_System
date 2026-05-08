/*
 * EEPROM_Prv.h
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 */

#ifndef EEPROM_EEPROM_PRV_H_
#define EEPROM_EEPROM_PRV_H_

#define EEPROM_MAX_ADDR   0x3FFF


#define EEPROM_PAGE_SIZE    64

#define EEPROM_TOTAL_PAGES  256

#define EEPROM_I2C_DEVICE_READY_TRIAL  3
#define EEPROM_BYTE_LEN   1

 void MX_I2C3_Init(void);
void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c);
#endif /* EEPROM_EEPROM_PRV_H_ */
