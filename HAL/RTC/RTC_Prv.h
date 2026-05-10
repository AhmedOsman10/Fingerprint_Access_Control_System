/*
 * RTC_Prv.h
 *
 *  Created on: 27 Feb 2026
 *      Author: Ahmed
 *
 *  Private definitions for the RTC driver.
 *
 *  This file contains the internal macros and helper declarations used only
 *  inside the RTC driver implementation.
 *
 *  This file is not intended for direct use by the application layer.
 *  The application layer shall use RTC.h only.
 */

#ifndef RTC_RTC_PRV_H_
#define RTC_RTC_PRV_H_


/******************************************************************************************
 *                          RTC Register Address Macros
 *
 *  These macros define the internal register addresses used by the RTC device.
 *
 *  RTC_MEM_SECONDS_ADDR:  Start address of the time registers.
 *                         Used when reading or writing:
 *                           - seconds
 *                           - minutes
 *                           - hours
 *
 *  RTC_MEM_DAY_ADDR:      Start address of the date registers.
 *                         Used when reading or writing:
 *                           - day
 *                           - date
 *                           - month
 *                           - year
 ******************************************************************************************/
#define RTC_MEM_SECONDS_ADDR  0x00
#define RTC_MEM_DAY_ADDR      0x03


/******************************************************************************************
 *                          RTC Data Size Macros
 *
 *  These macros define the number of bytes used for time/date transfers.
 *
 *  RTC_TIME_SIZE:  Number of bytes used for time transfer.
 *                  Format:
 *                    [0] seconds
 *                    [1] minutes
 *                    [2] hours
 *
 *  RTC_DATE_SIZE:  Number of bytes used for date transfer.
 *                  Format:
 *                    [0] day
 *                    [1] date
 *                    [2] month
 *                    [3] year
 ******************************************************************************************/
#define RTC_TIME_SIZE    3
#define RTC_DATE_SIZE    4


/******************************************************************************************
 *                          RTC Timeout and I2C Settings
 *
 *  These macros define internal communication parameters used by the RTC driver.
 *
 *  RTC_MAX_TIMEOUT:             Maximum timeout used for standard RTC I2C
 *                               memory read/write operations.
 *
 *  RTC_SLAVE_ADDR:              7-bit RTC device address shifted left by 1,
 *                               as expected by STM32 HAL I2C APIs.
 *
 *  RTC_I2C_DEVICE_READY_TRIAL:  Number of attempts used when checking whether
 *                               the RTC device is ready on the I2C bus.
 *
 *  RTC_I2C_TIMEOUT:             Timeout used by HAL_I2C_IsDeviceReady().
 ******************************************************************************************/
#define RTC_MAX_TIMEOUT              100
#define RTC_SLAVE_ADDR               (0x68 << 1)
#define RTC_I2C_DEVICE_READY_TRIAL   3
#define RTC_I2C_TIMEOUT              3000


/******************************************************************************************
 *                          BCD Conversion Macros
 *
 *  The RTC device stores time/date fields in BCD (Binary-Coded Decimal) format.
 *  These helper macros convert between decimal and BCD.
 *
 *  DecToBCD(val):  Convert decimal value into BCD format.
 *
 *  BCDToDec(val):  Convert BCD value into decimal format.
 *
 *  Example:
 *    Decimal 25  -> BCD 0x25
 *    BCD 0x42    -> Decimal 42
 ******************************************************************************************/
#define DecToBCD(val)    ((((val) / 10) << 4) | ((val) % 10))
#define BCDToDec(val)    ((((val) >> 4) * 10) + ((val) & 0x0F))


/******************************************************************************************
 *                                  MX_I2C3_Init()
 *
 *  Internal I2C3 initialization helper.
 *
 *  Description:
 *    - Configures the I2C3 peripheral used by the RTC driver.
 *    - Sets the required I2C timing and addressing parameters.
 *    - Called during RTC_Init().
 *
 *  Internal Use:
 *    - Used only by RTC.c
 *    - Not part of the public RTC API
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
 void MX_I2C3_Init(void);


#endif /* RTC_RTC_PRV_H_ */
