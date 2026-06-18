/*
 * App.c
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 *
 *  Application layer implementation for the access control system.
 *
 *  This file is responsible for:
 *    - receiving commands from the GUI
 *    - interpreting those commands at application level
 *    - coordinating the lower drivers:
 *        - USART
 *        - RTC
 *        - Fingerprint driver
 *    - building and sending response frames to the GUI
 *    - managing high-level system behavior such as:
 *        - enrollment request handling
 *        - access logging
 *        - enrollment status reporting
 *
 *  Layering rule:
 *  ----------------------------------------------------------------------------
 *  APP layer defines WHAT the system should do.
 *  Driver layers define HOW the hardware actions are performed.
 *
 *  Example:
 *    - APP layer requests enrollment mode
 *    - Fingerprint driver handles the full sensor sequence internally
 *
 *  Architecture:
 *      APP Layer    -> system logic / GUI protocol / coordination
 *      Driver Layer -> hardware handling / low-level state machines
 */

#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_pwr.h>
#include "FreeRTOS.h"
#include "task.h"
#include <USART.h>
#include <RTC.h>
#include <FP.h>
#include "Sys.h"
#include "RELAY.h"
#include "INTERNAL_RTC.h"
#include "App_Cfg.h"
#include "App_Prv.h"
#include "App.h"



/* Application RX state machine current state.
 *
 * This variable tracks the current parser state for frames received from the GUI.
 * The GUI sends a custom application packet format, so the state machine
 * must keep track of which field is expected next.
 *
 * Initial state:
 *   APP_Rx_Wait_Sof
 * meaning:
 *   wait for Start Of Frame first
 */
static App_Rx_St_t  App_Rx_St = APP_Rx_Wait_Sof;

/* Stores the currently received frame from the GUI.
 *
 * Once a full frame is received and validated, this structure contains:
 *   - command
 *   - payload length
 *   - payload bytes
 *
 * The application logic later checks this structure to decide what action to take.
 */
static APP_RxFrame_t APP_RxFrame;


/******************************************************************************************
 *                                  APP_Init()
 *
 *  Initialize the application layer.
 *
 *  Description:
 *    - Initializes all modules required by the application.
 *    - This includes:
 *        - USART driver for GUI communication
 *        - RTC driver for time/date logging
 *        - Fingerprint driver for access control operations
 *
 *  Initialization Order:
 *    1. USART
 *    2. RTC
 *    3. Fingerprint driver
 *
 *  Application Impact:
 *    - The application is considered ready only if all required modules
 *      are initialized successfully.
 *
 *  Returns:
 *    APP_Init_Success: Application initialized successfully.
 *    APP_Init_Failed:  Application initialization failed.
 ******************************************************************************************/
APP_Err_St_t APP_Init(void )
{
	/* Initialize the USART used for communication with the GUI/PC.
	 *
	 * This channel is used by the application protocol and is independent
	 * from the fingerprint driver protocol.
	 */
	USART_Err_St_t USART_Err_St = USART_Init(APP_USART_NUM_);

	/* If GUI communication cannot be initialized,
	 * the application cannot operate correctly.
	 */
	if (USART_Err_St == USART_InitFailed)
	{
		return APP_Init_Failed;
	}

	/* Initialize RTC so access events can be timestamped. */
	RTC_Err_St_t RTC_Err_St = RTC_Init();

	/* If RTC initialization fails, access logs will not have valid timestamps,
	 * so application initialization is considered failed.
	 */
	if(RTC_Err_St == RTC_Init_Failed)
	{
		return APP_Init_Failed;
	}

	/* Initialize fingerprint driver.
	 *
	 * The application depends on this driver for:
	 *   - search mode
	 *   - enroll mode
	 *   - user result retrieval
	 *   - enrollment status retrieval
	 */
	FP_Err_St_t FP_Err_St = FP_Init();

	/* If fingerprint driver initialization fails,
	 * access control functionality is unavailable.
	 */
	if(FP_Err_St == FP_InitFailed)
	{
		return APP_Init_Failed;
	}

	RELAY_Init(RELAY_NUM_1);

	/* All required modules initialized successfully */
	return APP_Init_Success;
}


/******************************************************************************************
 *                              APP_Send_Cmd()
 *
 *  Build and send one application packet to the GUI.
 *
 *  Packet Format:
 *  ----------------------------------------------------------------------------
 *  [0] SOF         : Start Of Frame
 *  [1] CMD         : Command ID / message type
 *  [2] LEN         : Number of payload bytes
 *  [3..N] DATA     : Payload bytes
 *  [Last] CHECKSUM : XOR checksum
 *  ----------------------------------------------------------------------------
 *
 *  Checksum Rule:
 *    CHECKSUM = CMD ^ LEN ^ DATA[0] ^ DATA[1] ^ ... ^ DATA[N]
 *
 *  Description:
 *    - Builds a full application protocol frame.
 *    - Calculates checksum.
 *    - Sends the frame byte-by-byte using USART.
 *
 *  Internal Use:
 *    - Used when sending:
 *        - access logs
 *        - enrollment status updates
 *        - any future application responses to the GUI
 *
 *  Parameters:
 *    APP_CMD_Inst:     Command ID to send.
 *    Data_payload:     Pointer to payload data.
 *    Data_payload_len: Number of payload bytes.
 *
 *  Note:
 *    The current implementation does not return a value even though
 *    the function type is APP_Err_St_t. This can be improved later.
 ******************************************************************************************/
static APP_Err_St_t APP_Send_Cmd(APP_CMD_t APP_CMD_Inst, uint8_t *Data_payload, uint8_t Data_payload_len)
{
	/* Local TX buffer used to build the full outgoing frame */
	uint8_t app_tx_buff[30];

	/* Payload iterator */
	uint8_t indx = 0;

	/* XOR checksum accumulator */
	uint8_t check_sum = 0;

	/* ---------------- Packet Build ---------------- */

	/* Byte 0 = Start Of Frame
	 *
	 * This allows the GUI receiver to detect where a frame starts.
	 */
	app_tx_buff[0] = APP_SOF;

	/* Byte 1 = command */
	app_tx_buff[1] = APP_CMD_Inst;

	/* Byte 2 = payload length */
	app_tx_buff[2] = Data_payload_len;

	/* Start checksum with CMD and LEN.
	 *
	 * SOF is intentionally not included in checksum.
	 */
	check_sum = app_tx_buff[1] ^ app_tx_buff[2];

	/* Copy payload bytes and keep updating checksum */
	for(indx = 0; indx < Data_payload_len; indx++)
	{
		app_tx_buff[3 + indx] = Data_payload[indx];
		check_sum ^= Data_payload[indx];
	}

	/* Store checksum after payload */
	app_tx_buff[3 + Data_payload_len] = check_sum;

	/* ---------------- Packet Transmission ---------------- */

	/* Total transmitted bytes =
	 *   SOF + CMD + LEN + PAYLOAD + CHECKSUM
	 * = 4 + payload_len
	 */
	for(uint8_t i = 0 ; i < 4 + Data_payload_len; i++)
	{
		USART_SendByte(APP_USART_NUM_, app_tx_buff[i]);
	}
}


/******************************************************************************************
 *                              APP_Check_RxFrame()
 *
 *  Receive and parse GUI packets.
 *
 *  Description:
 *    - Implements the RX state machine for the application protocol.
 *    - Reconstructs the incoming GUI frame byte by byte.
 *    - Validates frame checksum before marking it complete.
 *
 *  Expected Frame Format:
 *    [SOF][CMD][LEN][DATA...][CS]
 *
 *  Internal Flow:
 *    1. Wait for SOF
 *    2. Receive CMD
 *    3. Receive LEN
 *    4. Receive DATA bytes
 *    5. Receive CS
 *    6. Verify checksum
 *    7. If valid -> mark frame complete
 *
 *  Note:
 *    - This function only receives and validates the frame.
 *    - Command handling is performed later in APP_Cyclic().
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
static void APP_Check_RxFrame(void)
{
	/* Holds one received USART byte at a time */
	uint8_t rx_byte;

	/* Payload index used while receiving DATA bytes.
	 *
	 * static is required because this function is called cyclically and
	 * a complete frame may arrive over multiple calls.
	 */
	static uint8_t data_indx = 0;

	/* Read all currently available bytes from USART */
	while (USART_ReceiveByte(APP_USART_NUM_, &rx_byte) == USART_Rx_Ok)
	{
		switch (App_Rx_St)
		{
		case APP_Rx_Wait_Sof:
		{
			/* Wait for Start Of Frame.
			 *
			 * Any non-SOF byte is ignored, allowing resynchronization
			 * if noise or corrupted bytes are received.
			 */
			if (rx_byte == APP_SOF)
			{
				/* Valid SOF detected -> next expected byte is CMD */
				App_Rx_St = APP_Rx_Get_Cmd;
			}
			break;
		}

		case APP_Rx_Get_Cmd:
		{
			/* Store command byte */
			APP_RxFrame.cmd = rx_byte;

			/* Next field is payload length */
			App_Rx_St = APP_Rx_Get_Len;
			break;
		}

		case APP_Rx_Get_Len:
		{
			/* Store payload length */
			APP_RxFrame.len = rx_byte;

			/* If length is zero, the next byte will be checksum directly.
			 * Otherwise move to DATA reception.
			 */
			if (APP_RxFrame.len == 0)
			{
				App_Rx_St = APP_Rx_Get_Cs;
			}
			else
			{
				/* Reset payload index before collecting payload */
				data_indx = 0;
				App_Rx_St = APP_Rx_Get_Data;
			}

			break;
		}

		case APP_Rx_Get_Data:
		{
			/* Store payload byte */
			APP_RxFrame.data[data_indx] = rx_byte;

			/* Move to next payload index */
			data_indx++;

			/* Once all payload bytes are received, expect checksum next */
			if (data_indx >= APP_RxFrame.len)
			{
				App_Rx_St = APP_Rx_Get_Cs;
			}

			break;
		}

		case APP_Rx_Get_Cs:
		{
			/* Received checksum byte */
			uint8_t received_cs = rx_byte;

			/* Recalculate checksum locally */
			uint8_t calc_cs = APP_RxFrame.cmd ^ APP_RxFrame.len;

			/* Include payload bytes in checksum calculation */
			for (uint8_t i = 0; i < APP_RxFrame.len; i++)
			{
				calc_cs ^= APP_RxFrame.data[i];
			}

			/* Compare local checksum with received checksum */
			if (received_cs == calc_cs)
			{
				/* Frame is valid and ready for application processing */
				App_Rx_St = APP_Rx_Complete_Frame;
			}
			else
			{
				/* Invalid checksum -> discard frame and restart synchronization */
				App_Rx_St = APP_Rx_Wait_Sof;
			}

			break;
		}

		case APP_Rx_Complete_Frame:
		{
			/* A full valid frame is already available.
			 *
			 * Do not overwrite it until APP_Check_Response() consumes it
			 * and resets the RX state machine.
			 */
			break;
		}
		}
	}
}


/******************************************************************************************
 *                              APP_Check_Response()
 *
 *  Check whether a full valid GUI frame is available.
 *
 *  Description:
 *    - Evaluates the application RX state machine.
 *    - Detects whether a complete and checksum-verified GUI frame is ready.
 *    - Resets the RX state machine for the next frame.
 *
 *  Usage:
 *    - Used by APP_Cyclic() after APP_Check_RxFrame().
 *
 *  Returns:
 *    APP_Rx_Full_Packet_ok:  A full valid frame is available.
 *    APP_Rx_Full_Packet_Nok: No complete valid frame is available.
 ******************************************************************************************/
static APP_Err_St_t APP_Check_Response(void)
{
	/* Default assumption: no complete frame available */
	APP_Err_St_t APP_Err_St = APP_Rx_Full_Packet_Nok;

	/* If RX state machine reached complete-frame state,
	 * a full valid frame is ready.
	 */
	if(App_Rx_St == APP_Rx_Complete_Frame)
	{
		APP_Err_St = APP_Rx_Full_Packet_ok;

		/* Reset RX state for next incoming GUI frame */
		App_Rx_St = APP_Rx_Wait_Sof;
	}

	return APP_Err_St;
}


/******************************************************************************************
 *                                  APP_Cyclic()
 *
 *  Main periodic function of the application layer.
 *
 *  Description:
 *    - Acts as the central execution point of the application layer.
 *    - Coordinates the GUI protocol, fingerprint driver, and RTC driver.
 *
 *  Execution Flow:
 *    1. Receive and validate GUI frames.
 *    2. Process GUI commands.
 *    3. If in search mode:
 *         - check for fingerprint result
 *         - collect date/time
 *         - send access log to GUI
 *    4. If in enroll mode:
 *         - read current enrollment instruction
 *         - send status update to GUI when changed
 *
 *  Integration:
 *    - Must be called periodically.
 *    - Can be used in:
 *        - super loop
 *        - scheduler
 *        - RTOS task
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void APP_Cyclic(void)
{



	/* Match result returned by fingerprint driver */
	uint8_t match_st;

	/* Matched user ID returned by fingerprint driver */
	uint16_t user_id;

	/* User ID to send to GUI.
	 *
	 * If no match is found, 0 is sent.
	 */
	uint16_t id_to_send;

	/* RTC structures used to timestamp access events */
	RTC_Time_t time;
	RTC_Date_t date;

	/* Last enrollment instruction sent to GUI.
	 *
	 * Used to avoid sending duplicate status frames continuously.
	 */
	static FP_GetEnroll_Instruction_t prev_inst = FP_E_Inst_Idle;


	RTC_GetTime(&time);

	APP_HandleSleepMode(&time);




	/* ------------------------------------------------------------------
	 * Step 1: Receive and validate GUI frames
	 * ------------------------------------------------------------------
	 * 	APP_HandleSleepMode();
	 */
	APP_Check_RxFrame();

	/* If a full valid GUI frame is available, process it */
	if(APP_Check_Response() == APP_Rx_Full_Packet_ok)
	{
		/* Handle enrollment request from GUI */
		if(APP_RxFrame.cmd == APP_Enroll_Req)
		{
			/* Switch fingerprint driver to enrollment mode.
			 *
			 * The fingerprint driver will handle the full enrollment flow internally.
			 */
			FP_SetMode(FP_ENROLL_MODE);
		}
	}

	/* ------------------------------------------------------------------
	 * Step 2: Behavior in fingerprint search mode
	 * ------------------------------------------------------------------
	 *
	 * In this mode, the fingerprint driver continuously searches for users.
	 * If a result is available, the application logs it to the GUI.
	 */
	if(FP_GetMode() == FP_SEARCH_MODE)
	{
		/* Payload sent to GUI as access log:
		 * [0] access state
		 * [1] user ID high
		 * [2] user ID low
		 * [3] hour
		 * [4] minute
		 * [5] second
		 * [6] day
		 * [7] month
		 * [8] year high
		 * [9] year low
		 */
		uint8_t user_payload[10];

		/* Try to retrieve one search result from fingerprint driver */
		if(FP_Get_User(&match_st, &user_id) == FP_GetUser_Ok)
		{
			/* Get current date and time for logging */
			RTC_GetDate(&date);
			RTC_GetTime(&time);

			/* Byte 0 = access state
			 * granted if match found, otherwise denied
			 */
			user_payload[0] = ((match_st == FP_MATCH_ST) ? APP_USER_GRANTED : APP_USER_DENIED);

			/* If access granted -> send actual matched ID
			 * If access denied  -> send 0
			 */
			id_to_send = ((match_st == FP_MATCH_ST) ? user_id : 0);

			/* Split 16-bit user ID into high and low bytes */
			user_payload[1] = (id_to_send >> 8);
			user_payload[2] = (id_to_send & 0xFF);

			/* Add current time */
			user_payload[3] = time.hours;
			user_payload[4] = time.minutes;
			user_payload[5] = time.seconds;

			/* Add current date */
			user_payload[6] = date.Day;
			user_payload[7] = date.month;
			user_payload[8] = date.year >> 8;
			user_payload[9] = date.year & 0xFF;

			/* Send access log to GUI */
			APP_Send_Cmd(APP_Log_Access, user_payload, APP_LOG_ACCESS_DATA_LEN);

			// Relay On
			if(match_st == FP_MATCH_ST)
			{
				RELAY_On_With_Time(RELAY_NUM_1, 5000);
			}
			/* Reset previous enrollment instruction.
			 *
			 * This ensures that if the system enters enrollment mode later,
			 * the first enrollment status will be sent correctly.
			 */
			prev_inst = FP_E_Inst_Idle;

			/* Future extension:
			 * If access granted, a relay or door control action can be triggered here.
			 */
		}
	}

	/* ------------------------------------------------------------------
	 * Step 3: Behavior in fingerprint enroll mode
	 * ------------------------------------------------------------------
	 *
	 * In this mode, the application does not manage the low-level steps.
	 * It only queries the fingerprint driver for the current high-level
	 * instruction/status and forwards it to the GUI.
	 */
	else if(FP_GetMode() == FP_ENROLL_MODE)
	{
		/* Read current enrollment instruction from fingerprint driver */
		FP_GetEnroll_Instruction_t current_inst = FP_GetEnroll_Instruction();

		/* Send enrollment status only if it changed.
		 *
		 * This avoids repeatedly sending the same status every cycle.
		 */
		if(current_inst != prev_inst)
		{

			uint8_t enroll_success_payload[APP_ENROLL_SUCCESS_PAYLOAD_LEN] ;
			if(current_inst == FP_E_Inst_Success)
			{

				uint16_t curr_user_id = FP_Get_Curr_User_Id();
				enroll_success_payload[0] = current_inst;
				enroll_success_payload[1] =  (curr_user_id >> 8);
				enroll_success_payload[2] = (curr_user_id & 0xFF) ;

				/* Send updated enrollment status to GUI */
				APP_Send_Cmd(APP_Enroll_St, enroll_success_payload, APP_ENROLL_SUCCESS_PAYLOAD_LEN);
			}
			else{
				/* Send updated enrollment status to GUI */
				enroll_success_payload[0] = current_inst;
				enroll_success_payload[1] =  0;
				enroll_success_payload[2] = 0;
				APP_Send_Cmd(APP_Enroll_St, enroll_success_payload, APP_ENROLL_INST_LEN);
			}


			/* Save it as the last transmitted instruction */
			prev_inst = current_inst;
		}
	}
}


void APP_HandleSleepMode(RTC_Time_t *time)
{
    static uint8_t exti_awake_mode = 0;
    static TickType_t exti_start_tick = 0;
    static uint8_t has_slept_today = 0;

    if(INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag())
    {
    	    exti_awake_mode = 1;
    	    exti_start_tick = xTaskGetTickCount();

    	    APP_SendSleepStatus(APP_SYSTEM_AWAKE);
    }

    if(exti_awake_mode)
    {
        if((xTaskGetTickCount() - exti_start_tick) >= pdMS_TO_TICKS(10UL * 1000UL))
        {
            exti_awake_mode = 0;

            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepModeOnly();
        }
    }

    if(time->hours == 0 && time->minutes == 25)
    {
        if(has_slept_today == 0)
        {
            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepMode(0, 26);

            APP_SendSleepStatus(APP_SYSTEM_AWAKE);

            has_slept_today = 1;
        }
    }
    else if(time->hours == 6 && time->minutes == 1)
    {
        if(has_slept_today == 0)
        {
            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepMode(6, 59);

            APP_SendSleepStatus(APP_SYSTEM_AWAKE);

            has_slept_today = 1;
        }
    }
    else
    {
        has_slept_today = 0;
    }
}

void APP_SendSleepStatus(uint8_t state)
{
    uint8_t payload[1];

    payload[0] = state;

    APP_Send_Cmd(APP_Sleep_St, payload, 1);
}
