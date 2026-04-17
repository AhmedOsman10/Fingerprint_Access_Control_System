/*
 * FP.c
 *
 *  Created on: 2 Feb 2026
 *      Author: Ahmed
 *
 *  This file implements the low-level driver for the Fingerprint module.
 *  Communication is done via USART.
 *
 *  Packet Format Used:
 *  -------------------------------------------------------------------------
 *  Header (2) + Address (4) + PID (1) + Length (2) + Instruction Code (1) + Data (N) + Checksum (2)
 *
 *  Length = Instruction + Data + Checksum
 *  Checksum = arithmetic sum of: PID + Length (2 bytes) + Instruction + Data
 *  -------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>

#include "USART.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "FP.h"
#include "FP_Prv.h"
#include "FP_Cfg.h"
#include "stm32f4xx_hal.h"

static FP_Rx_St_t FP_Rx_St = FP_Wait_Rx_Header_H;
static FP_Enroll_St_t FP_Enroll_St = FP_E_Idle;
static uint8_t FP_Rx_Buff[30];
static uint16_t FP_Rx_Indx = 0;
static uint16_t FP_Rx_Packect_Len = 0;
static FP_Mode_t FP_Curr_Mode = FP_SEARCH_MODE;
static uint16_t FP_NextPage = 1;

static QueueHandle_t FP_Search_Q_Buff;
/******************************************************************************************
 *                                  FP_Init()
 *
 * Purpose:
 *  Initializes the fingerprint module driver.
 *
 * Description:
 *  - The fingerprint module communicates via USART.
 *  - This function initializes the configured USART instance.
 *  - If USART initialization succeeds, the FP driver is considered initialized.
 *
 * Returns:
 *  - FP_InitSuccess  : if USART init succeeded
 *  - FP_InitFailed   : if USART init failed
 ******************************************************************************************/
FP_Err_St_t FP_Init(void)
{
	FP_Err_St_t fp_err_st = FP_InitFailed;        /* Default state: failed */
	USART_Err_St_t usart_err_st = USART_InitFailed;

	/* Initialize the USART used for fingerprint communication */
	usart_err_st = USART_Init(USART_NUM_2);

	if(usart_err_st == USART_InitSuccess)
	{
		/* If USART is ready, FP driver is ready */
		fp_err_st = FP_InitSuccess;
	}
	else
	{
		/* Keep failure state */
	}

	if(fp_err_st == FP_InitSuccess)
	{
		FP_Search_Q_Buff = xQueueCreate(10, sizeof(FP_Search_Data_t));

		if(FP_Search_Q_Buff == NULL)
		{
			fp_err_st = FP_InitFailed;
		}
	}

	return fp_err_st;
}


/******************************************************************************************
 *                              FP_SendCommand()
 *
 * Purpose:
 *  Builds and sends a fingerprint command packet over USART.
 *
 * Parameters:
 *  inst_code    -> Fingerprint instruction code (command ID)
 *  payload      -> Pointer to data bytes (if required by command)
 *  payload_len  -> Number of data bytes
 *
 * Packet Construction:
 *
 *  Byte Index | Field
 *  --------------------------------------------------
 *      0-1    | Header (fixed: 0xEF01)
 *      2-5    | Module Address (default: 0xFFFFFFFF)
 *        6    | PID (Packet Identifier = command packet)
 *      7-8    | Length (Instruction + Data + Checksum)
 *        9    | Instruction Code
 *     10..N   | Data (payload)
 *     last 2  | Checksum (16-bit)
 *
 * Checksum Calculation:
 *  Sum of all bytes starting from PID (index 6)
 *  up to the last payload byte.
 *  Header and Address are NOT included in checksum.
 *
 ******************************************************************************************/
FP_Err_St_t FP_SendCommand(uint8_t inst_code , uint8_t *payload , uint8_t payload_len)
{
	FP_Err_St_t FP_Err_St = FP_SendCmd_Success;
	uint8_t tx[30];      /* Transmission buffer (holds complete packet) */
	uint16_t sum = 0 ;   /* 16-bit checksum accumulator */

	/*
	   Length field represents: Instruction Code (1 byte) + Data (payload_len bytes) + Checksum (2 bytes)
	 */
	uint16_t FP_Packet_len = payload_len + INST_CODE_LEN + CHECK_SUM_LEN ;

	/* -------------------- Packet Building -------------------- */

	/* Command header (fixed value defined in configuration) */
	tx[0] = FP_HEADER_H;
	tx[1] = FP_HEADER_L;

	/* Module address (default address = 0xFFFFFFFF) */
	tx[2] = 0xFF;
	tx[3] = 0xFF;
	tx[4] = 0xFF;
	tx[5] = 0xFF;

	/* PID: indicates this is a command packet (usually 0x01) */
	tx[6] = FP_PID_CMD;

	/* Length field (Big Endian: High byte first) */
	tx[7]  = (FP_Packet_len >> 8);      /* LEN_H */
	tx[8]  = (FP_Packet_len & 0xFF) ;   /* LEN_L */

	/* Instruction Code */
	tx[9] = inst_code;

	/* -------------------- Payload Copy -------------------- */

	/* Copy payload data after instruction byte */
	for(uint8_t i = 0 ; i < payload_len ;i++)
	{
		if(payload != NULL)
			tx[10 + i] = payload[i];
	}

	/* -------------------- Checksum Calculation -------------------- */

	/*
	   Checksum is calculated from: PID (index 6) up to the last payload byte
	   Range: 6 → (9 + payload_len)
	 */
	for(uint8_t i = 6 ; i < (10 + payload_len) ; i++)
	{
		sum = sum  + tx[i];
	}

	/* Checksum is placed immediately after payload */
	uint8_t sum_indx_start = 10 + payload_len ;

	tx[sum_indx_start]     = sum >> 8;     /* Checksum High Byte */
	tx[sum_indx_start + 1] = sum & 0xFF;   /* Checksum Low Byte */

	/* Total frame length to be transmitted */
	uint8_t frame_len = sum_indx_start + CHECK_SUM_LEN;

	/* -------------------- Transmission -------------------- */

	/* Send full frame byte-by-byte using USART driver */
	for (uint8_t i = 0 ; i < frame_len; i++)
	{
		if(USART_SendByte(USART_NUM_2, tx[i]) !=  USART_Tx_Ok)
		{
			FP_Err_St = FP_SendCmd_Failed;
			break;
		}
	}
	return FP_Err_St;
}


// FP_Rx_Cyclic ==> check response ==> periodic task
void FP_Rx_Cyclic(void)
{
	uint16_t calc_sum = 0  ;   /* Used to calculate checksum on MCU side */
	uint16_t rec_sum  = 0  ;   /* Used to store received checksum from sensor */
	uint8_t byte = 0 ;         /* Holds each received byte from USART */

	/* We use "break" after each case, even if the condition inside the case is true, because we want to process ONLY one received byte per loop iteration.
	 * If we did not use "break", the execution would continue to the next case inside the same switch statement without receiving a new byte first.
	 * By using "break", we exit the switch statement and return back to the while loop, where USART_ReceiveByte() is called again.
	 * This guarantees that:
	 *   - A new byte is received from UART.
	 *   - The variable "byte" gets updated with the new value.
	 *   - The state machine progresses correctly one byte at a time.
	 * In simple words: One state → processes one byte → exit → receive next byte → process again.
	 */

	/* Keep reading bytes as long as USART driver says a byte is available */
	while(USART_ReceiveByte(FP_USART_NUM_ , &byte) == USART_Rx_Ok)
	{
		/* State machine that reconstructs the packet byte by byte */
		switch(FP_Rx_St)
		{

		/* Check header high*/
		case FP_Wait_Rx_Header_H:
		{
			/* Waiting for first header byte */
			if(byte == FP_HEADER_H)
			{
				/* First header byte correct → move to next state */
				FP_Rx_St = FP_Wait_Rx_Header_L;
			}
			break;
		}

		/* Check header low */
		case FP_Wait_Rx_Header_L:
		{
			/* Waiting for second header byte */
			if(byte == FP_HEADER_L)
			{
				/* Header fully matched → now expect address bytes */
				FP_Rx_St = FP_Wait_Rx_ADDR;
			}
			else
			{
				/* Header mismatch → restart synchronization */
				FP_Rx_St = FP_Wait_Rx_Header_H;
			}
			break;
		}

		/* We assume the Address field is correct. The address consists of 4 bytes. We are not interested in validating or storing these bytes in this driver.
		 * The bytes are still received from UART, but we simply ignore their values and just count them to keep the packet aligned.
		 */
		case FP_Wait_Rx_ADDR:
		{
			/* Count address bytes */
			FP_Rx_Indx++;

			/* After receiving 4 address bytes, reset index and move to PID state. */
			if(FP_Rx_Indx == 4)
			{
				/* Reset index so it can be reused for data buffering */
				FP_Rx_Indx = 0;

				/* Move to PID state */
				FP_Rx_St = FP_Wait_Rx_Pid;
			}

			break;
		}

		/* check PID ===> 0x07 */
		case FP_Wait_Rx_Pid:
		{
			/* Expecting ACK packet PID */
			if(byte == FP_PID_ACK)
			{
				/* Correct PID → next expect Length High byte */
				FP_Rx_St = FP_Wait_Rx_len_H;
			}
			else
			{
				/* Unexpected PID → reset state machine */
				FP_Rx_St = FP_Wait_Rx_Header_H;
			}
			break;
		}

		/* Check packet Length High */
		case FP_Wait_Rx_len_H:
		{
			/* Store MSB of length */
			FP_Rx_Packect_Len = byte << 8;

			/* Next expect LSB of length */
			FP_Rx_St = FP_Wait_Rx_len_L;
			break;
		}

		/* Check packet Length low  */
		case FP_Wait_Rx_len_L:
		{
			/* Complete 16-bit length value */
			FP_Rx_Packect_Len |= byte ;

			/* After length is known, start collecting payload + checksum */
			FP_Rx_St = FP_Wait_Rx_Data;
			break;
		}

		/* Check packet Data  */
		case FP_Wait_Rx_Data:
		{
			/* We created this array (FP_Rx_Buff[]) to store the data that comes AFTER the length field, even though the length itself is already
			 * stored in a separate variable. If you look carefully, you will notice that we did NOT store the previous fields such as: Header, Address and PID.
			 * The reason is that these fields are not important for our application logic at this stage. We only validate them and then ignore them.
			 * However, starting from the Length and everything after it (Payload + Checksum), we MUST store those bytes because:
			 *   1) We need them to calculate and verify the checksum.
			 *   2) The payload contains useful information such as:
			 *        - Confirmation code
			 *        - Returned data from the sensor
			 *   3) The payload might be needed later in the application layer.
			 */

			/* Store received byte into buffer */
			FP_Rx_Buff[FP_Rx_Indx] = byte;

			/* Increment buffer index */
			FP_Rx_Indx++;

			/* Firstly:
			 * This condition will NOT execute until ALL expected bytes (payload + checksum) are fully received. This means the loop will execute at least 3 times:
			 *   - Minimum 1 byte for data
			 *   - 2 bytes for checksum
			 Secondly:
			 * We do NOT know how many bytes we should receive immediately after the Length field because the payload size depends on
			 * the response of the command sent to the sensor. The Length value tells us exactly how many bytes follow: Length = Payload + Checksum
			 * The last 2 bytes ALWAYS belong to the checksum. Everything before those 2 bytes belongs to the payload.
			 * Example:
			 *   If Length = 5
			 *   → First 3 bytes  = Payload
			 *   → Last 2 bytes   = Checksum
			 */

			/* Check if full expected length is received */
			if(FP_Rx_Indx == FP_Rx_Packect_Len)
			{
				/************************* Our Calculated Checksum ***********************************/
				calc_sum = FP_PID_ACK + FP_Rx_Packect_Len ;

				/* Add all payload bytes to checksum */
				for(uint8_t i = 0 ; i < FP_Rx_Packect_Len - 2 ; i++)
				{
					calc_sum += FP_Rx_Buff[i];
				}
				/****************************************************************************************/

				/*********************** Received Checksum (Calculated by Sensor) **********************/
				rec_sum = FP_Rx_Buff[FP_Rx_Packect_Len - 2] << 8 | FP_Rx_Buff[FP_Rx_Packect_Len - 1];
				/******************************************************************************/

				/* Compare calculated checksum with received checksum */
				if(calc_sum == rec_sum)
				{
					/* Valid frame */
					FP_Rx_Indx = 0;
					FP_Rx_St = FP_Wait_Rx_Frame_Complete;
				}
				else
				{
					/*TODO: Error handling*/
					FP_Rx_Indx = 0;
					FP_Rx_St = FP_Wait_Rx_Header_H;
				}
			}
			break;
		}

		/* Check packet Data  */
		case FP_Wait_Rx_Frame_Complete:
		{
			/* At this state, full frame has been received and verified.
			 * Upper layer should call FP_CheckPacket() to consume it.
			 */
			break;
		}
		}
	}
}

void FP_SetMode(FP_Mode_t mode)
{

	FP_Curr_Mode = mode;

	FP_Enroll_St = FP_E_Idle;

	if(mode == FP_ENROLL_MODE)
	{
		FP_Enroll_St = FP_E_Start;
	}
}

FP_Mode_t FP_GetMode(void)
{
	return FP_Curr_Mode;
}


/* This function purpose is to check the packet status.
 * - FP_Err_St: is a local variable returned to FP_Enroll_SM() (or any other caller) to indicate whether the received packet frame is completed and valid or not.
 * - FP_Rx_St: is a global receive state variable updated by FP_Rx_Cyclic().
 *   If FP_Rx_St reaches FP_Wait_Rx_Frame_Complete, that means:
 *     -> Full frame was received
 *     -> Checksum was verified successfully
 *
 * - After detecting a complete frame, we reset FP_Rx_St back to FP_Wait_Rx_Header_H so the receiver is ready to start syncing from the header for the next command response.
 *   This makes the next reception start correctly and normally.
 */
FP_Err_St_t FP_CheckPacket(void)
{
	FP_Err_St_t FP_Err_St = FP_Rx_Full_Packet_Nok;

	/* If the RX state machine reached "Frame Complete", then a full valid packet is ready */
	if(FP_Rx_St == FP_Wait_Rx_Frame_Complete)
	{
		/* Reset RX state to be ready for next packet reception */
		FP_Rx_St = FP_Wait_Rx_Header_H;

		/* Inform caller that packet is complete */
		FP_Err_St = FP_Rx_Full_Packet_Ok;
	}

	return FP_Err_St;
}


void FP_Enroll_SM(void)
{
	static uint8_t err_c = 0;
	switch (FP_Enroll_St)
	{
	case FP_E_Idle:
	{
		/* Do nothing */
		break;
	}

	case FP_E_Start:
	{
		FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
		break;
	}

	case FP_E_SEND_GET_IMG_1_CMD:
	{
		/* Send generation image command to collect user finger */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_GET_IMG_1_CMD;

		break;
	}

	case FP_E_WAIT_GET_IMG_1_CMD:
	{
		/* Wait for the generation image response */
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				FP_Enroll_St = FP_E_SEND_GEN_CHAR_1_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
			}
		}
		else{
			err_c++;
			if(err_c > 5)
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
				err_c = 0;
			}
		}
		break;
	}

	case FP_E_SEND_GEN_CHAR_1_CMD:
	{
		uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);
		FP_Enroll_St = FP_E_WAIT_GEN_CHAR_1_CMD;
		break;
	}

	case FP_E_WAIT_GEN_CHAR_1_CMD:
	{
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_LIFT_CHECK_CMD:
	{
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_LIFT_CHECK_CMD;
		break;
	}

	case FP_E_WAIT_LIFT_CHECK_CMD:
	{
		/* Wait for the generation image response */
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_NO_FINGER)
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_GET_IMG_2_CMD:
	{
		/* Send generation image command to collect user finger */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_GET_IMG_2_CMD;
		break;
	}

	case FP_E_WAIT_GET_IMG_2_CMD:
	{
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				FP_Enroll_St = FP_E_SEND_GEN_CHAR_2_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_GEN_CHAR_2_CMD:
	{
		uint8_t FP_Buff_2 = FP_CHAR_FILE_2;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_2, 1);
		FP_Enroll_St = FP_E_WAIT_GEN_CHAR_2_CMD;
		break;
	}

	case FP_E_WAIT_GEN_CHAR_2_CMD:
	{
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				FP_Enroll_St = FP_E_SEND_MERGE_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_MERGE_CMD:
	{
		FP_SendCommand(FP_MERGE_TWO_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_MERGE_CMD;
		break;
	}

	case FP_E_WAIT_MERGE_CMD:
	{
		/* Wait for the generation image response */
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_MERGE_SUCCESS)
			{
				FP_Enroll_St = FP_E_SEND_STORE_CMD;
			}
			else
			{
				FP_Enroll_St = FP_E_SEND_MERGE_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_STORE_CMD:
	{
		// store the image into the flash
		/* char file          Page id high       page id low */
		uint8_t FP_Store_Payload[3] = {FP_CHAR_FILE_1, (FP_NextPage >> 8), (FP_NextPage & 0xff)};

		FP_SendCommand(FP_STORE_IMG_CMD,FP_Store_Payload, 3);
		FP_Enroll_St = FP_E_WAIT_STORE_CMD;
		break;
	}

	case FP_E_WAIT_STORE_CMD:
	{
		/* Wait for the generation image response */
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_STORE_SUCCESS)
			{
				FP_Enroll_St = FP_E_Success;
			}
			else
			{
				FP_Enroll_St = FP_E_Failed;
			}
		}
		break;
	}

	case FP_E_Success:
	{
		FP_NextPage++;
		FP_SetMode(FP_SEARCH_MODE);
		break;
	}

	case FP_E_Failed:
	{
		FP_SetMode(FP_SEARCH_MODE);
		break;
	}
	}
}


FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void)
{
	if(FP_Curr_Mode != FP_ENROLL_MODE)
	{
		return FP_E_Inst_Idle;
	}

	switch(FP_Enroll_St)
	{

	/* Place Your finger */
	case FP_E_SEND_GET_IMG_1_CMD:
	case FP_E_WAIT_GET_IMG_1_CMD:

		return FP_E_Inst_Place_Finger;
		break;

		/*Lift the finger*/
	case FP_E_SEND_LIFT_CHECK_CMD:
	case FP_E_WAIT_LIFT_CHECK_CMD:

		return FP_E_Inst_Lift_Finger;
		break;

		/* Place the finger again*/
	case FP_E_SEND_GET_IMG_2_CMD:
	case FP_E_WAIT_GET_IMG_2_CMD:

		return FP_E_Inst_Place_Finger_Again;
		break;

		/*Processing */
	case FP_E_SEND_GEN_CHAR_1_CMD:
	case FP_E_WAIT_GEN_CHAR_1_CMD:
	case FP_E_SEND_GEN_CHAR_2_CMD:
	case FP_E_WAIT_GEN_CHAR_2_CMD:
	case FP_E_SEND_MERGE_CMD:
	case FP_E_WAIT_MERGE_CMD:
	case FP_E_SEND_STORE_CMD:
	case FP_E_WAIT_STORE_CMD:

		return FP_E_Inst_Processing;
		break;



	case FP_E_Success:
		return FP_E_Inst_Success;
		break;

	case FP_E_Failed:
		return FP_E_Inst_Failed;


	}
}


void FP_SEARCH_SM(void)
{
	static FP_Search_St_t FP_Search_St = FP_S_SEND_GET_IMG_CMD;
	static uint16_t user_id = 0;
	static uint32_t start = 0;

	switch (FP_Search_St)
	{
	case FP_S_SEND_GET_IMG_CMD:
	{
		/* Send generation image command to collect user finger */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		start = HAL_GetTick(); // 100
		FP_Search_St = FP_S_WAIT_GET_IMG_CMD;
		break;
	}

	case FP_S_WAIT_GET_IMG_CMD:
	{
		/* Wait for the generation image response */
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				FP_Search_St = FP_S_SEND_GEN_CHAR_1_CMD;
			}
			else
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		else
		{
			//current - start > 1000
			if(HAL_GetTick() - start > 2000)
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}


		}
		break;
	}

	case FP_S_SEND_GEN_CHAR_1_CMD:
	{
		uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);
		start = HAL_GetTick(); // 100
		FP_Search_St = FP_S_WAIT_GEN_CHAR_1_CMD;
		break;
	}

	case FP_S_WAIT_GEN_CHAR_1_CMD:
	{
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				FP_Search_St = FP_S_SEND_SEARCH_CMD;
			}
			else
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		else
		{
			//current - start > 1000
			if(HAL_GetTick() - start > 2000)
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}


		}
		break;
	}

	case FP_S_SEND_SEARCH_CMD:
	{
		uint8_t search_payload[5] = { FP_CHAR_FILE_1, 0x00, 0x00, 0x00, 0x64};
		FP_SendCommand(FP_SEARCH_CMD, search_payload, 5);
		start = HAL_GetTick(); // 100
		FP_Search_St = FP_S_WAIT_SEARCH_CMD;
		break;
	}

	case FP_S_WAIT_SEARCH_CMD:
	{
		if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check if get the image successful */
			if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_MATCH)
			{
				user_id = ((FP_Rx_Buff[1] << 8) | FP_Rx_Buff[2]);
				FP_Search_St = FP_S_MATCH;
			}
			else if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_NOT_MATCH)
			{
				FP_Search_St = FP_S_NOT_MATCH;
			}
		}
		else
		{
			//current - start > 1000
			if(HAL_GetTick() - start > 2000)
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}


		}
		break;
	}

	case FP_S_MATCH:
	{
		FP_Search_Data_t FP_Search_Data;
		FP_Search_Data.match_st = FP_MATCH_ST;
		FP_Search_Data.user_id = user_id;

		xQueueSend(FP_Search_Q_Buff, &FP_Search_Data, 0);

		FP_Search_St = FP_S_SEND_GET_IMG_CMD;
		break;
	}

	case FP_S_NOT_MATCH:
	{
		FP_Search_Data_t FP_Search_Data;
		FP_Search_Data.match_st = FP_NOT_MATCH_ST;
		FP_Search_Data.user_id = 0;

		xQueueSend(FP_Search_Q_Buff, &FP_Search_Data, 0);

		FP_Search_St = FP_S_SEND_GET_IMG_CMD;
		break;
	}
	}
}


void FP_MainFunction_Cyclic(void)
{
    FP_Rx_Cyclic();

    switch (FP_Curr_Mode)
    {
        case FP_ENROLL_MODE:
        {
            FP_Enroll_SM();
            break;
        }

        case FP_SEARCH_MODE:
        {
            FP_SEARCH_SM();
            break;
        }
    }
}


void FP_SimpleTesT(void)
{
	static uint8_t flag = 0;


	if(flag == 0)
	{
		FP_Err_St_t FP_Err_St = FP_Init();
		//			HAL_Delay(1000);
		FP_SendCommand(FP_GEN_IMG_CMD , NULL , 0);
		flag = 1;
	}
	FP_Rx_Cyclic();

	if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
	{
		if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == 0x00)
		{
			printf("Detected finger");
		}
		FP_SendCommand(FP_GEN_IMG_CMD , NULL , 0);
		//			HAL_Delay(150);
		printf("Simple Test Success");
	}
}


FP_Err_St_t FP_Get_User(uint8_t *match_st , uint16_t *user_id)
{
	FP_Err_St_t FP_Err_St = FP_GetUser_NOk;
	FP_Search_Data_t ret_dat;
	if(xQueueReceive(FP_Search_Q_Buff, &ret_dat, 0) == pdPASS)
	{
		*match_st = ret_dat.match_st;
		*user_id = ret_dat.user_id;
		FP_Err_St = FP_GetUser_Ok;
	}
	else
	{
		FP_Err_St = FP_GetUser_NOk;
	}
	return FP_Err_St;
}
