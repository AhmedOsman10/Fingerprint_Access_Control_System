/*
 * App.c
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 *
 *  This file represents the Application Layer of the system.
 *
 *  Main Responsibilities:
 *  ----------------------------------------------------------------------------
 *  1. Receive commands from the GUI/PC through USART
 *  2. Interpret those commands and trigger system actions
 *  3. Interact with lower-level drivers such as:
 *        - USART driver
 *        - RTC driver
 *        - Fingerprint driver
 *  4. Build response packets and send them back to the GUI
 *  5. Manage high-level system behavior such as:
 *        - fingerprint enrollment request
 *        - access logging
 *        - enrollment status updates
 *
 *  Important Design Idea:
 *  ----------------------------------------------------------------------------
 *  This file should focus on WHAT the system should do,
 *  while the lower drivers focus on HOW it is done.
 *
 *  Example:
 *  - App layer says: "Start enrollment"
 *  - FP driver handles the full fingerprint module sequence internally
 *
 *  This is clean layered architecture:
 *      APP Layer  -> system logic / decisions / protocol with GUI
 *      Driver Layer -> hardware handling / low-level state machines
 */

#include <stdio.h>
#include <stdint.h>
#include <USART.h>
#include <RTC.h>
#include <FP.h>

#include "App_Cfg.h"
#include "App_Prv.h"
#include "App.h"


/* Application RX state machine current state.
 *
 * This state machine is responsible for receiving frames coming from the GUI.
 * The GUI sends data using a custom packet format, so this variable tracks
 * which byte/field we are currently expecting.
 *
 * Initial state:
 *   APP_Rx_Wait_Sof
 * meaning:
 *   wait for Start Of Frame first
 */
static App_Rx_St_t  App_Rx_St = APP_Rx_Wait_Sof;

/* This structure stores the currently received frame from the GUI.
 *
 * Once a full packet is received and checksum is verified,
 * this structure contains:
 *   - command
 *   - length
 *   - payload data
 *
 * Then the application layer can inspect it and decide what to do.
 */
static APP_RxFrame_t APP_RxFrame;


/******************************************************************************************
 *                                  APP_Init()
 *
 * Purpose:
 *  Initializes all modules required by the application.
 *
 * Description:
 *  This function is the top-level initialization for the whole access control
 *  application. It initializes all drivers that the App layer depends on.
 *
 * Initialization order:
 *  1. USART -> because GUI communication depends on it
 *  2. RTC   -> because access log timestamps depend on it
 *  3. FP    -> because fingerprint functions depend on it
 *
 * Why this function is important:
 *  - The application must not run unless all required modules are ready.
 *  - If even one dependency fails, the system is considered not ready.
 *
 * Returns:
 *  APP_Init_Success : all required modules initialized correctly
 *  APP_Init_Failed  : at least one module failed during initialization
 ******************************************************************************************/
APP_Err_St_t APP_Init(void )
{
	/* Initialize USART used by the application-to-GUI protocol.
	 *
	 * This USART is different from the fingerprint internal logic view.
	 * Here, the App layer uses it as the communication channel with the PC/GUI.
	 */
	USART_Err_St_t USART_Err_St = USART_Init(APP_USART_NUM_);

	/* If USART initialization fails, the App cannot communicate with the GUI,
	 * so there is no point continuing initialization.
	 */
	if (USART_Err_St == USART_InitFailed)
	{
		return APP_Init_Failed;
	}

	/* Initialize RTC so access events can be timestamped with date and time.
	 *
	 * This is important because when a user is granted/denied access,
	 * the GUI should receive the event with a valid timestamp.
	 */
	RTC_Err_St_t RTC_Err_St = RTC_Init();

	/* If RTC fails, the system loses logging accuracy,
	 * so application init is considered failed.
	 */
	if(RTC_Err_St == RTC_Init_Failed)
	{
		return APP_Init_Failed;

	}

	/* Initialize the fingerprint driver.
	 *
	 * The application depends heavily on FP driver:
	 *   - search mode
	 *   - enroll mode
	 *   - get user result
	 *   - get enroll instructions
	 */
	FP_Err_St_t FP_Err_St = FP_Init();

	/* If fingerprint driver fails, the access control feature itself cannot work. */
	if(FP_Err_St == FP_InitFailed)
	{
		return APP_Init_Failed;
	}

	/* All required modules initialized successfully. */
	return APP_Init_Success;
}


/******************************************************************************************
 *                              APP_Send_Cmd()
 *
 * Purpose:
 *  Builds and sends one application packet to the GUI.
 *
 * Packet Format:
 *  -----------------------------------------------------------------
 *  [0] SOF         : Start of frame
 *  [1] CMD         : Command ID / message type
 *  [2] LEN         : Number of payload bytes
 *  [3..N] DATA     : Payload bytes
 *  [Last] CHECKSUM : XOR checksum
 *  -----------------------------------------------------------------
 *
 * Description:
 *  This function is the transmit-side packet builder for the application protocol.
 *  It takes a command and optional payload, builds a complete frame,
 *  calculates checksum, then sends the frame byte-by-byte using USART.
 *
 * Checksum Rule:
 *  CHECKSUM = CMD ^ LEN ^ DATA[0] ^ DATA[1] ^ ... ^ DATA[N]
 *
 * Why XOR checksum?
 *  - Simple
 *  - Fast
 *  - Good enough for a small custom protocol
 *
 * Notes:
 *  - This is an App-layer protocol, separate from the fingerprint protocol.
 *  - Keeping protocols separate is good design.
 *
 * Parameters:
 *  APP_CMD_Inst     : command to send to the GUI
 *  Data_payload     : pointer to payload bytes
 *  Data_payload_len : number of payload bytes
 *
 * Returns:
 *  APP_Err_St_t
 *
 * Note:
 *  The current implementation does not actually return a value,
 *  even though the function type says APP_Err_St_t.
 *  This should be fixed later.
 ******************************************************************************************/
static APP_Err_St_t APP_Send_Cmd(APP_CMD_t APP_CMD_Inst, uint8_t *Data_payload, uint8_t Data_payload_len)
{
	/* Local TX buffer that holds the full application frame before sending */
	uint8_t app_tx_buff[30];

	/* Used to iterate through payload bytes */
	uint8_t indx = 0;

	/* Checksum accumulator */
	uint8_t check_sum = 0;

	/* ---------------- Packet Build Start ---------------- */

	/* Byte 0 = Start Of Frame
	 * This byte helps the receiver know where a new packet begins.
	 */
	app_tx_buff[0] = APP_SOF;

	/* Byte 1 = command */
	app_tx_buff[1] = APP_CMD_Inst;

	/* Byte 2 = payload length */
	app_tx_buff[2] = Data_payload_len;

	/* Start checksum using CMD and LEN.
	 * SOF is not included in checksum.
	 */
	check_sum = app_tx_buff[1] ^ app_tx_buff[2];

	/* Copy payload bytes and keep updating checksum. */
	for(indx = 0; indx < Data_payload_len; indx++ )
	{
		app_tx_buff[3 + indx] = Data_payload[indx];
		check_sum ^= Data_payload[indx];
	}

	/* Store checksum at the end of the frame */
	app_tx_buff[3 + Data_payload_len] = check_sum;

	/* ---------------- Packet Transmission ---------------- */

	/* Send the full frame byte by byte.
	 *
	 * Total frame bytes:
	 *   SOF + CMD + LEN + PAYLOAD + CS
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
 * Purpose:
 *  Receives and parses incoming packets from the GUI.
 *
 * Description:
 *  This function is the receive-side state machine for the App protocol.
 *  It reads bytes from USART one by one and reconstructs a full frame.
 *
 * Frame format expected:
 *  [SOF][CMD][LEN][DATA...][CS]
 *
 * State Machine Steps:
 *  1. Wait for SOF
 *  2. Receive CMD
 *  3. Receive LEN
 *  4. Receive DATA bytes (if LEN > 0)
 *  5. Receive CS
 *  6. Validate checksum
 *  7. If valid -> mark frame complete
 *     else     -> discard and restart
 *
 * Why a state machine?
 *  - UART data comes byte by byte
 *  - We should not assume the whole frame arrives at once
 *  - State machine is clean, robust, and non-blocking
 *
 * Why while(USART_ReceiveByte(...) == USART_Rx_Ok)?
 *  - Because there may be multiple bytes already waiting in the RX buffer
 *  - So this function tries to consume all available bytes in this cycle
 *
 * Important:
 *  This function does NOT directly process commands.
 *  It only reconstructs and validates the frame.
 *  Actual command handling is done later in APP_Cylic().
 ******************************************************************************************/
static void APP_Check_RxFrame(void)
{
    /* Stores one received byte from USART each iteration */
    uint8_t rx_byte;

    /* Index used while receiving payload data bytes.
     *
     * static is important here because the function is called cyclically.
     * If only part of a frame arrives now, and the rest arrives later,
     * we must remember how many data bytes were already stored.
     */
    static uint8_t data_indx = 0;

    /* Keep reading bytes as long as USART says data is available */
    while (USART_ReceiveByte(APP_USART_NUM_, &rx_byte) == USART_Rx_Ok)
    {
        switch (App_Rx_St)
        {
            case APP_Rx_Wait_Sof:
            {
                /* State: waiting for Start Of Frame.
                 *
                 * Any byte that is not SOF is ignored.
                 * This allows resynchronization if noise or corrupted bytes appear.
                 */
                if (rx_byte == APP_SOF)
                {
                    /* Valid start detected -> next byte should be CMD */
                    App_Rx_St = APP_Rx_Get_Cmd;
                }
                break;
            }

            case APP_Rx_Get_Cmd:
            {
                /* Store received command byte */
                APP_RxFrame.cmd = rx_byte;

                /* Next expected field is LEN */
                App_Rx_St = APP_Rx_Get_Len;
                break;
            }

            case APP_Rx_Get_Len:
            {
                /* Store payload length */
                APP_RxFrame.len = rx_byte;

                /* If LEN = 0, there is no data field,
                 * so the next byte must be checksum directly.
                 */
                if (APP_RxFrame.len == 0)
                {
                    App_Rx_St = APP_Rx_Get_Cs;
                }
                else
                {
                    /* Reset payload index before starting to store data bytes */
                    data_indx = 0;

                    /* Next state: collect payload bytes */
                    App_Rx_St = APP_Rx_Get_Data;
                }

                break;
            }

            case APP_Rx_Get_Data:
            {
                /* Store payload byte into frame data array */
                APP_RxFrame.data[data_indx] = rx_byte;

                /* Move to next payload index */
                data_indx++;

                /* Once all expected payload bytes are received,
                 * next expected byte becomes checksum.
                 */
                if (data_indx >= APP_RxFrame.len)
                {
                    App_Rx_St = APP_Rx_Get_Cs;
                }

                break;
            }

            case APP_Rx_Get_Cs:
            {
                /* This byte is the received checksum from sender */
                uint8_t received_cs = rx_byte;

                /* Recalculate checksum locally for verification */
                uint8_t calc_cs = APP_RxFrame.cmd ^ APP_RxFrame.len;

                /* Include all payload bytes in checksum calculation */
                for (uint8_t i = 0; i < APP_RxFrame.len; i++)
                {
                    calc_cs = calc_cs ^ APP_RxFrame.data[i];
                }

                /* Compare received checksum with calculated checksum */
                if (received_cs == calc_cs)
                {
                    /* Valid complete frame */
                    App_Rx_St = APP_Rx_Complete_Frame;
                }
                else
                {
                    /* Invalid frame:
                     * checksum mismatch means data corruption or bad alignment.
                     * Reset parser and wait for a fresh frame from SOF.
                     */
                    App_Rx_St = APP_Rx_Wait_Sof;
                }

                break;
            }

            case APP_Rx_Complete_Frame:
            {
                /* Frame is already complete and waiting to be consumed.
                 *
                 * We do not overwrite it here until upper layer calls APP_Check_Response()
                 * and resets the state machine.
                 */
                break;
            }
        }
    }

    /* Validate the usart rx byte */

    /* move state */
}


/******************************************************************************************
 *                              APP_Check_Response()
 *
 * Purpose:
 *  Tells the application whether a full valid GUI frame is ready.
 *
 * Description:
 *  This function acts like a "frame ready" checker.
 *  It is very similar in idea to FP_CheckPacket().
 *
 * Behavior:
 *  - If RX state machine reached APP_Rx_Complete_Frame
 *      -> return packet OK
 *      -> reset RX state to wait for the next frame
 *  - Otherwise
 *      -> return packet NOK
 *
 * Why reset here?
 *  Because once the upper layer accepts the frame,
 *  the receiver must be ready for the next one.
 *
 * Returns:
 *  APP_Rx_Full_Packet_ok  : full valid frame available
 *  APP_Rx_Full_Packet_Nok : no complete frame yet
 ******************************************************************************************/
static APP_Err_St_t APP_Check_Response(void)
{
	APP_Err_St_t APP_Err_St = APP_Rx_Full_Packet_Nok;

	/* If the state machine finished receiving and validating a frame */
	if(App_Rx_St == APP_Rx_Complete_Frame)
	{
		/* Inform caller that a complete frame is ready */
		APP_Err_St = APP_Rx_Full_Packet_ok;

		/* Reset RX state so next incoming frame can be received */
		App_Rx_St = APP_Rx_Wait_Sof;
	}

	return APP_Err_St;
}


/******************************************************************************************
 *                                  APP_Cylic()
 *
 * Purpose:
 *  Main cyclic function of the application layer.
 *
 * Description:
 *  This function should be called periodically from the super loop or scheduler.
 *  It is the main application logic handler.
 *
 * Main Tasks:
 *  -----------------------------------------------------------------------
 *  1. Check whether GUI sent any command
 *  2. If GUI requested enrollment -> switch FP driver to enroll mode
 *  3. If system is in fingerprint search mode:
 *       - check if a search result is available
 *       - read date/time from RTC
 *       - send access log to GUI
 *  4. If system is in enroll mode:
 *       - read current enroll instruction from FP driver
 *       - send instruction updates to GUI only when changed
 *
 * Important Design Idea:
 *  APP_Cylic() does not directly perform low-level work.
 *  It coordinates the modules.
 *
 * Example:
 *  - FP driver handles fingerprint state machine internally
 *  - RTC driver handles RTC registers internally
 *  - App layer only asks for results and sends them to GUI
 ******************************************************************************************/
void APP_Cylic(void)
{
	/* Will hold fingerprint match result status */
	uint8_t match_st;

	/* Will hold returned fingerprint user ID */
	uint16_t user_id;

	/* Final ID to send to GUI.
	 * If fingerprint did not match, this will be sent as 0.
	 */
	uint16_t id_to_send;

	/* RTC structures used for timestamping access events */
	RTC_Time_t time;
	RTC_Date_t date;

	/* Stores the previously sent enroll instruction.
	 *
	 * Why is this important?
	 * To avoid sending repeated identical instructions to GUI every cycle.
	 *
	 * Example:
	 * If current instruction is still "Place Finger",
	 * there is no need to resend it hundreds of times.
	 */
	static FP_GetEnroll_Instruction_t prev_inst =  FP_E_Inst_Idle;

	/* ------------------------------------------------------------------
	 * Step 1: Check if GUI sent any new frame
	 * ------------------------------------------------------------------
	 */
	APP_Check_RxFrame();

	/* If a valid full frame was received from GUI */
	if(APP_Check_Response() == APP_Rx_Full_Packet_ok)
	{
		/* If GUI requested enrollment mode */
		if(APP_RxFrame.cmd == APP_Enroll_Req)
		{
			/* Tell fingerprint driver to switch to enroll mode.
			 *
			 * From here, FP driver handles the full enrollment sequence internally.
			 */
			FP_SetMode(FP_ENROLL_MODE);
		}
	}

	/* ------------------------------------------------------------------
	 * Step 2: Behavior when FP driver is in SEARCH MODE
	 * ------------------------------------------------------------------
	 *
	 * In this mode, fingerprint driver continuously searches for user fingerprints.
	 * Once a result is available, App layer reads it and sends a log to GUI.
	 */
	if(FP_GetMode() == FP_SEARCH_MODE)
	{
		/* Payload that will be sent to GUI for access log.
		 *
		 * Layout:
		 * [0] match status (granted / denied)
		 * [1] user id high
		 * [2] user id low
		 * [3] hour
		 * [4] minute
		 * [5] second
		 * [6] day
		 * [7] month
		 * [8] year high
		 * [9] year low
		 */
		uint8_t user_payload[10];

		/* blocking ==> data ===> search blocking ==> gaurantee ===> data
//		APP_Send_Cmd(APP_Log_Access ,NULL ,0);
		 */

		/* Try to get a search result from the FP driver.
		 * FP_Get_User() is non-blocking and reads from queue.
		 */
		if(FP_Get_User(&match_st, &user_id) == FP_GetUser_Ok)
		{
			/* If a fingerprint result exists, get current date/time for logging */
			RTC_GetDate(&date);
			RTC_GetTime(&time);

			/* Byte 0 = access result
			 * If fingerprint matched -> granted
			 * else -> denied
			 */
			user_payload[0] = ((match_st == FP_MATCH_ST)? APP_USER_GRANTED : APP_USER_DENIED);

			/* Decide which user ID should be sent.
			 *
			 * If access granted:
			 *   send real matched user ID
			 *
			 * If access denied:
			 *   send 0 because there is no valid matched user
			 */
			id_to_send = ((match_st == FP_MATCH_ST)? user_id : 0);

			/* User ID is 16-bit, so split it into High byte and Low byte */
			user_payload[1] =(id_to_send >> 8);
			user_payload[2] =(id_to_send & 0xFF);

			/* Add current time to payload */
			user_payload[3] = time.hours;
			user_payload[4] = time.minutes;
			user_payload[5] = time.seconds;

			/* Add current date to payload */
			user_payload[6] = date.Day;
			user_payload[7] = date.month;
			user_payload[8] = date.year >> 8;
			user_payload[9] = date.year & 0xFF;

			/* Send access log frame to GUI */
			APP_Send_Cmd(APP_Log_Access, user_payload, APP_LOG_ACCESS_DATA_LEN);

			/* Reset previous instruction to idle.
			 *
			 * Why?
			 * Because when enrollment ends and system returns to search mode,
			 * we want enroll UI logic to restart cleanly next time.
			 */
			prev_inst  = FP_E_Inst_Idle;

			/* If the user grated turn on the relay
			 * (future application logic can be added here)
			 */
		}
	}

	/* ------------------------------------------------------------------
	 * Step 3: Behavior when FP driver is in ENROLL MODE
	 * ------------------------------------------------------------------
	 *
	 * In this mode, App layer does not directly manage enrollment steps.
	 * Instead, it asks FP driver:
	 *   "What should I tell the GUI now?"
	 *
	 * The FP driver returns a high-level instruction such as:
	 *   - Place finger
	 *   - Lift finger
	 *   - Place finger again
	 *   - Processing
	 *   - Success
	 *   - Failed
	 */
	else if (FP_GetMode() == FP_ENROLL_MODE)
	{
		/* Get current user-friendly instruction from fingerprint driver */
		FP_GetEnroll_Instruction_t current_inst = FP_GetEnroll_Instruction();

		/* Only send the instruction if it changed.
		 *
		 * This is very important because APP_Cylic() runs repeatedly.
		 * Without this condition, the same instruction would be sent continuously.
		 *
		 * So this is an optimization + protocol cleanliness improvement.
		 */
		if(current_inst != prev_inst)
		{
			/* Send current enrollment status to GUI */
			APP_Send_Cmd(APP_Enroll_St, &current_inst, 1);

			/* Save it so next cycles know what was last transmitted */
			prev_inst = current_inst;
		}

	}
}
