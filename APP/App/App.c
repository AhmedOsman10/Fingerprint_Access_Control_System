/*
 * App.c
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */


#include <stdio.h>
#include <stdint.h>
#include <USART.h>
#include <RTC.h>
#include <FP.h>

#include "App_Cfg.h"
#include "App_Prv.h"
#include "App.h"

APP_Err_St_t APP_Init(void )
{
	USART_Err_St_t USART_Err_St = USART_Init(APP_USART_NUM_);
	if (USART_Err_St == USART_InitFailed)
	{
		return APP_Init_Failed;
	}

	RTC_Err_St_t RTC_Err_St = RTC_Init();
	if(RTC_Err_St == RTC_Init_Failed)
	{
		return APP_Init_Failed;

	}

	FP_Err_St_t FP_Err_St = FP_Init();

	if(FP_Err_St == FP_InitFailed)
	{
		return APP_Init_Failed;
	}

	return APP_Init_Success;
}

static APP_Err_St_t APP_Send_Cmd(APP_CMD_t APP_CMD_Inst , uint8_t *Data_payload , uint8_t Data_payload_len)
{
	uint8_t app_tx_buff[30];
	uint8_t indx = 0;
	uint8_t check_sum = 0;
	app_tx_buff[0] = APP_SOF;
	app_tx_buff[1] = APP_CMD_Inst;
	app_tx_buff[2] = Data_payload_len;

	checksum = app_tx_buff[1] ^ app_tx_buff[2];
	for(indx = 0 ; indx <Data_payload_len ; indx++ )
	{
		app_tx_buff[3 + indx] = Data_payload[indx];
		checksum ^= Data_payload[indx];
	}
	app_tx_buff[3 + Data_payload_len] = check_sum;

	for(uint8_t i = 0 ; i < 4 + Data_payload_len; i++)
	{
		USART_SendByte(APP_USART_NUM_, app_tx_buff[i]);
	}
}

void APP_Cylic(void)
{

	uint8_t match_st;
	uint16_t user_id;
	uint16_t id_to_send;
	RTC_Time_t time;
	RTC_Date_t date;




	// search mode
	// if the user access the finger ===> send the log access data
	if(FP_GetMode() == FP_SEARCH_MODE)
	{

		RTC_GetDate(&date);
		RTC_GetTime(&time);

		uint8_t user_payload[10];
		if(FP_Get_User(&match_st,&user_id) == FP_GetUser_Ok)
		{


			// send match state (granted or denied )
			user_payload[0] = ((match_st == FP_MATCH_ST)? APP_USER_GRANTED : APP_USER_DENIED);

			// send the user id (2 bytes ==> High byte , Low byte )
			id_to_send = ((match_st == FP_MATCH_ST)? user_id : 0);
			user_payload[1] =( id_to_send >> 8);
			user_payload[2] =( id_to_send & 0xFF);

			// Send the the time and the date
			user_payload[3] = time.hours;
			user_payload[4] = time.minutes;
			user_payload[5] = time.seconds;


			user_payload[6] = date.Day;
			user_payload[7] = date.month;
			user_payload[8] = date.year >> 8;
			user_payload[8] = date.year & 0xFF;



			APP_Send_Cmd(APP_Log_Access ,user_payload ,APP_LOG_ACCESS_DATA_LEN);

			// If the user grated turn on the relay
		}

	}
	else if (FP_GetMode() == FP_ENROLL_MODE)
	{

	}
}
