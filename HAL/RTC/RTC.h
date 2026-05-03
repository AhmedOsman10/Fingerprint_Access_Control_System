/*
 * RTC.h
 *
 *  Created on: 27 Feb 2026
 *      Author: Ahmed
 *
 *  Public API for the RTC (Real-Time Clock) driver.
 *
 *  This module provides time and date services for the system.
 *
 *  Typical use:
 *    - timestamping events (e.g., access logs)
 *    - retrieving current system time
 *
 *  Integration:
 *    - must be initialized once using RTC_Init()
 *    - APIs can be called from:
 *        - application layer
 *        - tasks
 *        - periodic functions
 */

#ifndef RTC_RTC_H_
#define RTC_RTC_H_


/******************************************************************************************
 *                                  RTC_Err_St_t
 *
 *  Return type used by RTC APIs.
 *
 *  Values:
 *    RTC_Init_Success:     RTC initialized successfully.
 *    RTC_Init_Failed:      RTC initialization failed.
 *
 *    RTC_SetTime_Success:  Time set successfully.
 *    RTC_SetTime_Failed:   Failed to set time.
 *
 *    RTC_GetTime_Success:  Time retrieved successfully.
 *    RTC_GetTime_Failed:   Failed to get time.
 *
 *    RTC_GetDate_Success:  Date retrieved successfully.
 *    RTC_GetDate_Failed:   Failed to get date.
 *
 *    RTC_SetDate_Success:  Date set successfully.
 *    RTC_SetDate_Failed:   Failed to set date.
 ******************************************************************************************/
typedef enum RTC_Err_St_e
{
	RTC_Init_Success,
	RTC_Init_Failed,

	RTC_SetTime_Success,
	RTC_SetTime_Failed,

	RTC_GetTime_Success,
	RTC_GetTime_Failed,

	RTC_GetDate_Success,
	RTC_GetDate_Failed,

	RTC_SetDate_Success,
	RTC_SetDate_Failed,
}RTC_Err_St_t;


/******************************************************************************************
 *                                  RTC_Time_t
 *
 *  Time structure (24-hour format).
 *
 *  Fields:
 *    seconds: 0 - 59
 *    minutes: 0 - 59
 *    hours  : 0 - 23
 *
 *  Application Use:
 *    Used with RTC_SetTime() and RTC_GetTime().
 ******************************************************************************************/
typedef struct RTC_Time_s{
	uint8_t seconds; /* 0 - 59 */
	uint8_t minutes; /* 0 - 59 */
	uint8_t hours;   /* 0 - 23 */
}RTC_Time_t;


/******************************************************************************************
 *                                  RTC_Date_t
 *
 *  Date structure.
 *
 *  Fields:
 *    Day  : 1 - 7   (day of week)
 *    Date : 1 - 31  (day of month)
 *    month: 1 - 12
 *    year : full year value (e.g., 2026)
 *
 *  Application Use:
 *    Used with RTC_SetDate() and RTC_GetDate().
 ******************************************************************************************/
typedef struct RTC_Date_s{
	uint8_t Day;    /* 1 - 7 */
	uint8_t Date;   /* 1 - 31 */
	uint8_t month;  /* 1 - 12 */
	uint16_t year;  /* e.g., 2026 */
}RTC_Date_t;


/******************************************************************************************
 *                                  RTC_Init()
 *
 *  Initialize the RTC peripheral.
 *
 *  Application Use:
 *    Call once during system initialization.
 *
 *  Example:
 *    if(RTC_Init() != RTC_Init_Success)
 *    {
 *        // handle error
 *    }
 *
 *  Returns:
 *    RTC_Init_Success / RTC_Init_Failed
 ******************************************************************************************/
RTC_Err_St_t RTC_Init(void);


/******************************************************************************************
 *                                  RTC_SetTime()
 *
 *  Set current time.
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t.
 *
 *  Application Use:
 *    RTC_Time_t t = {30, 15, 10};
 *    RTC_SetTime(&t);
 *
 *  Returns:
 *    RTC_SetTime_Success / RTC_SetTime_Failed
 ******************************************************************************************/
RTC_Err_St_t RTC_SetTime(RTC_Time_t *time);


/******************************************************************************************
 *                                  RTC_SetDate()
 *
 *  Set current date.
 *
 *  Parameters:
 *    date: Pointer to RTC_Date_t.
 *
 *  Returns:
 *    RTC_SetDate_Success / RTC_SetDate_Failed
 ******************************************************************************************/
RTC_Err_St_t RTC_SetDate(RTC_Date_t *date);


/******************************************************************************************
 *                                  RTC_SetTimeDate()
 *
 *  Set both time and date.
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t.
 *    date: Pointer to RTC_Date_t.
 *
 *  Application Use:
 *    Used during initial RTC configuration.
 *
 *  Returns:
 *    Combined result of time/date setting.
 ******************************************************************************************/
RTC_Err_St_t RTC_SetTimeDate(RTC_Time_t *time, RTC_Date_t *date);


/******************************************************************************************
 *                                  RTC_GetTime()
 *
 *  Get current time.
 *
 *  Parameters:
 *    time: Pointer to RTC_Time_t to store result.
 *
 *  Example:
 *    RTC_Time_t t;
 *    RTC_GetTime(&t);
 *
 *  Returns:
 *    RTC_GetTime_Success / RTC_GetTime_Failed
 ******************************************************************************************/
RTC_Err_St_t RTC_GetTime(RTC_Time_t *time);


/******************************************************************************************
 *                                  RTC_GetDate()
 *
 *  Get current date.
 *
 *  Parameters:
 *    date: Pointer to RTC_Date_t to store result.
 *
 *  Returns:
 *    RTC_GetDate_Success / RTC_GetDate_Failed
 ******************************************************************************************/
RTC_Err_St_t RTC_GetDate(RTC_Date_t *date);


#endif /* RTC_RTC_H_ */
