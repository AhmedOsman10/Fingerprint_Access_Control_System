/*
 * FP_Prv.h
 *
 *  Created on: 4 Feb 2026
 *      Author: Ahmed
 *
 *  Private definitions for the Fingerprint driver.
 *
 *  This header contains the internal macros, internal state types, and
 *  internal helper APIs used only inside the fingerprint driver.
 *
 *  This file is not intended for direct use by the application layer.
 *  The application layer shall use FP.h only.
 */

#ifndef FP_FP_PRV_H_
#define FP_FP_PRV_H_


/******************************************************************************************
 *                          Fingerprint Packet Fixed Bytes
 *
 *  These macros define the fixed fields used in the fingerprint packet protocol.
 *
 *  FP_HEADER_H:  High byte of the fingerprint packet header.
 *  FP_HEADER_L:  Low byte of the fingerprint packet header.
 *
 *  FP_PID_CMD:   Packet Identifier used when the MCU sends a command packet.
 *  FP_PID_ACK:   Packet Identifier used when the sensor returns an ACK packet.
 ******************************************************************************************/
#define FP_HEADER_H  0xEF
#define FP_HEADER_L	 0x01
#define FP_PID_CMD	 0x01
#define FP_PID_ACK	 0x07


/******************************************************************************************
 *                          Fingerprint Instruction Codes
 *
 *  These macros define the command codes sent to the fingerprint module.
 *
 *  FP_GEN_IMG_CMD:        Capture a fingerprint image from the sensor.
 *  FP_GEN_FILE_CHAR_CMD:  Convert the captured image into a character file.
 *  FP_MERGE_TWO_IMG_CMD:  Merge two character files into one fingerprint template.
 *  FP_STORE_IMG_CMD:      Store the merged template in sensor flash memory.
 *  FP_SEARCH_CMD:         Search the stored fingerprint library.
 ******************************************************************************************/
#define FP_GEN_IMG_CMD        0x01
#define FP_GEN_FILE_CHAR_CMD  0x02
#define FP_MERGE_TWO_IMG_CMD  0x05  /* RegModel command */
#define FP_STORE_IMG_CMD      0x06  /* Store command */
#define FP_SEARCH_CMD         0x04  /* Search command */


/******************************************************************************************
 *                          Packet Field Length Macros
 *
 *  These macros define constant field sizes used during packet construction and packet parsing.
 *
 *  CHECK_SUM_LEN:  Checksum field length in bytes.
 *  INST_CODE_LEN:  Instruction code field length in bytes.
 ******************************************************************************************/
#define CHECK_SUM_LEN   2
#define INST_CODE_LEN   1


/******************************************************************************************
 *                      Fingerprint Response / Confirmation Codes
 *
 *  These macros define response values returned by the fingerprint module.
 *
 *  In most ACK packets, the first payload byte is the confirmation code.
 *  This value indicates whether the requested operation succeeded or failed.
 *
 *  FP_CONFIRM_CODE_INDX:  Index of the confirmation code inside FP_Rx_Buff[].
 *
 *  FP_GET_FINGER_OK:      Finger image capture succeeded.
 *  FP_GET_FINGER_NOK:     Finger image capture failed.
 *  FP_NO_FINGER:          No finger detected on the sensor.
 *
 *  FP_MERGE_SUCCESS:      Merging two character files succeeded.
 *  FP_STORE_SUCCESS:      Template storage in flash succeeded.
 *  FP_GEN_CHAR_OK:        Character file generation succeeded.
 *
 *  FP_SEARCH_MATCH:       Search found a stored fingerprint match.
 *  FP_SEARCH_NOT_MATCH:   Search completed with no match found.
 ******************************************************************************************/
#define FP_CONFIRM_CODE_INDX       0x00
#define FP_GET_FINGER_OK           0x00
#define FP_GET_FINGER_NOK          0x03
#define FP_NO_FINGER               0x02
#define FP_MERGE_SUCCESS           0x00
#define FP_STORE_SUCCESS           0x00
#define FP_GEN_CHAR_OK             0x00
#define FP_SEARCH_MATCH            0x00
#define FP_SEARCH_NOT_MATCH        0x09


/******************************************************************************************
 *                          Character Buffer Selection
 *
 *  These macros define the internal character buffer IDs used by the
 *  fingerprint module.
 *
 *  FP_CHAR_FILE_1:  Character buffer 1.
 *  FP_CHAR_FILE_2:  Character buffer 2.
 *
 *  Typical enrollment flow:
 *    - First fingerprint image  -> FP_CHAR_FILE_1
 *    - Second fingerprint image -> FP_CHAR_FILE_2
 *    - Merge both into one final template
 ******************************************************************************************/
#define FP_CHAR_FILE_1   0x01
#define FP_CHAR_FILE_2   0x02

#define FRAME_COMPLETED 1
#define  FRAME_NOT_COMPLETED 0

#define FP_EMPTY_DATABASE_CMD   0x0D


/******************************************************************************************
 *                                  FP_Rx_St_t
 *
 *  Internal RX state machine states used by FP_Rx_Cyclic().
 *
 *  This state machine parses the fingerprint response packet byte by byte.
 *
 *  State flow:
 *    FP_Wait_Rx_Header_H:       Waiting for first header byte.
 *    FP_Wait_Rx_Header_L:       Waiting for second header byte.
 *    FP_Wait_Rx_ADDR:           Receiving the 4 address bytes.
 *    FP_Wait_Rx_Pid:            Waiting for ACK packet identifier.
 *    FP_Wait_Rx_len_H:          Waiting for packet length high byte.
 *    FP_Wait_Rx_len_L:          Waiting for packet length low byte.
 *    FP_Wait_Rx_Data:           Receiving payload and checksum bytes.
 *    FP_Wait_Rx_Frame_Complete: Full valid frame received.
 ******************************************************************************************/
typedef enum FP_Rx_St_e
{
	FP_Wait_Rx_Header_H,        /* Waiting for first header byte */
	FP_Wait_Rx_Header_L,        /* Waiting for second header byte */
	FP_Wait_Rx_ADDR,            /* Receiving 4 address bytes */
	FP_Wait_Rx_Pid,             /* Waiting for ACK packet identifier */
	FP_Wait_Rx_len_H,           /* Waiting for packet length high byte */
	FP_Wait_Rx_len_L,           /* Waiting for packet length low byte */
	FP_Wait_Rx_Data,            /* Receiving payload + checksum bytes */
	FP_Wait_Rx_Frame_Complete,  /* Full valid frame received */
}FP_Rx_St_t;


/******************************************************************************************
 *                                FP_Enroll_St_t
 *
 *  Internal enrollment state machine states used by FP_Enroll_SM().
 *
 *  This state machine manages the full enrollment sequence in a non-blocking way.
 *
 *  Internal flow:
 *    - start enrollment
 *    - capture first fingerprint image
 *    - generate first character file
 *    - check finger lift
 *    - capture second fingerprint image
 *    - generate second character file
 *    - merge both character files
 *    - store template in flash
 *    - report success or failure
 *
 *  SEND states:
 *    Send the command to the module.
 *
 *  WAIT states:
 *    Wait for and evaluate the module response.
 ******************************************************************************************/
typedef enum FP_Enroll_St_e
{
	FP_E_Idle,                  /* Enrollment inactive */
	FP_E_Start,                 /* Enrollment start point */

	FP_E_SEND_GET_IMG_1_CMD,    /* Send first image capture command */
	FP_E_WAIT_GET_IMG_1_CMD,    /* Wait for first image capture response */

	FP_E_SEND_GEN_CHAR_1_CMD,   /* Send first character generation command */
	FP_E_WAIT_GEN_CHAR_1_CMD,   /* Wait for first character generation response */

	FP_E_SEND_GET_IMG_2_CMD,    /* Send second image capture command */
	FP_E_WAIT_GET_IMG_2_CMD,    /* Wait for second image capture response */

	FP_E_SEND_GEN_CHAR_2_CMD,   /* Send second character generation command */
	FP_E_WAIT_GEN_CHAR_2_CMD,   /* Wait for second character generation response */

	FP_E_SEND_LIFT_CHECK_CMD,   /* Send finger-lift check command */
	FP_E_WAIT_LIFT_CHECK_CMD,   /* Wait for finger-lift check response */

	FP_E_SEND_MERGE_CMD,        /* Send merge command */
	FP_E_WAIT_MERGE_CMD,        /* Wait for merge response */

	FP_E_SEND_STORE_CMD,        /* Send store template command */
	FP_E_WAIT_STORE_CMD,        /* Wait for store response */

	FP_E_Success,               /* Enrollment completed successfully */
	FP_E_Failed,                /* Enrollment failed */
}FP_Enroll_St_t;


/******************************************************************************************
 *                                FP_Search_St_t
 *
 *  Internal search state machine states used by FP_SEARCH_SM().
 *
 *  This state machine manages the fingerprint search sequence in a non-blocking way.
 *
 *  Internal flow:
 *    - capture fingerprint image
 *    - generate character file
 *    - search stored fingerprint database
 *    - report match or no match
 ******************************************************************************************/
typedef enum FP_Search_St_e
{
	FP_S_SEND_GET_IMG_CMD,      /* Send image capture command */
	FP_S_WAIT_GET_IMG_CMD,      /* Wait for image capture response */

	FP_S_SEND_GEN_CHAR_1_CMD,   /* Send character generation command */
	FP_S_WAIT_GEN_CHAR_1_CMD,   /* Wait for character generation response */

	FP_S_SEND_SEARCH_CMD,       /* Send search command */
	FP_S_WAIT_SEARCH_CMD,       /* Wait for search response */

	FP_S_MATCH,                 /* Search result: match found */
	FP_S_NOT_MATCH,             /* Search result: no match found */
}FP_Search_St_t;


/******************************************************************************************
 *                              Internal Driver APIs
 *
 *  These functions are private helper APIs used only inside the
 *  fingerprint driver implementation.
 *
 *  FP_SendCommand():
 *      Build and send a fingerprint command packet.
 *
 *  FP_Enroll_SM():
 *      Run the internal enrollment state machine.
 *
 *  FP_SEARCH_SM():
 *      Run the internal search state machine.
 *
 *  FP_CheckPacket():
 *      Check whether a full valid response packet is available.
 ******************************************************************************************/



/******************************************************************************************
 *                                  FP_SendCommand()
 *
 *  Build and send a command packet to the fingerprint module.
 *
 *  Description:
 *    This function constructs a full fingerprint protocol packet using:
 *      - header
 *      - address
 *      - packet identifier (PID)
 *      - length
 *      - instruction code
 *      - optional payload
 *      - checksum
 *
 *    The packet is then transmitted byte-by-byte using the configured USART.
 *
 *  Internal Use:
 *    - Used by enrollment and search state machines
 *    - Called in SEND states of the driver
 *
 *  Parameters:
 *    inst_code:   Instruction code to be sent to the module.
 *
 *    payload:     Pointer to optional payload data.
 *                 Can be NULL if no payload is required.
 *
 *    payload_len: Number of payload bytes.
 *
 *  Returns:
 *    FP_SendCmd_Success: Packet was built and transmitted successfully.
 *    FP_SendCmd_Failed:  Transmission failed.
 *
 *  Note:
 *    This function does not wait for a response.
 *    Response handling is performed separately by the RX state machine.
 ******************************************************************************************/
FP_Err_St_t FP_SendCommand(uint8_t inst_code , uint8_t *payload , uint8_t payload_len);



/******************************************************************************************
 *                                  FP_Enroll_SM()
 *
 *  Run the internal enrollment state machine.
 *
 *  Description:
 *    This function implements the full fingerprint enrollment sequence
 *    in a non-blocking, cyclic manner.
 *
 *    The sequence includes:
 *      - capturing two fingerprint images
 *      - generating character files
 *      - checking finger lift
 *      - merging character files
 *      - storing the final template in flash
 *
 *  Internal Behavior:
 *    - Uses FP_Enroll_St_t to track current state
 *    - Alternates between SEND and WAIT states
 *    - Transitions are based on received responses from the module
 *
 *  Internal Use:
 *    - Called from FP_MainFunction_Cyclic()
 *    - Executed only when driver mode is FP_ENROLL_MODE
 *
 *  Note:
 *    - This function must be called periodically
 *    - It does not block while waiting for responses
 ******************************************************************************************/
void FP_Enroll_SM(void);



/******************************************************************************************
 *                                  FP_SEARCH_SM()
 *
 *  Run the internal fingerprint search state machine.
 *
 *  Description:
 *    This function implements the fingerprint search sequence
 *    in a non-blocking, cyclic manner.
 *
 *    The sequence includes:
 *      - capturing fingerprint image
 *      - generating character file
 *      - searching stored fingerprint database
 *
 *  Internal Behavior:
 *    - Uses FP_Search_St_t to track current state
 *    - Alternates between SEND and WAIT states
 *    - Evaluates responses to determine match or no match
 *
 *  Internal Use:
 *    - Called from FP_MainFunction_Cyclic()
 *    - Executed only when driver mode is FP_SEARCH_MODE
 *
 *  Output:
 *    - On completion, result is stored internally
 *    - Result is later accessed via FP_Get_User()
 *
 *  Note:
 *    - This function must be called periodically
 *    - It does not block while waiting for responses
 ******************************************************************************************/
void FP_SEARCH_SM(void);



/******************************************************************************************
 *                                  FP_CheckPacket()
 *
 *  Check whether a full valid fingerprint response packet is available.
 *
 *  Description:
 *    This function evaluates the RX state machine status to determine
 *    whether a complete and valid packet has been received.
 *
 *  Internal Behavior:
 *    - Checks if RX state reached "frame complete"
 *    - Validates that the packet is ready for processing
 *
 *  Internal Use:
 *    - Used by WAIT states in both enrollment and search state machines
 *    - Used to decide when to process a received response
 *
 *  Returns:
 *    FP_Rx_Full_Packet_Ok:  A complete valid packet is available.
 *    FP_Rx_Full_Packet_Nok: No complete packet available yet.
 *
 *  Note:
 *    - Packet parsing itself is handled by FP_Rx_Cyclic()
 *    - This function only checks availability/status
 ******************************************************************************************/
FP_Err_St_t FP_CheckPacket(void);


#endif /* FP_FP_PRV_H_ */
