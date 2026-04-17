/*
 * RTC_Prv.h
 *
 *  Created on: 27 Feb 2026
 *      Author: Ahmed
 */

#ifndef RTC_RTC_PRV_H_
#define RTC_RTC_PRV_H_


#define RTC_MEM_SECONDS_ADDR  0x00
#define RTC_MEM_DAY_ADDR  	  0x03

#define RTC_TIME_SIZE  3
#define RTC_DATE_SIZE  4
#define RTC_MAX_TIMEOUT  100

#define RTC_SLAVE_ADDR                  (  0x68 << 1)
#define RTC_I2C_DEVICE_READY_TRIAL          3
#define RTC_I2C_TIMEOUT                     3000

#define DecToBCD(val)    (  ((val / 10) << 4)  | (val % 10) )
#define BCDToDec(val)    (  ((val >> 4) * 10)  + (val & 0x0f) )


static void MX_I2C3_Init(void);



#endif /* RTC_RTC_PRV_H_ */
