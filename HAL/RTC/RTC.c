/******************************************************************************
 * @file    RTC.c
 * @author  Ahmed Abdelrhman
 * @brief   Implementation file for External RTC Driver.
 *
 * @project Fingerprint Access Control System - STM32F407
 * @note    Final GitHub-ready cleanup: comments, spacing, and readability only.
 *          Application behavior and logic are intentionally unchanged.
 ******************************************************************************/

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
 *    - use MCAL I2C interface for the RTC device
 *    - check RTC device availability
 *    - write time values to RTC registers
 *    - write date values to RTC registers
 *    - read time values from RTC registers
 *    - read date values from RTC registers
 *
 *  Communication:
 *    - RTC communication is performed over I2C3
 *    - HAL I2C APIs are used for memory read/write operations
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

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

#include "stm32f4xx_hal_i2c.h"
#include "stm32f4xx_hal_i2c_ex.h"
#include "I2C.h"

#include "RTC_Prv.h"
#include "RTC.h"

/******************************************************************************************
 *                                  RTC_Init()
 *
 *  Initialize the RTC driver.
 *
 *  Description:
 *    - Initializes MCAL I2C driver.
 *    - Checks whether the RTC device is present and ready on the I2C bus.
 *
 *  Internal Behavior:
 *    - Calls I2C_Init() to configure the I2C peripheral and GPIO pins.
 *    - Uses MCAL I2C device-ready wrapper to verify communication with the RTC.
 *
 *  Returns:
 *    RTC_Init_Success: RTC initialized and device is reachable.
 *    RTC_Init_Failed:  RTC initialization failed or device not detected.
 ******************************************************************************************/
RTC_Err_St_t RTC_Init(void)
{
    /* Default return assumes initialization will succeed */
    RTC_Err_St_t RTC_Err_St = RTC_Init_Success;

    /* HAL status used to evaluate I2C operation */
    HAL_StatusTypeDef I2C_St;

    /* Initialize MCAL I2C driver */
    I2C_Init();

    /* Check whether RTC slave is responding on the I2C bus */
    I2C_St = I2C_IsDeviceReady(RTC_SLAVE_ADDR, RTC_I2C_DEVICE_READY_TRIAL, RTC_I2C_TIMEOUT);

    if (I2C_St == HAL_OK)
    {
        RTC_Err_St = RTC_Init_Success;
    }
    else
    {
        RTC_Err_St = RTC_Init_Failed;
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
    /* Default return assumes time setting will succeed */
    RTC_Err_St_t RTC_Err_St = RTC_SetTime_Success;

    /* Local buffer matching RTC time register layout */
    uint8_t Time[RTC_TIME_SIZE];

    /* Convert decimal values from application format into BCD format
     *
     * Seconds register:
     *   bit 7 is masked with 0x7F to keep CH/control bit cleared.
     */
    Time[0] = DecToBCD(time->seconds) & 0x7F;
    Time[1] = DecToBCD(time->minutes);
    Time[2] = DecToBCD(time->hours);

    /* Write time registers starting from seconds register */
    HAL_StatusTypeDef i2c_st =
            I2C_Mem_Write(RTC_SLAVE_ADDR, RTC_MEM_SECONDS_ADDR, I2C_MEMADD_SIZE_8BIT, Time, RTC_TIME_SIZE, RTC_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
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
    /* Default return assumes date setting will succeed */
    RTC_Err_St_t RTC_Err_St = RTC_SetDate_Success;

    /* Local buffer matching RTC date register layout */
    uint8_t Date[RTC_DATE_SIZE];

    /* Convert application values to BCD format before writing to RTC */
    Date[0] = DecToBCD(date->Day);
    Date[1] = DecToBCD(date->Date);
    Date[2] = DecToBCD(date->month);

    /* If application uses full year value such as 2026,
     * only the last two digits are stored in the RTC register.
     */
    Date[3] = DecToBCD((uint8_t)(date->year % 100));

    /* Write date registers starting from day register */
    HAL_StatusTypeDef i2c_st =
            I2C_Mem_Write(RTC_SLAVE_ADDR, RTC_MEM_DAY_ADDR, I2C_MEMADD_SIZE_8BIT, Date, RTC_DATE_SIZE, RTC_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
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
 *  Application Use:
 *    Useful during initial RTC configuration when both time and date
 *    need to be programmed together.
 *
 *  Returns:
 *    RTC_SetTime_Success: Both time and date were written successfully.
 *    RTC_SetTime_Failed:  Failed while setting time.
 *    RTC_SetDate_Failed:  Failed while setting date after time succeeded.
 *
 *  Note:
 *    Return type is RTC_Err_St_t, so the function returns whichever
 *    failure occurred first, or success if both operations succeeded.
 ******************************************************************************************/
RTC_Err_St_t RTC_SetTimeDate(RTC_Time_t *time, RTC_Date_t *date)
{
    /* First set time */
    RTC_Err_St_t rtc_err_st = RTC_SetTime(time);

    if (rtc_err_st != RTC_SetTime_Success)
    {
        return RTC_SetTime_Failed;
    }

    /* Then set date */
    rtc_err_st = RTC_SetDate(date);

    if (rtc_err_st != RTC_SetDate_Success)
    {
        return RTC_SetDate_Failed;
    }

    /* Both operations succeeded */
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
 *  Internal Behavior:
 *    - Reads 3 bytes starting from RTC seconds register.
 *    - Converts each field from BCD to decimal.
 *
 *  Returns:
 *    RTC_GetTime_Success: Time read successfully.
 *    RTC_GetTime_Failed:  Time read operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_GetTime(RTC_Time_t *time)
{
    /* Default return assumes time read will succeed */
    RTC_Err_St_t RTC_Err_St = RTC_GetTime_Success;

    /* Local buffer receiving raw RTC register values */
    uint8_t Time[RTC_TIME_SIZE];

    /* Read time registers starting from seconds register */
    HAL_StatusTypeDef i2c_st = I2C_Mem_Read(RTC_SLAVE_ADDR, RTC_MEM_SECONDS_ADDR, I2C_MEMADD_SIZE_8BIT, Time, RTC_TIME_SIZE, RTC_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        /* Read failed */
        RTC_Err_St = RTC_GetTime_Failed;
    }
    else
    {
        /* Convert seconds from BCD to decimal.
         * Mask with 0x7F first to ignore CH/control bit if present.
         */
        time->seconds = BCDToDec(Time[0] & 0x7F);

        /* Convert minutes from BCD to decimal */
        time->minutes = BCDToDec(Time[1]);

        /* Convert hours from BCD to decimal.
         * Mask with 0x3F to keep only hour bits in 24-hour format.
         */
        time->hours = BCDToDec(Time[2] & 0x3F);
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
 *  Internal Behavior:
 *    - Reads 4 bytes starting from RTC day register.
 *    - Converts each field from BCD to decimal.
 *
 *  Returns:
 *    RTC_GetDate_Success: Date read successfully.
 *    RTC_GetDate_Failed:  Date read operation failed.
 ******************************************************************************************/
RTC_Err_St_t RTC_GetDate(RTC_Date_t *date)
{
    /* Default return assumes date read will succeed */
    RTC_Err_St_t RTC_Err_St = RTC_GetDate_Success;

    /* Local buffer receiving raw RTC register values */
    uint8_t Date[RTC_DATE_SIZE];

    /* Read date registers starting from day register */
    HAL_StatusTypeDef i2c_st =
            I2C_Mem_Read(RTC_SLAVE_ADDR, RTC_MEM_DAY_ADDR, I2C_MEMADD_SIZE_8BIT, Date, RTC_DATE_SIZE, RTC_MAX_TIMEOUT);

    if (i2c_st != HAL_OK)
    {
        RTC_Err_St = RTC_GetDate_Failed;
    }
    else
    {
        /* Convert day-of-week from BCD to decimal */
        date->Day = BCDToDec(Date[0]);

        /* Convert day-of-month from BCD to decimal */
        date->Date = BCDToDec(Date[1]);

        /* Convert month from BCD to decimal.
         * Mask with 0x1F to keep month bits only.
         */
        date->month = BCDToDec(Date[2] & 0x1F);

        /* Convert two-digit RTC year into full year */
        date->year = BCDToDec(Date[3]) + 2000;
    }

    return RTC_Err_St;
}
