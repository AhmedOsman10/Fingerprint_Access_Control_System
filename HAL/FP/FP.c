/*
 * FP.c
 *
 *  Created on: 2 Feb 2026
 *      Author: Ahmed
 *
 *  Fingerprint driver implementation.
 *
 *  This file contains the internal runtime logic of the fingerprint driver.
 *  It handles:
 *    - command packet transmission
 *    - response packet reception and parsing
 *    - enrollment state machine
 *    - search state machine
 *    - public API implementation
 *
 *  Communication with the fingerprint module is performed over USART.
 *
 *  Fingerprint Packet Format
 *  ----------------------------------------------------------------------------
 *  Header (2) + Address (4) + PID (1) + Length (2) + Instruction Code (1) +
 *  Data (N) + Checksum (2)
 *
 *  Length   = Instruction + Data + Checksum
 *  Checksum = arithmetic sum of:
 *               PID + Length (2 bytes) + Instruction + Data
 *  ----------------------------------------------------------------------------
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
#include "EEPROM.h"

/* Enrollment state machine current state.
 *
 * This variable tracks the current step of the enrollment process.
 * The enrollment logic progresses step by step across multiple cyclic calls.
 */
static FP_Enroll_St_t FP_Enroll_St = FP_E_Idle;

/* RX buffer used to store the received bytes after the Length field.
 *
 * Why do we store only bytes after Length?
 * Because the earlier fields such as:
 *   - Header
 *   - Address
 *   - PID
 * are only needed for packet validation and synchronization.
 *
 * The bytes after Length are important because they contain:
 *   - confirmation code
 *   - returned payload data
 *   - checksum bytes
 */
static uint8_t FP_Rx_Buff[30];

/* General RX index used by the receive state machine.
 *
 * It is reused in multiple RX states:
 *   - to count the 4 address bytes
 *   - to index payload/checksum bytes inside FP_Rx_Buff[]
 */
static uint16_t FP_Rx_Indx = 0;

/* Packet length extracted from the received response.
 *
 * This value represents the number of bytes after the Length field.
 * In other words:
 *   payload + checksum
 */
static uint16_t FP_Rx_Packect_Len = 0;

/* Current public driver mode.
 *
 * The driver starts in search mode by default.
 */
static FP_Mode_t FP_Curr_Mode = FP_SEARCH_MODE;

/* Next fingerprint page used during enrollment.
 *
 * This value represents the next storage location inside the fingerprint sensor database where a new fingerprint template will be saved.
 *
 * When enrollment succeeds:
 *   - the template is stored at FP_NextPage
 *   - then FP_NextPage is incremented
 *   - the updated value is saved into EEPROM
 *
 * EEPROM storage allows the system to remember the next available page even after reset or power loss.
 */
static uint16_t FP_NextPage = 1;

/* Flag indicating that a full valid RX frame has been received and verified.
 *
 * Set by:   FP_Rx_Cyclic()   (after checksum validation)
 * Cleared by: FP_CheckPacket() (after application reads the frame)
 *
 * volatile is used to prevent compiler optimization issues because
 * this variable is shared between different execution contexts
 * (RX processing and application logic), and may later be updated
 * from interrupt or RTOS task context.
 */
static volatile uint8_t FP_Rx_Frame_Ready_Flag = FRAME_NOT_COMPLETED;


/* Queue used to pass search results from the driver layer to the application layer.
 *
 * Search result data is written into this queue by FP_SEARCH_SM(),
 * and later read by the application through FP_Get_User().
 */
static QueueHandle_t FP_Search_Q_Buff;


/******************************************************************************************
 *                                  FP_Init()
 *
 *  Initialize the fingerprint driver.
 *
 *  Description:
 *    - Initializes the USART interface used for communication with the
 *      fingerprint module.
 *    - Creates the internal FreeRTOS queue used to transfer search results
 *      from the driver layer to the application layer.
 *    - Initializes EEPROM used for persistent fingerprint page storage.
 *    - Restores the last stored fingerprint page after power-up/reset.
 *
 *  Internal Behavior:
 *    - Calls USART_Init() for the configured USART instance.
 *    - Creates a queue of type FP_Search_Data_t.
 *    - Initializes EEPROM driver.
 *    - Reads FP_NextPage from EEPROM memory.
 *    - Initializes EEPROM page value if EEPROM is blank.
 *
 *  EEPROM Persistence:
 *    - FP_NextPage is stored in EEPROM so fingerprint IDs are preserved
 *      between resets and power cycles.
 *    - This prevents overwriting previously enrolled fingerprints.
 *
 *  Application Impact:
 *    - This function must be called once before using any driver API.
 *    - If this function fails, the driver is not operational.
 *
 *  Returns:
 *    FP_InitSuccess: Driver initialized successfully.
 *    FP_InitFailed:  Initialization failed (USART, queue, or EEPROM error).
 ******************************************************************************************/
FP_Err_St_t FP_Init(void)
{
	/* Default return value is failure until all required steps succeed */
	FP_Err_St_t fp_err_st = FP_InitFailed;

	/* Temporary variables used to reconstruct stored fingerprint page ID
	 * from EEPROM high and low bytes
	 */
	uint8_t next_page_h = 0;
	uint8_t next_page_l = 0;

	/* Default USART init state */
	USART_Err_St_t usart_err_st = USART_InitFailed;

	/* Initialize the configured USART channel used by the fingerprint driver.
	 *
	 * This USART carries all commands sent to the fingerprint module
	 * and all responses received from it.
	 */
	usart_err_st = USART_Init(FP_USART_NUM_);

	/* If USART was initialized successfully, the driver can continue initialization */
	if(usart_err_st == USART_InitSuccess)
	{
		fp_err_st = FP_InitSuccess;
	}

	/* Only create the queue if USART init succeeded */
	if(fp_err_st == FP_InitSuccess)
	{
		/* Create queue used to pass search results to the application layer.
		 *
		 * Queue length : 10 elements
		 * Element type : FP_Search_Data_t
		 *
		 * Why queue?
		 * Because the search state machine may detect results asynchronously,
		 * while the application may read them later.
		 */
		FP_Search_Q_Buff = xQueueCreate(10, sizeof(FP_Search_Data_t));

		/* If queue creation fails, the driver is not considered fully initialized */
		if(FP_Search_Q_Buff == NULL)
		{
			fp_err_st = FP_InitFailed;
		}
		else
		{
			/* Initialize EEPROM driver used for persistent storage */
			EEPROM_Err_St_t EEPROM_Err_St = EEPROM_Init();

			/* If EEPROM initialization fails,
			 * fingerprint page persistence cannot be guaranteed
			 */
			if(EEPROM_Err_St == EEPROM_Init_Failed)
			{
				fp_err_st = FP_InitFailed;
			}
			else
			{
				/* Read previously stored next fingerprint page number
				 * from EEPROM memory after power-up/reset
				 */
				EEPROM_Read_Byte(FP_EEPROM_NEXT_PAGE_H_MEM_ADDR, &next_page_h);

				/* Read low byte of stored page number */
				EEPROM_Read_Byte(FP_EEPROM_NEXT_PAGE_L_MEM_ADDR, &next_page_l);

				/* Reconstruct full 16-bit page ID from EEPROM bytes */
				FP_NextPage = (next_page_h << 8) | next_page_l;

				/* Fresh EEPROM devices commonly contain 0xFF in all bytes.
				 * If both EEPROM bytes are still 0xFF,
				 * the reconstructed page becomes 0xFFFF,
				 * which indicates no valid page has been stored yet.
				 */
				if(FP_NextPage == 0xFFFF)
				{
					/* Start fingerprint storage from page 1 */
					FP_NextPage = 1;

					/* Store initialized page value into EEPROM */
					EEPROM_Write_Byte(FP_EEPROM_NEXT_PAGE_H_MEM_ADDR, FP_NextPage >> 8);

					/* Store low byte of initialized page value */
					EEPROM_Write_Byte(FP_EEPROM_NEXT_PAGE_L_MEM_ADDR, FP_NextPage & 0xFF);
				}
			}
		}
	}

	return fp_err_st;
}


/******************************************************************************************
 *                                  FP_SendCommand()
 *
 *  Build and transmit a fingerprint command packet over USART.
 *
 *  Description:
 *    - Constructs a full protocol frame including:
 *        header, address, PID, length, instruction, payload, checksum.
 *    - Calculates checksum based on protocol definition.
 *    - Sends the packet byte-by-byte via USART.
 *
 *  Internal Use:
 *    - Used by enrollment and search state machines.
 *    - Called during SEND states.
 *
 *  Parameters:
 *    inst_code:   Instruction code to send.
 *    payload:     Pointer to payload data (NULL if not used).
 *    payload_len: Number of payload bytes.
 *
 *  Returns:
 *    FP_SendCmd_Success: Packet transmitted successfully.
 *    FP_SendCmd_Failed:  Transmission failed.
 *
 *  Note:
 *    - This function does not wait for a response.
 *    - Response handling is performed by the RX state machine.
 ******************************************************************************************/
FP_Err_St_t FP_SendCommand(uint8_t inst_code, uint8_t *payload, uint8_t payload_len)
{
	/* Default return assumes transmission will succeed */
	FP_Err_St_t FP_Err_St = FP_SendCmd_Success;

	/* Local transmission buffer used to build the full packet before sending */
	uint8_t tx[30];

	/* 16-bit checksum accumulator */
	uint16_t sum = 0;

	/* Packet length definition in this protocol:
	 *   Instruction Code (1 byte) + Payload (N bytes) + Checksum (2 bytes)
	 *
	 * The header, address, PID, and length fields themselves are not included
	 * in the Length field.
	 */
	uint16_t FP_Packet_len = payload_len + INST_CODE_LEN + CHECK_SUM_LEN;

	/* -------------------- Packet Build -------------------- */

	/* Fixed packet header */
	tx[0] = FP_HEADER_H;
	tx[1] = FP_HEADER_L;

	/* Default module address = 0xFFFFFFFF
	 *
	 * In many fingerprint modules, the default address is all 0xFF.
	 * This driver currently assumes that default address.
	 */
	tx[2] = 0xFF;
	tx[3] = 0xFF;
	tx[4] = 0xFF;
	tx[5] = 0xFF;

	/* Packet Identifier for command packet */
	tx[6] = FP_PID_CMD;

	/* Length field is transmitted in big-endian format:
	 *   high byte first, then low byte
	 */
	tx[7] = (FP_Packet_len >> 8);
	tx[8] = (FP_Packet_len & 0xFF);

	/* Instruction code */
	tx[9] = inst_code;

	/* Copy payload bytes after the instruction code.
	 *
	 * Payload is optional.
	 * If payload pointer is NULL, no copy is performed.
	 */
	for(uint8_t i = 0; i < payload_len; i++)
	{
		if(payload != NULL)
		{
			tx[10 + i] = payload[i];
		}
	}

	/* -------------------- Checksum Calculation -------------------- */

	/* Checksum is calculated from:
	 *   PID + Length(2 bytes) + Instruction + Payload
	 *
	 * In this packet layout, that corresponds to indices:
	 *   6 .. (9 + payload_len)
	 *
	 * Header and address are NOT included in checksum.
	 */
	for(uint8_t i = 6; i < (10 + payload_len); i++)
	{
		sum += tx[i];
	}

	/* Checksum is stored immediately after payload */
	uint8_t sum_indx_start = 10 + payload_len;

	/* Store checksum in big-endian format */
	tx[sum_indx_start]     = (sum >> 8);
	tx[sum_indx_start + 1] = (sum & 0xFF);

	/* Total number of bytes to transmit:
	 * from tx[0] up to the second checksum byte
	 */
	uint8_t frame_len = sum_indx_start + CHECK_SUM_LEN;

	/* -------------------- Transmission -------------------- */

	/* Send the full packet byte by byte using USART driver.
	 *
	 * The driver sends one byte at a time because USART is inherently byte-oriented.
	 */
	for(uint8_t i = 0; i < frame_len; i++)
	{
		/* If any byte fails to send, stop immediately and report failure */
		if(USART_SendByte(FP_USART_NUM_, tx[i]) != USART_Tx_Ok)
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
 *  Receive and parse fingerprint module response packets.
 *
 *  Description:
 *    - Implements a byte-by-byte RX state machine.
 *    - Reconstructs incoming packets from the UART stream.
 *    - Validates packet structure:
 *        header → address → PID → length → payload → checksum.
 *    - Stores payload + checksum in FP_Rx_Buff[].
 *    - Verifies checksum before marking packet as valid.
 *
 *  Internal Behavior:
 *    - Consumes one byte per state transition.
 *    - Resets to header state on any invalid condition.
 *    - Stops parsing when a full valid frame is received.
 *
 *  Packet Handling:
 *    - Only ACK packets are accepted.
 *    - Payload and checksum are stored for later processing.
 *
 *  Integration:
 *    - Must be called periodically.
 *    - Called from FP_MainFunction_Cyclic().
 *
 *  Interaction with Other Modules:
 *    - Packet readiness is checked using FP_CheckPacket().
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_Rx_Cyclic(void)
{
	/* RX state machine current state.
	 *
	 * This variable is used by FP_Rx_Cyclic() to track where the parser currently is
	 * in the incoming fingerprint response packet.
	 *
	 * Initial state:
	 *   wait for the first header byte of a new packet.
	 */
	static FP_Rx_St_t FP_Rx_St = FP_Wait_Rx_Header_H;

	/* Local checksum calculated by MCU from received packet fields */
	uint16_t calc_sum = 0;

	/* Checksum value received from the fingerprint sensor */
	uint16_t rec_sum = 0;

	/* Holds one received UART byte at a time */
	uint8_t byte = 0;

	/* Read as many bytes as are currently available from USART.
	 *
	 * This allows the cyclic function to consume all pending UART bytes
	 * each time it runs.
	 */
	while(USART_ReceiveByte(FP_USART_NUM_, &byte) == USART_Rx_Ok)
	{
		switch(FP_Rx_St)
		{
		case FP_Wait_Rx_Header_H:
		{
			/* Wait for the first header byte.
			 *
			 * Any byte that is not FP_HEADER_H is ignored.
			 * This lets the parser resynchronize with the next valid packet.
			 */
			if(byte == FP_HEADER_H)
			{
				FP_Rx_St = FP_Wait_Rx_Header_L;
			}
			break;
		}

		case FP_Wait_Rx_Header_L:
		{
			/* Wait for the second header byte.
			 *
			 * If it matches, packet synchronization is successful.
			 * If not, restart from header high.
			 */
			if(byte == FP_HEADER_L)
			{
				FP_Rx_St = FP_Wait_Rx_ADDR;
			}
			else
			{
				FP_Rx_St = FP_Wait_Rx_Header_H;
			}
			break;
		}

		case FP_Wait_Rx_ADDR:
		{
			/* Receive the 4-byte module address.
			 *
			 * This driver currently does not validate or store the address bytes.
			 * It only counts them to keep packet parsing aligned correctly.
			 */
			FP_Rx_Indx++;

			/* After 4 address bytes, move to PID state */
			if(FP_Rx_Indx == 4)
			{
				/* Reset index because it will be reused later for payload buffer */
				FP_Rx_Indx = 0;
				FP_Rx_St = FP_Wait_Rx_Pid;
			}
			break;
		}

		case FP_Wait_Rx_Pid:
		{
			/* Expect ACK packet identifier.
			 *
			 * This driver currently accepts only ACK packets.
			 */
			if(byte == FP_PID_ACK)
			{
				FP_Rx_St = FP_Wait_Rx_len_H;
			}
			else
			{
				/* Invalid packet type -> restart synchronization */
				FP_Rx_St = FP_Wait_Rx_Header_H;
			}
			break;
		}

		case FP_Wait_Rx_len_H:
		{
			/* Store high byte of packet length */
			FP_Rx_Packect_Len = (byte << 8);

			/* Next, receive the low byte */
			FP_Rx_St = FP_Wait_Rx_len_L;
			break;
		}

		case FP_Wait_Rx_len_L:
		{
			/* Complete the 16-bit packet length */
			FP_Rx_Packect_Len |= byte;

			/* After length is known, the remaining bytes are:
			 *   payload + checksum
			 */
			FP_Rx_St = FP_Wait_Rx_Data;
			break;
		}

		case FP_Wait_Rx_Data:
		{
			/* Store bytes after Length field.
			 *
			 * These bytes are important because they include:
			 *   - confirmation code
			 *   - returned payload data
			 *   - checksum bytes
			 */
			FP_Rx_Buff[FP_Rx_Indx] = byte;
			FP_Rx_Indx++;

			/* Once all payload + checksum bytes are received,
			 * calculate and verify checksum.
			 */
			if(FP_Rx_Indx == FP_Rx_Packect_Len)
			{
				/* Checksum calculation according to protocol:
				 *   PID + Length + Payload
				 *
				 * Here:
				 *   - PID is FP_PID_ACK
				 *   - Length is FP_Rx_Packect_Len
				 *   - Payload bytes are stored in FP_Rx_Buff[0 .. len-3]
				 *
				 * The last 2 bytes of FP_Rx_Buff are checksum bytes,
				 * so they are excluded from local checksum calculation.
				 */
				calc_sum = FP_PID_ACK + FP_Rx_Packect_Len;

				for(uint8_t i = 0; i < FP_Rx_Packect_Len - 2; i++)
				{
					calc_sum += FP_Rx_Buff[i];
				}

				/* Reconstruct received checksum from the last two bytes */
				rec_sum = (FP_Rx_Buff[FP_Rx_Packect_Len - 2] << 8) | FP_Rx_Buff[FP_Rx_Packect_Len - 1];

				/* Compare locally calculated checksum with received checksum */
				if(calc_sum == rec_sum)
				{
					/* Packet is valid and passed integrity check.
					 *
					 * At this point:
					 *  - Full frame has been received
					 *  - Checksum is correct
					 *  - FP_Rx_Buff[] contains valid payload + checksum
					 *
					 * The frame is now ready for higher-level processing.
					 */

					/* Reset index to prepare for next incoming frame */
					FP_Rx_Indx = 0;

					/* Restart RX state machine to look for next packet.
					 *
					 * We do NOT stay in "Frame Complete" state because
					 * packet availability is handled using a flag mechanism.
					 */
					FP_Rx_St = FP_Wait_Rx_Header_H;

					/* Signal to application layer that a full valid frame is ready.
					 *
					 * This flag will be read and cleared by FP_CheckPacket().
					 */
					FP_Rx_Frame_Ready_Flag = FRAME_COMPLETED;
				}
				else
				{
					/* Invalid checksum means packet corruption or bad alignment.
					 * Discard current frame and restart synchronization.
					 */
					FP_Rx_Indx = 0;
					FP_Rx_St = FP_Wait_Rx_Header_H;
				}
			}
			break;
		}
		}
	}
}


/******************************************************************************************
 *                                  FP_SetMode()
 *
 *  Set the current operating mode of the fingerprint driver.
 *
 *  Description:
 *    - Updates the global driver mode.
 *    - Resets the enrollment state machine.
 *    - If enrollment mode is selected, prepares the state machine to start.
 *
 *  Internal Behavior:
 *    - Always resets FP_Enroll_St to FP_E_Idle.
 *    - If mode == FP_ENROLL_MODE -> moves to FP_E_Start.
 *
 *  Parameters:
 *    mode: Desired driver mode:
 *          FP_SEARCH_MODE or FP_ENROLL_MODE.
 *
 *  Note:
 *    - Switching to search mode stops enrollment flow.
 *    - Switching to enroll mode restarts enrollment from the beginning.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_SetMode(FP_Mode_t mode)
{
	/* Update public driver mode */
	FP_Curr_Mode = mode;

	/* Any mode change resets enrollment state to known initial condition */
	FP_Enroll_St = FP_E_Idle;

	/* If enrollment mode is selected, prepare enrollment state machine to start */
	if(mode == FP_ENROLL_MODE)
	{
		FP_Enroll_St = FP_E_Start;
	}
}


/******************************************************************************************
 *                                  FP_GetMode()
 *
 *  Get the current fingerprint driver mode.
 *
 *  Description:
 *    - Returns the current operating mode of the driver.
 *
 *  Returns:
 *    FP_SEARCH_MODE: Driver is in search mode.
 *    FP_ENROLL_MODE: Driver is in enroll mode.
 ******************************************************************************************/
FP_Mode_t FP_GetMode(void)
{
	return FP_Curr_Mode;
}


/******************************************************************************************
 *                                  FP_CheckPacket()
 *
 *  Check if a full valid response packet is available.
 *
 *  Description:
 *    - Checks whether a complete and checksum-verified packet has been received.
 *    - Uses a flag set by the RX state machine (FP_Rx_Cyclic()).
 *    - Clears the flag after the packet is consumed.
 *
 *  Internal Behavior:
 *    - Evaluates FP_Rx_Frame_Ready_Flag.
 *    - If set, it indicates that a full valid frame is available in FP_Rx_Buff[].
 *    - Clears the flag to allow reception of the next packet.
 *
 *  Usage:
 *    - Used by WAIT states in enrollment and search state machines.
 *    - Acts as synchronization point between RX layer and logic layer.
 *
 *  Design Note:
 *    - This function does NOT depend on RX state anymore.
 *    - Packet readiness is managed using a flag instead of state machine state.
 *
 *  Returns:
 *    FP_Rx_Full_Packet_Ok:  Packet is ready and can be processed.
 *    FP_Rx_Full_Packet_Nok: No packet available.
 ******************************************************************************************/
FP_Err_St_t FP_CheckPacket(void)
{
	FP_Err_St_t FP_Err_St = FP_Rx_Full_Packet_Nok;

	/* Check if RX state machine has completed a valid frame */
	if(FP_Rx_Frame_Ready_Flag == FRAME_COMPLETED)
	{
		/* Notify caller that a valid packet is ready */
		FP_Err_St = FP_Rx_Full_Packet_Ok;

		/* Clear flag after reading to allow next frame reception */
		FP_Rx_Frame_Ready_Flag = FRAME_NOT_COMPLETED;
	}

	return FP_Err_St;
}


/******************************************************************************************
 *                                  FP_Enroll_SM()
 *
 *  Execute the fingerprint enrollment state machine.
 *
 *  Description:
 *    - Implements a non-blocking enrollment sequence.
 *    - Processes enrollment across multiple cyclic calls.
 *
 *  Enrollment Flow:
 *    1. Capture first fingerprint image.
 *    2. Generate first character file.
 *    3. Detect finger removal.
 *    4. Capture second fingerprint image.
 *    5. Generate second character file.
 *    6. Merge both images into a template.
 *    7. Store template in flash.
 *
 *  Internal Behavior:
 *    - Uses FP_SendCommand() for command transmission.
 *    - Uses FP_CheckPacket() for response synchronization.
 *    - Uses FP_Rx_Buff[] for confirmation code evaluation.
 *    - Uses retry and timeout mechanisms for robustness.
 *
 *  State Handling:
 *    - Alternates between SEND and WAIT states.
 *    - Progress depends on sensor responses.
 *
 *  Completion:
 *    - On success: increments FP_NextPage and returns to search mode.
 *    - On failure: returns to search mode.
 *
 *  Integration:
 *    - Called only when FP_Curr_Mode == FP_ENROLL_MODE.
 *    - Must be executed cyclically.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_Enroll_SM(void)
{
	/* Retry counter used while waiting for the first image capture response.
	 *
	 * static is used so the counter value is preserved across cyclic calls.
	 */
	static uint8_t err_c = 0;

	switch(FP_Enroll_St)
	{
	case FP_E_Idle:
	{
		/* No active enrollment operation */
		break;
	}

	case FP_E_Start:
	{
		/* Enrollment entry point:
		 * move directly to first image capture command
		 */
		FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
		break;
	}

	case FP_E_SEND_GET_IMG_1_CMD:
	{
		/* Request the module to capture the first fingerprint image */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);

		/* After sending, wait for response */
		FP_Enroll_St = FP_E_WAIT_GET_IMG_1_CMD;
		break;
	}

	case FP_E_WAIT_GET_IMG_1_CMD:
	{
		/* Wait for first image capture response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			/* Check confirmation code */
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				/* First fingerprint image captured successfully */
				FP_Enroll_St = FP_E_SEND_GEN_CHAR_1_CMD;
			}
			else
			{
				/* Finger not captured correctly -> retry first image capture */
				FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
			}
		}
		else
		{
			/* No valid packet yet -> count wait cycles */
			err_c++;

			/* If waiting too long, retry command */
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
		/* Convert first captured image into character file 1 */
		uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);

		/* Wait for response */
		FP_Enroll_St = FP_E_WAIT_GEN_CHAR_1_CMD;
		break;
	}

	case FP_E_WAIT_GEN_CHAR_1_CMD:
	{
		/* Wait for character-file generation response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				/* First character file generated successfully.
				 * Next step is to ensure user lifts finger before second scan.
				 */
				FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
			}
			else
			{
				/* Character generation failed -> restart from first image capture */
				FP_Enroll_St = FP_E_SEND_GET_IMG_1_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_LIFT_CHECK_CMD:
	{
		/* Reuse image capture command to detect if finger was removed.
		 *
		 * The module will return "no finger" when the user lifts finger.
		 */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_LIFT_CHECK_CMD;
		break;
	}

	case FP_E_WAIT_LIFT_CHECK_CMD:
	{
		/* Wait until module confirms no finger on sensor */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_NO_FINGER)
			{
				/* Finger successfully lifted -> proceed to second capture */
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
			else
			{
				/* Finger still present -> keep checking */
				FP_Enroll_St = FP_E_SEND_LIFT_CHECK_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_GET_IMG_2_CMD:
	{
		/* Capture second fingerprint image */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_GET_IMG_2_CMD;
		break;
	}

	case FP_E_WAIT_GET_IMG_2_CMD:
	{
		/* Wait for second image capture response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				/* Second image captured successfully */
				FP_Enroll_St = FP_E_SEND_GEN_CHAR_2_CMD;
			}
			else
			{
				/* Retry second capture */
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_GEN_CHAR_2_CMD:
	{
		/* Convert second captured image into character file 2 */
		uint8_t FP_Buff_2 = FP_CHAR_FILE_2;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_2, 1);
		FP_Enroll_St = FP_E_WAIT_GEN_CHAR_2_CMD;
		break;
	}

	case FP_E_WAIT_GEN_CHAR_2_CMD:
	{
		/* Wait for second character-file generation response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				/* Both character files are ready -> merge them */
				FP_Enroll_St = FP_E_SEND_MERGE_CMD;
			}
			else
			{
				/* Retry second capture flow */
				FP_Enroll_St = FP_E_SEND_GET_IMG_2_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_MERGE_CMD:
	{
		/* Merge character file 1 and character file 2 into one template */
		FP_SendCommand(FP_MERGE_TWO_IMG_CMD, NULL, 0);
		FP_Enroll_St = FP_E_WAIT_MERGE_CMD;
		break;
	}

	case FP_E_WAIT_MERGE_CMD:
	{
		/* Wait for merge response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_MERGE_SUCCESS)
			{
				/* Template created successfully -> store in flash */
				FP_Enroll_St = FP_E_SEND_STORE_CMD;
			}
			else
			{
				/* Retry merge if required */
				FP_Enroll_St = FP_E_SEND_MERGE_CMD;
			}
		}
		break;
	}

	case FP_E_SEND_STORE_CMD:
	{
		/* Store merged template in flash at FP_NextPage
		 *
		 * Payload format:
		 *   [0] Character buffer number
		 *   [1] Page ID high byte
		 *   [2] Page ID low byte
		 */

		uint8_t FP_Store_Payload[3] = {FP_CHAR_FILE_1, (FP_NextPage >> 8), (FP_NextPage & 0xFF)};

		FP_SendCommand(FP_STORE_IMG_CMD, FP_Store_Payload, 3);
		FP_Enroll_St = FP_E_WAIT_STORE_CMD;
		break;
	}

	case FP_E_WAIT_STORE_CMD:
	{
		/* Wait for store response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_STORE_SUCCESS)
			{
				/* Enrollment completed successfully */
				FP_Enroll_St = FP_E_Success;
			}
			else
			{
				/* Store operation failed */
				FP_Enroll_St = FP_E_Failed;
			}
		}
		break;
	}

	case FP_E_Success:
	{
		/* Increment page only on successful enrollment */
		FP_NextPage++;

		/* Save updated next available fingerprint page into EEPROM so enrolled users remain tracked after reset or power loss
		 */
		EEPROM_Write_Byte(FP_EEPROM_NEXT_PAGE_H_MEM_ADDR, FP_NextPage >> 8);

		/* Store low byte of next available page number */
		EEPROM_Write_Byte(FP_EEPROM_NEXT_PAGE_L_MEM_ADDR, FP_NextPage & 0xFF);

		/* Return driver back to normal search mode */
		FP_SetMode(FP_SEARCH_MODE);
		break;
	}

	case FP_E_Failed:
	{
		/* Return driver back to normal search mode after failure */
		FP_SetMode(FP_SEARCH_MODE);
		break;
	}
	}
}


/******************************************************************************************
 *                           FP_GetEnroll_Instruction()
 *
 *  Convert internal enrollment state into user-facing instruction.
 *
 *  Description:
 *    - Maps internal FP_Enroll_St states to high-level instructions.
 *    - Used by the application layer to update UI.
 *
 *  Behavior:
 *    - Returns FP_E_Inst_Idle if not in enrollment mode.
 *    - Otherwise returns instruction based on current state.
 *
 *  Usage:
 *    - Called periodically by application/UI layer.
 *
 *  Returns:
 *    FP_GetEnroll_Instruction_t representing current instruction.
 ******************************************************************************************/
FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void)
{
	/* If driver is not in enrollment mode, no enrollment instruction is active */
	if(FP_Curr_Mode != FP_ENROLL_MODE)
	{
		return FP_E_Inst_Idle;
	}

	/* Map internal enrollment states into user-facing instructions */
	switch(FP_Enroll_St)
	{
	case FP_E_SEND_GET_IMG_1_CMD:
	case FP_E_WAIT_GET_IMG_1_CMD:
	{
		/* User should place finger for first scan */
		return FP_E_Inst_Place_Finger;
	}

	case FP_E_SEND_LIFT_CHECK_CMD:
	case FP_E_WAIT_LIFT_CHECK_CMD:
	{
		/* User should lift finger */
		return FP_E_Inst_Lift_Finger;
	}

	case FP_E_SEND_GET_IMG_2_CMD:
	case FP_E_WAIT_GET_IMG_2_CMD:
	{
		/* User should place finger again for second scan */
		return FP_E_Inst_Place_Finger_Again;
	}

	case FP_E_SEND_GEN_CHAR_1_CMD:
	case FP_E_WAIT_GEN_CHAR_1_CMD:
	case FP_E_SEND_GEN_CHAR_2_CMD:
	case FP_E_WAIT_GEN_CHAR_2_CMD:
	case FP_E_SEND_MERGE_CMD:
	case FP_E_WAIT_MERGE_CMD:
	case FP_E_SEND_STORE_CMD:
	case FP_E_WAIT_STORE_CMD:
	{
		/* Internal processing step */
		return FP_E_Inst_Processing;
	}

	case FP_E_Success:
	{
		return FP_E_Inst_Success;
	}

	case FP_E_Failed:
	{
		return FP_E_Inst_Failed;
	}
	}

	/* Safety fallback */
	return FP_E_Inst_Idle;
}


/******************************************************************************************
 *                                  FP_SEARCH_SM()
 *
 *  Execute the fingerprint search state machine.
 *
 *  Description:
 *    - Implements continuous fingerprint matching in a non-blocking manner.
 *
 *  Search Flow:
 *    1. Capture fingerprint image.
 *    2. Generate character file.
 *    3. Search fingerprint database.
 *    4. Report match or no match.
 *
 *  Internal Behavior:
 *    - Uses HAL_GetTick() for timeout supervision.
 *    - Uses FP_SendCommand() for communication.
 *    - Uses FP_CheckPacket() for response detection.
 *    - Extracts user ID from FP_Rx_Buff[] on match.
 *
 *  Output:
 *    - Pushes results to FP_Search_Q_Buff queue.
 *
 *  Integration:
 *    - Called only when FP_Curr_Mode == FP_SEARCH_MODE.
 *    - Runs continuously.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_SEARCH_SM(void)
{
	/* Search state machine current state */
	static FP_Search_St_t FP_Search_St = FP_S_SEND_GET_IMG_CMD;

	/* Holds matched user/page ID returned by sensor on successful match */
	static uint16_t user_id = 0;

	/* Timestamp used for timeout checks in WAIT states */
	static uint32_t start = 0;

	switch(FP_Search_St)
	{
	case FP_S_SEND_GET_IMG_CMD:
	{
		/* Capture fingerprint image */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);

		/* Save start time for timeout supervision */
		start = HAL_GetTick();

		/* Move to response wait state */
		FP_Search_St = FP_S_WAIT_GET_IMG_CMD;
		break;
	}

	case FP_S_WAIT_GET_IMG_CMD:
	{
		/* Wait for image capture response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GET_FINGER_OK)
			{
				/* Image captured successfully -> convert to character file */
				FP_Search_St = FP_S_SEND_GEN_CHAR_1_CMD;
			}
			else
			{
				/* Capture failed or no valid finger -> retry */
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		else
		{
			/* If response takes too long, restart search flow */
			if(HAL_GetTick() - start > 2000)
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		break;
	}

	case FP_S_SEND_GEN_CHAR_1_CMD:
	{
		/* Convert captured image into character file 1 */
		uint8_t FP_Buff_1 = FP_CHAR_FILE_1;
		FP_SendCommand(FP_GEN_FILE_CHAR_CMD, &FP_Buff_1, 1);

		/* Restart timeout supervision */
		start = HAL_GetTick();
		FP_Search_St = FP_S_WAIT_GEN_CHAR_1_CMD;
		break;
	}

	case FP_S_WAIT_GEN_CHAR_1_CMD:
	{
		/* Wait for character generation response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_GEN_CHAR_OK)
			{
				/* Character file ready -> search database */
				FP_Search_St = FP_S_SEND_SEARCH_CMD;
			}
			else
			{
				/* Character generation failed -> restart flow */
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		else
		{
			if(HAL_GetTick() - start > 2000)
			{
				FP_Search_St = FP_S_SEND_GET_IMG_CMD;
			}
		}
		break;
	}

	case FP_S_SEND_SEARCH_CMD:
	{
		/* Search payload:
		 * [0] Character buffer
		 * [1] Start page high
		 * [2] Start page low
		 * [3] Page count high
		 * [4] Page count low
		 *
		 * Current configuration:
		 *   search from page 0
		 *   across 100 pages
		 */
		uint8_t search_payload[5] = {FP_CHAR_FILE_1, 0x00, 0x00, 0x00, 0x64};
		FP_SendCommand(FP_SEARCH_CMD, search_payload, 5);

		/* Restart timeout supervision */
		start = HAL_GetTick();
		FP_Search_St = FP_S_WAIT_SEARCH_CMD;
		break;
	}

	case FP_S_WAIT_SEARCH_CMD:
	{
		/* Wait for search response */
		if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
		{
			if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_MATCH)
			{
				/* Extract matched user/page ID from payload bytes [1] and [2] */
				user_id = ((FP_Rx_Buff[1] << 8) | FP_Rx_Buff[2]);
				FP_Search_St = FP_S_MATCH;
			}
			else if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == FP_SEARCH_NOT_MATCH)
			{
				FP_Search_St = FP_S_NOT_MATCH;
			}
		}
		else
		{
			/* Timeout -> restart search flow */
			if(HAL_GetTick() - start > 2000)
			{
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

		/* Restart search flow for next fingerprint */
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

		/* Restart search flow for next fingerprint */
		FP_Search_St = FP_S_SEND_GET_IMG_CMD;
		break;
	}
	}
}


/******************************************************************************************
 *                                  FP_Get_User()
 *
 *  Retrieve the latest fingerprint search result.
 *
 *  Description:
 *    - Reads one result from the internal FreeRTOS queue.
 *    - Copies match status and user ID to output parameters.
 *
 *  Behavior:
 *    - Non-blocking queue read.
 *    - Returns immediately if no data is available.
 *
 *  Parameters:
 *    match_st: Output pointer for match status.
 *    user_id:  Output pointer for matched user ID.
 *
 *  Returns:
 *    FP_GetUser_Ok:  Result retrieved successfully.
 *    FP_GetUser_NOk: No new result available.
 *
 *  Note:
 *    - user_id is valid only if match_st == FP_MATCH_ST.
 ******************************************************************************************/
FP_Err_St_t FP_Get_User(uint8_t *match_st, uint16_t *user_id)
{
	/* Default assumption: no new result available */
	FP_Err_St_t FP_Err_St = FP_GetUser_NOk;

	/* Local variable used to receive one queue entry */
	FP_Search_Data_t ret_dat;

	/* Try to read one item from queue without blocking */
	if(xQueueReceive(FP_Search_Q_Buff, &ret_dat, 0) == pdPASS)
	{
		/* Copy returned data to caller's output pointers */
		*match_st = ret_dat.match_st;
		*user_id = ret_dat.user_id;
		FP_Err_St = FP_GetUser_Ok;
	}
	else
	{
		/* No queue entry available */
		FP_Err_St = FP_GetUser_NOk;
	}

	return FP_Err_St;
}


/******************************************************************************************
 *                              FP_MainFunction_Cyclic()
 *
 *  Main periodic execution function of the fingerprint driver.
 *
 *  Description:
 *    - Acts as the central execution point of the driver.
 *    - Handles both RX processing and state machine execution.
 *
 *  Execution Flow:
 *    1. Call FP_Rx_Cyclic() to process incoming data.
 *    2. Execute state machine based on current mode:
 *         - FP_Enroll_SM()
 *         - FP_SEARCH_SM()
 *
 *  Integration:
 *    - Must be called periodically.
 *    - Can be used in:
 *        - super loop
 *        - scheduler
 *        - RTOS task
 *
 *  Note:
 *    - Driver functionality depends entirely on this function being called.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_MainFunction_Cyclic(void)
{
	/* Always process RX first.
	 *
	 * This ensures that any newly received UART bytes are parsed before
	 * the higher-level logic checks for packet readiness.
	 */
	FP_Rx_Cyclic();

	/* Run the logic corresponding to current driver mode */
	switch(FP_Curr_Mode)
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
 *  Perform basic communication test with the fingerprint module.
 *
 *  Description:
 *    - Initializes driver on first call.
 *    - Sends a simple command to detect fingerprint.
 *    - Prints debug output using printf().
 *
 *  Usage:
 *    - Intended for debugging and bring-up only.
 *
 *  Behavior:
 *    - Repeatedly sends command and checks response.
 *    - Confirms basic communication and module response.
 *
 *  Note:
 *    - Not intended for production use.
 *    - Uses blocking debug prints.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void FP_SimpleTesT(void)
{
	/* One-time init/test-start flag */
	static uint8_t flag = 0;

	/* First entry:
	 * initialize driver and send first test command
	 */
	if(flag == 0)
	{
		FP_Err_St_t FP_Err_St = FP_Init();

		/* Prevent compiler warning in case init result is not used in this test */
		(void)FP_Err_St;

		/* Send a basic image generation command for communication test */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		flag = 1;
	}

	/* Keep parser running */
	FP_Rx_Cyclic();

	/* If a full valid response packet is available */
	if(FP_CheckPacket() == FP_Rx_Full_Packet_Ok)
	{
		/* Confirmation code 0x00 indicates successful image capture */
		if(FP_Rx_Buff[FP_CONFIRM_CODE_INDX] == 0x00)
		{
			printf("Detected finger");
		}

		/* Repeat command to continue test loop */
		FP_SendCommand(FP_GEN_IMG_CMD, NULL, 0);
		printf("Simple Test Success");
	}
}
