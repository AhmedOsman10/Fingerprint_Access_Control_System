/*
 * RTC.c
 *
 *  Created on: 27 Feb 2026
 *      Author: Ahmed
 *
 *  RTC driver implementation.
 *
 *  This file provides the implementation of the RTC driver APIs.
 *
 *  Responsibilities:
 *    - initialize the RTC driver
 *    - check RTC device availability through MCAL I2C driver
 *    - write time values to RTC registers
 *    - write date values to RTC registers
 *    - read time values from RTC registers
 *    - read date values from RTC registers
 *
 *  Communication:
 *    - RTC communication is performed over I2C3
 *    - I2C access is abstracted through the MCAL I2C driver
 *
 *  Data Format:
 *    - The external RTC device stores time/date values in BCD format
 *    - The driver converts between:
 *        - decimal format used by the application
 *        - BCD format used by the RTC device
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "RTC_Prv.h"
#include "RTC.h"

#include "I2C.h"


/******************************************************************************************
 *                                  RTC_Init()
 *
 *  Initialize the RTC driver.
 *
 *  Description:
 *    - Initializes the I2C peripheral used by the RTC through MCAL I2C driver.
 *    - Checks whether the RTC device is present and ready on the I2C bus.
 *
 *  Internal Behavior:
 *    - Calls I2C_Init() for the configured RTC I2C channel.
 *    - Calls I2C_IsDeviceReady() to verify communication with the RTC device.
 *
 *  Returns:
 *    RTC_Init_Success: RTC initialized and device is reachable.
 *    RTC_Init_Failed:  RTC initialization failed or device not detected.
 ******************************************************************************************/
RTC_Err_St_t RTC_Init(void)
{
	RTC_Err_St_t RTC_Err_St = RTC_Init_Success;

	/* Initialize the I2C channel used by RTC */
	if(I2C_Init(RTC_I2C_NUM) != I2C_Init_Success)
	{
		RTC_Err_St = RTC_Init_Failed;
	}
	else
	{
		/* Check whether RTC slave is responding on the I2C bus */
		if(I2C_IsDeviceReady(RTC_I2C_NUM, RTC_SLAVE_ADDR) != I2C_Device_Ready)
		{
			RTC_Err_St = RTC_Init_Failed;
		}
	}

	return RTC_Err_St;
}


/******************************************************************************************
 *                                  RTC_SetTime()
 *
 *  Set current time in the RTC device.
 *
 *  Description:
 *    - Converts application time values from decimal to BCD format.
 *    - Writes seconds, minutes, and hours into RTC time registers.
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t containing:
 *          - seconds
 *          - minutes
 *          - hours
 *
 *  Internal Behavior:
 *    - Builds a local 3-byte buffer:
 *        [0] seconds
 *        [1] minutes
 *        [2] hours
 *    - Writes the buffer starting from the seconds register address.
 *
 *  Returns:
 *    RTC_SetTime_Success: Time written successfully.
 *    RTC_SetTime_Failed:  Time write operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_SetTime(RTC_Time_t *time)
{
	RTC_Err_St_t RTC_Err_St = RTC_SetTime_Success;

	uint8_t Time[RTC_TIME_SIZE];

	Time[0] = DecToBCD(time->seconds) & 0x7F;
	Time[1] = DecToBCD(time->minutes);
	Time[2] = DecToBCD(time->hours);

	if(I2C_MemWrite(RTC_I2C_NUM,
			        RTC_SLAVE_ADDR,
			        RTC_MEM_SECONDS_ADDR,
			        Time,
			        RTC_TIME_SIZE) != I2C_Write_Success)
	{
		RTC_Err_St = RTC_SetTime_Failed;
	}

	return RTC_Err_St;
}


/******************************************************************************************
 *                                  RTC_SetDate()
 *
 *  Set current date in the RTC device.
 *
 *  Description:
 *    - Converts application date values from decimal to BCD format.
 *    - Writes day, date, month, and year into RTC date registers.
 *
 *  Parameters:
 *    date: Pointer to RTC_Date_t containing:
 *          - Day   (day of week)
 *          - Date  (day of month)
 *          - month
 *          - year
 *
 *  Internal Behavior:
 *    - Builds a local 4-byte buffer:
 *        [0] Day
 *        [1] Date
 *        [2] month
 *        [3] year
 *    - Writes the buffer starting from the RTC day register address.
 *
 *  Returns:
 *    RTC_SetDate_Success: Date written successfully.
 *    RTC_SetDate_Failed:  Date write operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_SetDate(RTC_Date_t *date)
{
	RTC_Err_St_t RTC_Err_St = RTC_SetDate_Success;

	uint8_t Date[RTC_DATE_SIZE];

	Date[0] = DecToBCD(date->Day);
	Date[1] = DecToBCD(date->Date);
	Date[2] = DecToBCD(date->month);
	Date[3] = DecToBCD((uint8_t)(date->year % 100));

	if(I2C_MemWrite(RTC_I2C_NUM,
			        RTC_SLAVE_ADDR,
			        RTC_MEM_DAY_ADDR,
			        Date,
			        RTC_DATE_SIZE) != I2C_Write_Success)
	{
		RTC_Err_St = RTC_SetDate_Failed;
	}

	return RTC_Err_St;
}


/******************************************************************************************
 *                                  RTC_SetTimeDate()
 *
 *  Set both time and date.
 *
 *  Description:
 *    - Calls RTC_SetTime() first.
 *    - If time update succeeds, calls RTC_SetDate().
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t.
 *    date: Pointer to RTC_Date_t.
 *
 *  Returns:
 *    RTC_SetTime_Success: Both time and date were written successfully.
 *    RTC_SetTime_Failed:  Failed while setting time.
 *    RTC_SetDate_Failed:  Failed while setting date after time succeeded.
 ******************************************************************************************/
RTC_Err_St_t RTC_SetTimeDate(RTC_Time_t *time , RTC_Date_t *date)
{
	RTC_Err_St_t rtc_err_st = RTC_SetTime(time);

	if(rtc_err_st != RTC_SetTime_Success)
	{
		return RTC_SetTime_Failed;
	}

	rtc_err_st = RTC_SetDate(date);

	if(rtc_err_st != RTC_SetDate_Success)
	{
		return RTC_SetDate_Failed;
	}

	return RTC_SetTime_Success;
}


/******************************************************************************************
 *                                  RTC_GetTime()
 *
 *  Read current time from the RTC device.
 *
 *  Description:
 *    - Reads seconds, minutes, and hours from RTC registers.
 *    - Converts the received BCD values into decimal format.
 *    - Stores the result in the user-provided RTC_Time_t structure.
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t where the time values will be stored.
 *
 *  Returns:
 *    RTC_GetTime_Success: Time read successfully.
 *    RTC_GetTime_Failed:  Time read operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_GetTime(RTC_Time_t *time)
{
	RTC_Err_St_t RTC_Err_St = RTC_GetTime_Success;

	uint8_t Time[RTC_TIME_SIZE];

	if(I2C_MemRead(RTC_I2C_NUM,
			       RTC_SLAVE_ADDR,
			       RTC_MEM_SECONDS_ADDR,
			       Time,
			       RTC_TIME_SIZE) != I2C_Read_Success)
	{
		RTC_Err_St = RTC_GetTime_Failed;
	}
	else
	{
		time->seconds = BCDToDec(Time[0] & 0x7F);
		time->minutes = BCDToDec(Time[1]);
		time->hours   = BCDToDec(Time[2] & 0x3F);
	}

	return RTC_Err_St;
}


/******************************************************************************************
 *                                  RTC_GetDate()
 *
 *  Read current date from the RTC device.
 *
 *  Description:
 *    - Reads day, date, month, and year from RTC registers.
 *    - Converts the received BCD values into decimal format.
 *    - Stores the result in the user-provided RTC_Date_t structure.
 *
 *  Parameters:
 *    date: Pointer to RTC_Date_t where the date values will be stored.
 *
 *  Returns:
 *    RTC_GetDate_Success: Date read successfully.
 *    RTC_GetDate_Failed:  Date read operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_GetDate(RTC_Date_t *date)
{
	RTC_Err_St_t RTC_Err_St = RTC_GetDate_Success;

	uint8_t Date[RTC_DATE_SIZE];

	if(I2C_MemRead(RTC_I2C_NUM,
			       RTC_SLAVE_ADDR,
			       RTC_MEM_DAY_ADDR,
			       Date,
			       RTC_DATE_SIZE) != I2C_Read_Success)
	{
		RTC_Err_St = RTC_GetDate_Failed;
	}
	else
	{
		date->Day   = BCDToDec(Date[0]);
		date->Date  = BCDToDec(Date[1]);
		date->month = BCDToDec(Date[2] & 0x1F);
		date->year  = BCDToDec(Date[3]) + 2000;
	}

	return RTC_Err_St;
}
