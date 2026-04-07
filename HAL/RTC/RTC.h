/*
 * RTC.h
 *
 *  Created on: 27 Feb 2026
 *      Author: Ahmed
 */

#ifndef RTC_RTC_H_
#define RTC_RTC_H_


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


typedef struct RTC_Time_s{
	uint8_t seconds; /* 0 - 59*/
	uint8_t minutes; /* 0 - 59*/
	uint8_t hours  ; /*0 - 23*//* 24 format*/
}RTC_Time_t;

typedef struct RTC_Date_s{
	uint8_t Day; /*1 - 7*/
	uint8_t Date; /*1 - 31*/
	uint8_t month; /*1 - 12*/
	uint16_t year; /*0 - 99*/
}RTC_Date_t;


RTC_Err_St_t RTC_Init(void);
RTC_Err_St_t RTC_SetTime(RTC_Time_t *time);
RTC_Err_St_t RTC_SetDate(RTC_Date_t *date);
RTC_Err_St_t RTC_SetTimeDate(RTC_Time_t *time , RTC_Date_t *date);
RTC_Err_St_t RTC_GetTime(RTC_Time_t *time);
RTC_Err_St_t RTC_GetDate(RTC_Date_t *date);


#endif /* RTC_RTC_H_ */
