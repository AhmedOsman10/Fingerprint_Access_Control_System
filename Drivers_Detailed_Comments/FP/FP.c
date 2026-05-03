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

/* RX state machine current state.
 * This variable controls how the driver parses the incoming UART stream.
 * It starts by waiting for the first byte of the fingerprint packet header.
 */
static FP_Rx_St_t FP_Rx_St = FP_Wait_Rx_Header_H;

/* Enrollment state machine current state.
 * This keeps track of the current step of fingerprint enrollment.
 */
static FP_Enroll_St_t FP_Enroll_St = FP_E_Idle;

/* RX buffer used to store the bytes received after the Length field.
 * This usually contains:
 *   - confirmation code
 *   - returned data from the sensor
 *   - checksum bytes at the end
 */
static uint8_t FP_Rx_Buff[30];

/* General receive index used by the RX state machine.
 * It is reused for:
 *   - counting the 4 address bytes
 *   - storing payload and checksum bytes into FP_Rx_Buff[]
 */
static uint16_t FP_Rx_Indx = 0;

/* Stores the packet length extracted from the received fingerprint response.
 * This value represents the number of bytes after the Length field,
 * which includes payload + checksum.
 */
static uint16_t FP_Rx_Packect_Len = 0;

/* Current operating mode of the fingerprint driver.
 * Default mode is search mode.
 */
static FP_Mode_t FP_Curr_Mode = FP_SEARCH_MODE;

/* Holds the next page ID in the fingerprint module flash memory.
 * Each successful enrollment stores the new fingerprint template in this page.
 */
static uint16_t FP_NextPage = 1;

/* FreeRTOS queue used to pass fingerprint search results
 * from the search state machine to the application layer.
 */
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
		/* Create a queue to carry search results from the driver
		 * to the application layer.
		 *
		 * Queue length = 10 elements
		 * Element type = FP_Search_Data_t
		 */
		FP_Search_Q_Buff = xQueueCreate(10, sizeof(FP_Search_Data_t));

		if(FP_Search_Q_Buff == NULL)
		{
			/* Queue creation failed, so initialization is considered failed */
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
		/* Only copy if payload pointer is valid */
		if(payload != NULL)
		{
			tx[10 + i] = payload[i];
		}
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
		/* If any single byte fails to transmit, stop immediately */
		if(USART_SendByte(USART_NUM_2, tx[i]) !=  USART_Tx_Ok)
		{
			FP_Err_St = FP_SendCmd_Failed;
			break;
		}
	}
	return FP_Err_St;
}


/******************************************************************************************
 *                                  FP_Rx_Cyclic()
 *
 * Purpose:
 *  Periodically receives and parses response packets coming from the fingerprint sensor.
 *
 * Description:
 *  - This function acts as the receive state machine of the driver.
 *  - It reads incoming UART bytes one by one from the USART driver.
 *  - It validates the packet structure in this sequence:
 *      Header High
 *      Header Low
 *      Address (4 bytes)
 *      PID
 *      Length High
 *      Length Low
 *      Payload + Checksum
 *  - It stores the useful bytes after the Length field into FP_Rx_Buff[].
 *  - After receiving the full packet, it calculates the checksum locally and compares
 *    it with the checksum received from the sensor.
 *  - If checksum is valid, the RX state moves to FP_Wait_Rx_Frame_Complete.
 *
 * Notes:
 *  - This function must be called periodically from the main cyclic function or task.
 *  - It does not directly return packet data to the caller.
 *    Instead, packet readiness is checked later using FP_CheckPacket().
 *  - Only ACK packets are accepted in the current implementation.
 *
 * RX Behavior:
 *  - The state machine consumes bytes one at a time.
 *  - If any field is invalid, the parser resets to waiting for a new header.
 *  - Once a valid packet is fully received, the function stops parsing further frames
 *    until the upper layer acknowledges it via FP_CheckPacket().
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
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
					/* Valid frame:
					 * - clear the receive index
					 * - mark the packet as complete for upper layer processing
					 */
					FP_Rx_Indx = 0;
					FP_Rx_St = FP_Wait_Rx_Frame_Complete;
				}
				else
				{
					/*TODO: Error handling*/
					/* Wrong checksum means packet corruption or bad synchronization.
					 * Reset parser and wait for a new packet from the beginning.
					 */
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


/******************************************************************************************
 *                                  FP_SetMode()
 *
 * Purpose:
 *  Sets the current operating mode of the fingerprint driver.
 *
 * Description:
 *  - Updates the global fingerprint mode variable.
 *  - Resets the enrollment state machine to idle.
 *  - If the selected mode is enrollment mode, it prepares the enrollment state machine
 *    to start from the first step.
 *
 * Parameters:
 *  mode : Desired fingerprint mode.
 *         Example:
 *           - FP_ENROLL_MODE
 *           - FP_SEARCH_MODE
 *
 * Notes:
 *  - Switching to FP_ENROLL_MODE starts the enrollment flow from FP_E_Start.
 *  - Switching to FP_SEARCH_MODE stops the enrollment sequence and returns control
 *    to the search flow.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_SetMode(FP_Mode_t mode)
{
	/* Update current driver mode */
	FP_Curr_Mode = mode;

	/* Reset enrollment state whenever mode changes */
	FP_Enroll_St = FP_E_Idle;

	if(mode == FP_ENROLL_MODE)
	{
		/* Prepare enrollment state machine to start */
		FP_Enroll_St = FP_E_Start;
	}
}


/******************************************************************************************
 *                                  FP_GetMode()
 *
 * Purpose:
 *  Returns the current operating mode of the fingerprint driver.
 *
 * Description:
 *  - Provides the application layer with the current internal driver mode.
 *  - Useful when the application wants to know whether the driver is currently
 *    in enrollment mode or search mode.
 *
 * Returns:
 *  - FP_Curr_Mode : Current fingerprint driver mode.
 ******************************************************************************************/
FP_Mode_t FP_GetMode(void)
{
	/* Return current mode */
	return FP_Curr_Mode;
}


/******************************************************************************************
 *                                FP_CheckPacket()
 *
 * Purpose:
 *  Checks whether a full valid fingerprint response packet has been received.
 *
 * Description:
 *  - This function checks the global RX state variable updated by FP_Rx_Cyclic().
 *  - If the RX state machine has reached FP_Wait_Rx_Frame_Complete, it means:
 *      1. A full packet was received.
 *      2. The checksum was verified successfully.
 *  - After detecting a complete packet, this function resets the RX state machine
 *    to FP_Wait_Rx_Header_H so the next response can be received correctly.
 *
 * Notes:
 *  - This function is the interface between the RX parser and the higher-level
 *    state machines such as enrollment and search.
 *  - It does not parse the payload itself; it only reports whether a valid packet
 *    is ready inside FP_Rx_Buff[].
 *
 * Returns:
 *  - FP_Rx_Full_Packet_Ok  : A full valid packet is ready.
 *  - FP_Rx_Full_Packet_Nok : No complete valid packet available yet.
 ******************************************************************************************/
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


/******************************************************************************************
 *                                  FP_Enroll_SM()
 *
 * Purpose:
 *  Runs the fingerprint enrollment state machine.
 *
 * Description:
 *  - This function handles the full process of enrolling a new fingerprint into
 *    the sensor memory.
 *  - It is designed as a non-blocking cyclic state machine.
 *  - The enrollment sequence includes:
 *      1. Capture first fingerprint image.
 *      2. Generate first character file.
 *      3. Ask user to lift finger.
 *      4. Capture second fingerprint image.
 *      5. Generate second character file.
 *      6. Merge both character files into one template.
 *      7. Store the template into sensor flash memory.
 *  - On success, the next page index is incremented and the mode returns to search mode.
 *  - On failure, the mode also returns to search mode.
 *
 * Internal Behavior:
 *  - Uses FP_SendCommand() to send commands.
 *  - Uses FP_CheckPacket() to detect valid sensor responses.
 *  - Uses FP_Rx_Buff[] to inspect the returned confirmation code.
 *
 * Notes:
 *  - This function should be called cyclically only when FP_Curr_Mode == FP_ENROLL_MODE.
 *  - It does not block waiting for responses.
 *  - It progresses step by step across multiple calls.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_Enroll_SM(void)
{
    /* Static retry counter.
     * Because this function runs cyclically, static preserves the counter value
     * between function calls.
     */
    static uint8_t err_c = 0;

    switch (FP_Enroll_St)
    {
        case FP_E_Idle:
        {
            /* Do nothing in idle state */
            break;
        }

        case FP_E_Start:
        {
            /* First transition of enrollment flow */
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
                    /* First fingerprint image captured successfully */
                    FP_Enroll_St = FP_E_SEND_GEN_CHAR_1_CMD;
                }
                else
                {
                    /* Retry image capture if no valid finger image yet */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
                }
            }
            else
            {
                /* Count how long we have been waiting without valid packet */
                err_c++;

                if (err_c > 5)
                {
                    /* Retry the command after repeated no-response cycles */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
                    err_c = 0;
                }
            }

            break;
        }

        case FP_E_SEND_GEN_CHAR_1_CMD:
        {
            /* Convert the first image into a character file stored in buffer 1 */
            uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
            FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);
            FP_Enroll_St = FP_E_WAIT_GEN_CHAR_1_CMD;
            break;
        }

        case FP_E_WAIT_GEN_CHAR_1_CMD:
        {
            /* Wait for response after generating the first character file */
            if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
            {
                /* Check if get the image successful */
                if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
                {
                    /* Ask user to lift finger after first template generation */
                    FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
                }
                else
                {
                    /* Restart process if character generation failed */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
                }
            }

            break;
        }

        case FP_E_SEND_LIFT_CHECK_CMD:
        {
            /* Reuse the get-image command to detect if the finger was lifted */
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
                    /* Finger removed successfully, proceed to second scan */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
                }
                else
                {
                    /* Keep checking until user lifts the finger */
                    FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
                }
            }

            break;
        }

        case FP_E_SEND_GET_IMG_2_CMD:
        {
            /* Send generation image command to collect user finger again */
            FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
            FP_Enroll_St = FP_E_WAIT_GET_IMG_2_CMD;
            break;
        }

        case FP_E_WAIT_GET_IMG_2_CMD:
        {
            /* Wait for second image capture response */
            if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
            {
                if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
                {
                    /* Second fingerprint image captured successfully */
                    FP_Enroll_St = FP_E_SEND_GEN_CHAR_2_CMD;
                }
                else
                {
                    /* Retry second image capture */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
                }
            }

            break;
        }

        case FP_E_SEND_GEN_CHAR_2_CMD:
        {
            /* Convert the second image into a character file stored in buffer 2 */
            uint8_t FP_Buff_2 = FP_CHAR_FILE_2;
            FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_2, 1);
            FP_Enroll_St = FP_E_WAIT_GEN_CHAR_2_CMD;
            break;
        }

        case FP_E_WAIT_GEN_CHAR_2_CMD:
        {
            /* Wait for response after generating the second character file */
            if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
            {
                /* Check if get the image successful */
                if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
                {
                    /* Both character files are ready, next merge them */
                    FP_Enroll_St = FP_E_SEND_MERGE_CMD;
                }
                else
                {
                    /* Retry second capture flow if second character generation failed */
                    FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
                }
            }

            break;
        }

        case FP_E_SEND_MERGE_CMD:
        {
            /* Merge the two character files into a single fingerprint template */
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
                    /* Template merge succeeded, now store it in flash */
                    FP_Enroll_St = FP_E_SEND_STORE_CMD;
                }
                else
                {
                    /* Retry merge if needed */
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

            /* Store the fingerprint template into the next available page */
            FP_SendCommand(FP_STORE_IMG_CMD, FP_Store_Payload, 3);
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
                    /* Full enrollment completed successfully */
                    FP_Enroll_St = FP_E_Success;
                }
                else
                {
                    /* Store step failed */
                    FP_Enroll_St = FP_E_Failed;
                }
            }

            break;
        }

        case FP_E_Success:
        {
            /* Increase page number only after successful enrollment */
            FP_NextPage++;

            /* Return to search mode after finishing enrollment */
            FP_SetMode(FP_SEARCH_MODE);
            break;
        }

        case FP_E_Failed:
        {
            /* Return to search mode after failure */
            FP_SetMode(FP_SEARCH_MODE);
            break;
        }
    }
}


/******************************************************************************************
 *                           FP_GetEnroll_Instruction()
 *
 * Purpose:
 *  Provides a user-friendly enrollment instruction based on the current enrollment state.
 *
 * Description:
 *  - Converts the internal enrollment state machine state into a higher-level
 *    instruction/status that can be shown to the application layer, LCD, GUI,
 *    or terminal.
 *  - Examples of returned instructions:
 *      - Place finger
 *      - Lift finger
 *      - Place finger again
 *      - Processing
 *      - Success
 *      - Failed
 *
 * Behavior:
 *  - If the driver is not in enrollment mode, this function returns idle.
 *  - Otherwise, it maps the current FP_Enroll_St value to a user instruction.
 *
 * Notes:
 *  - This function is useful to separate internal driver logic from UI logic.
 *  - The application can call it periodically while enrollment is active.
 *
 * Returns:
 *  - FP_GetEnroll_Instruction_t representing the current user instruction/state.
 ******************************************************************************************/
FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void)
{
    /* If not in enrollment mode, there is no active enrollment instruction */
    if (FP_Curr_Mode != FP_ENROLL_MODE)
    {
        return FP_E_Inst_Idle;
    }

    /* Convert internal state machine steps into user-friendly instructions */
    switch (FP_Enroll_St)
    {
        /* Place Your finger */
        case FP_E_SEND_GET_IMG_1_CMD:
        case FP_E_WAIT_GET_IMG_1_CMD:
        {
            return FP_E_Inst_Place_Finger;
            break;
        }

        /* Lift the finger */
        case FP_E_SEND_LIFT_CHECK_CMD:
        case FP_E_WAIT_LIFT_CHECK_CMD:
        {
            return FP_E_Inst_Lift_Finger;
            break;
        }

        /* Place the finger again */
        case FP_E_SEND_GET_IMG_2_CMD:
        case FP_E_WAIT_GET_IMG_2_CMD:
        {
            return FP_E_Inst_Place_Finger_Again;
            break;
        }

        /* Processing */
        case FP_E_SEND_GEN_CHAR_1_CMD:
        case FP_E_WAIT_GEN_CHAR_1_CMD:
        case FP_E_SEND_GEN_CHAR_2_CMD:
        case FP_E_WAIT_GEN_CHAR_2_CMD:
        case FP_E_SEND_MERGE_CMD:
        case FP_E_WAIT_MERGE_CMD:
        case FP_E_SEND_STORE_CMD:
        case FP_E_WAIT_STORE_CMD:
        {
            return FP_E_Inst_Processing;
            break;
        }

        case FP_E_Success:
        {
            return FP_E_Inst_Success;
            break;
        }

        case FP_E_Failed:
        {
            return FP_E_Inst_Failed;
            break;
        }
    }

    /* Safety return in case state is outside known cases */
    return FP_E_Inst_Idle;
}


/******************************************************************************************
 *                                  FP_SEARCH_SM()
 *
 * Purpose:
 *  Runs the fingerprint search state machine.
 *
 * Description:
 *  - This function continuously searches for a finger match in the sensor database.
 *  - It is implemented as a non-blocking cyclic state machine.
 *  - The search sequence includes:
 *      1. Capture fingerprint image.
 *      2. Generate character file from the captured image.
 *      3. Search the stored fingerprint library for a match.
 *      4. Push match or no-match result into a FreeRTOS queue.
 *
 * Internal Behavior:
 *  - Uses HAL_GetTick() for timeout handling between send and wait states.
 *  - Uses FP_SendCommand() to communicate with the fingerprint module.
 *  - Uses FP_CheckPacket() to detect complete valid responses.
 *  - On a successful match, extracts the user/page ID from FP_Rx_Buff[].
 *  - Sends results to FP_Search_Q_Buff so the application can read them later.
 *
 * Notes:
 *  - This function should be called cyclically only when FP_Curr_Mode == FP_SEARCH_MODE.
 *  - It restarts automatically after each match or no-match event.
 *  - It is designed to keep the system continuously ready for fingerprint scanning.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_SEARCH_SM(void)
{
    /* Current search state.
     * Static preserves state across cyclic calls.
     */
    static FP_Search_St_t FP_Search_St = FP_S_SEND_GET_IMG_CMD;

    /* Holds matched user ID returned from the module */
    static uint16_t user_id = 0;

    /* Stores reference time for timeout supervision */
    static uint32_t start = 0;

    switch (FP_Search_St)
    {
        case FP_S_SEND_GET_IMG_CMD:
        {
            /* Send generation image command to collect user finger */
            FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);

            /* Save current time to measure response timeout */
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
                    /* Finger image captured successfully */
                    FP_Search_St = FP_S_SEND_GEN_CHAR_1_CMD;
                }
                else
                {
                    /* Retry from the beginning if image capture failed */
                    FP_Search_St = FP_S_SEND_GET_IMG_CMD;
                }
            }
            else
            {
                //current - start > 1000
                if (HAL_GetTick() - start > 2000)
                {
                    /* Timeout waiting for module response */
                    FP_Search_St = FP_S_SEND_GET_IMG_CMD;
                }
            }

            break;
        }

        case FP_S_SEND_GEN_CHAR_1_CMD:
        {
            /* Generate character file in buffer 1 from captured image */
            uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
            FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);

            /* Restart timeout counter */
            start = HAL_GetTick(); // 100
            FP_Search_St = FP_S_WAIT_GEN_CHAR_1_CMD;
            break;
        }

        case FP_S_WAIT_GEN_CHAR_1_CMD:
        {
            /* Wait for module response after generating character file */
            if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
            {
                /* Check if get the image successful */
                if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
                {
                    /* Character file ready, proceed to search database */
                    FP_Search_St = FP_S_SEND_SEARCH_CMD;
                }
                else
                {
                    /* Restart search flow if character generation failed */
                    FP_Search_St = FP_S_SEND_GET_IMG_CMD;
                }
            }
            else
            {
                //current - start > 1000
                if (HAL_GetTick() - start > 2000)
                {
                    /* Timeout waiting for module response */
                    FP_Search_St = FP_S_SEND_GET_IMG_CMD;
                }
            }

            break;
        }

        case FP_S_SEND_SEARCH_CMD:
        {
            /* Payload format:
             * [0] = character buffer number
             * [1] = start page high
             * [2] = start page low
             * [3] = search count high
             * [4] = search count low
             *
             * This configuration searches:
             *   - starting from page 0x0000
             *   - across 0x0064 pages = 100 pages
             */
            uint8_t search_payload[5] = {FP_CHAR_FILE_1, 0x00, 0x00, 0x00, 0x64};
            FP_SendCommand(FP_SEARCH_CMD, search_payload, 5);

            /* Restart timeout counter */
            start = HAL_GetTick(); // 100
            FP_Search_St = FP_S_WAIT_SEARCH_CMD;
            break;
        }

        case FP_S_WAIT_SEARCH_CMD:
        {
            /* Wait for search result response */
            if (FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
            {
                /* Check if get the image successful */
                if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_MATCH)
                {
                    /* Extract matched page/user ID from returned payload */
                    user_id = ((FP_Rx_Buff[1] << 8) | FP_Rx_Buff[2]);
                    FP_Search_St = FP_S_MATCH;
                }
                else if (FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_NOT_MATCH)
                {
                    /* Search completed but there was no match */
                    FP_Search_St = FP_S_NOT_MATCH;
                }
            }
            else
            {
                //current - start > 1000
                if (HAL_GetTick() - start > 2000)
                {
                    /* Timeout while waiting for search response */
                    FP_Search_St = FP_S_SEND_GET_IMG_CMD;
                }
            }

            break;
        }

        case FP_S_MATCH:
        {
            /* Prepare queue item for successful match */
            FP_Search_Data_t FP_Search_Data;
            FP_Search_Data.match_st = FP_MATCH_ST;
            FP_Search_Data.user_id = user_id;

            /* Send result to queue without blocking */
            xQueueSend(FP_Search_Q_Buff, &FP_Search_Data, 0);

            /* Restart search process for next fingerprint */
            FP_Search_St = FP_S_SEND_GET_IMG_CMD;
            break;
        }

        case FP_S_NOT_MATCH:
        {
            /* Prepare queue item for no-match result */
            FP_Search_Data_t FP_Search_Data;
            FP_Search_Data.match_st = FP_NOT_MATCH_ST;
            FP_Search_Data.user_id = 0;

            /* Send result to queue without blocking */
            xQueueSend(FP_Search_Q_Buff, &FP_Search_Data, 0);

            /* Restart search process for next fingerprint */
            FP_Search_St = FP_S_SEND_GET_IMG_CMD;
            break;
        }
    }
}


/******************************************************************************************
 *                                  FP_Get_User()
 *
 * Purpose:
 *  Retrieves the latest fingerprint search result from the queue.
 *
 * Description:
 *  - Reads one search result entry from the FreeRTOS queue created during initialization.
 *  - If a new search result is available, the function copies:
 *      - match status
 *      - user ID
 *    into the caller-provided output variables.
 *  - If no result is available, it returns a failure status.
 *
 * Parameters:
 *  match_st : Pointer to variable where the match status will be stored.
 *  user_id  : Pointer to variable where the matched user ID will be stored.
 *
 * Notes:
 *  - This function performs a non-blocking queue receive.
 *  - It is intended to be called by the application layer.
 *  - Returned user_id is meaningful only if match_st indicates a successful match.
 *
 * Returns:
 *  - FP_GetUser_Ok  : A search result was available and copied successfully.
 *  - FP_GetUser_NOk : No new search result available.
 ******************************************************************************************/
FP_Err_St_t FP_Get_User(uint8_t *match_st, uint16_t *user_id)
{
    /* Default return assumes no new user result is available */
    FP_Err_St_t FP_Err_St = FP_GetUser_NOk;
    FP_Search_Data_t ret_dat;

    /* Try to receive one item from the queue without blocking */
    if (xQueueReceive(FP_Search_Q_Buff, &ret_dat, 0) == pdPASS)
    {
        /* Copy the received search result to output parameters */
        *match_st = ret_dat.match_st;
        *user_id = ret_dat.user_id;
        FP_Err_St = FP_GetUser_Ok;
    }
    else
    {
        /* No result available in the queue */
        FP_Err_St = FP_GetUser_NOk;
    }

    return FP_Err_St;
}


/******************************************************************************************
 *                              FP_MainFunction_Cyclic()
 *
 * Purpose:
 *  Main cyclic function of the fingerprint driver.
 *
 * Description:
 *  - This function is the top-level periodic function that should be called
 *    regularly by the application scheduler, super loop, or RTOS task.
 *  - It first services the RX state machine by calling FP_Rx_Cyclic().
 *  - Then it runs the state machine corresponding to the current driver mode:
 *      - Enrollment state machine
 *      - Search state machine
 *
 * Execution Flow:
 *  1. Receive and parse incoming UART bytes.
 *  2. Depending on the current mode, execute the required state machine.
 *
 * Notes:
 *  - This is the main driver entry point for periodic execution.
 *  - It should be called frequently enough to avoid missing UART processing
 *    and to keep the state machines responsive.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_MainFunction_Cyclic(void)
{
    /* Always process RX first so newly received bytes are parsed
     * before any higher-level state machine checks packet readiness.
     */
    FP_Rx_Cyclic();

    /* Run the appropriate state machine depending on the selected mode */
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


/******************************************************************************************
 *                                FP_SimpleTesT()
 *
 * Purpose:
 *  Performs a very simple communication test with the fingerprint sensor.
 *
 * Description:
 *  - This function is mainly intended for quick debugging/testing.
 *  - On the first call, it:
 *      1. Initializes the driver.
 *      2. Sends a single "generate image" command.
 *  - On later calls, it:
 *      1. Processes incoming UART bytes.
 *      2. Checks if a full valid response packet is available.
 *      3. Prints debug messages if a finger is detected.
 *      4. Sends the same command again to continue testing.
 *
 * Notes:
 *  - This function is not the main production workflow.
 *  - It is useful for verifying:
 *      - UART communication
 *      - packet reception
 *      - basic fingerprint module response
 *  - It uses printf() for debug output.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_SimpleTesT(void)
{
	static uint8_t flag = 0;

	/* Initialize and send the first test command only once */
	if(flag == 0)
	{
		FP_Err_St_t FP_Err_St = FP_Init();

		/* Keep compiler quiet if this variable is not used during simple test */
		(void)FP_Err_St;

		//			HAL_Delay(1000);

		/* Send a simple get-image command as communication test */
		FP_SendCommand(FP_GEN_IMG_CMD , NULL , 0);
		flag = 1;
	}

	/* Keep receiving and parsing bytes */
	FP_Rx_Cyclic();

	/* Check whether a full valid response packet is available */
	if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
	{
		/* Confirmation code 0x00 means finger detected successfully */
		if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == 0x00)
		{
			printf("Detected finger");
		}

		/* Send the same command again to continue the simple test loop */
		FP_SendCommand(FP_GEN_IMG_CMD , NULL , 0);

		//			HAL_Delay(150);
		printf("Simple Test Success");
	}
}
