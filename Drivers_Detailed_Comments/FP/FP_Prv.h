/*
 * FP_Prv.h
 *
 *  Created on: 4 Feb 2026
 *      Author: Ahmed
 */

#ifndef FP_FP_PRV_H_
#define FP_FP_PRV_H_

/******************************************************************************************
 *                          Fingerprint Packet Fixed Bytes
 *
 * Purpose:
 *  These macros define the fixed fields used in the fingerprint packet protocol.
 *
 * Description:
 *  - FP_HEADER_H / FP_HEADER_L:
 *      These are the 2 fixed header bytes at the start of each packet.
 *      They help the receiver synchronize with the beginning of a frame.
 *
 *  - FP_PID_CMD:
 *      Packet Identifier used when the MCU sends a command packet to the sensor.
 *
 *  - FP_PID_ACK:
 *      Packet Identifier used by the fingerprint sensor when it sends an
 *      acknowledgment/response packet back to the MCU.
 ******************************************************************************************/
#define FP_HEADER_H  0xEF
#define FP_HEADER_L	 0x01
#define FP_PID_CMD	 0x01
#define FP_PID_ACK	 0x07


/******************************************************************************************
 *                          Fingerprint Instruction Codes
 *
 * Purpose:
 *  These macros define the command codes sent to the fingerprint module.
 *
 * Description:
 *  Each command tells the fingerprint sensor to perform a specific action.
 *
 * Commands:
 *  FP_GEN_IMG_CMD:
 *      Ask the module to capture a fingerprint image from the sensor.
 *
 *  FP_GEN_FILE_CHAR_CMD:
 *      Convert the captured fingerprint image into a character file
 *      and store it in one of the sensor internal character buffers.
 *
 *  FP_SEARCH_CMD:
 *      Search the stored fingerprint library using a generated character file.
 *
 *  FP_MERGE_TWO_IMG_CMD:
 *      Merge the two generated character files into one fingerprint template/model.
 *      This is used during enrollment.
 *
 *  FP_STORE_IMG_CMD:
 *      Store the merged fingerprint template into the module flash memory.
 ******************************************************************************************/
#define FP_GEN_IMG_CMD        0x01
#define FP_GEN_FILE_CHAR_CMD  0x02
#define FP_MERGE_TWO_IMG_CMD  0x05  // reg model command
#define FP_STORE_IMG_CMD      0x06  // reg model command
#define FP_SEARCH_CMD         0x04  // Search command


/******************************************************************************************
 *                          Packet Field Length Macros
 *
 * Purpose:
 *  These macros define constant byte lengths used in packet construction.
 *
 * Description:
 *  - CHECK_SUM_LEN:
 *      Checksum field length in bytes.
 *
 *  - INST_CODE_LEN:
 *      Instruction code field length in bytes.
 *
 * Notes:
 *  These values are used when calculating packet length during transmission.
 ******************************************************************************************/
#define CHECK_SUM_LEN   2
#define INST_CODE_LEN   1


/******************************************************************************************
 *                      Fingerprint Response / Confirmation Codes
 *
 * Purpose:
 *  These macros define important response values returned by the fingerprint module.
 *
 * Description:
 *  The first payload byte in many ACK packets is the confirmation code.
 *  This code tells whether the requested command succeeded or failed.
 *
 * Values:
 *  FP_CONFIRM_CODE_INDX:
 *      Index of the confirmation code inside FP_Rx_Buff[].
 *
 *  FP_GET_FINGER_OK:
 *      Finger image capture succeeded.
 *
 *  FP_GET_FINGER_NOK:
 *      Finger image capture failed.
 *
 *  FP_NO_FINGER:
 *      No finger detected on the sensor.
 *
 *  FP_MERGE_SUCCESS:
 *      Merge of two character files succeeded.
 *
 *  FP_STORE_SUCCESS:
 *      Template storage into flash succeeded.
 *
 *  FP_GEN_CHAR_OK:
 *      Character file generation succeeded.
 *
 *  FP_SEARCH_MATCH:
 *      Search found a matching stored fingerprint.
 *
 *  FP_SEARCH_NOT_MATCH:
 *      Search completed but no match was found.
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
 * Purpose:
 *  These macros define the character buffer IDs inside the fingerprint module.
 *
 * Description:
 *  The sensor usually has two temporary character buffers:
 *    - Buffer 1
 *    - Buffer 2
 *
 *  During enrollment:
 *    - First fingerprint image is converted into character file 1
 *    - Second fingerprint image is converted into character file 2
 *    - Then both are merged into one template
 ******************************************************************************************/
#define FP_CHAR_FILE_1   0x01
#define FP_CHAR_FILE_2   0x02


/************************ FP Rx cyclic states **********/

/******************************************************************************************
 *                                  FP_Rx_St_t
 *
 * Purpose:
 *  This enum defines the states of the fingerprint RX state machine.
 *
 * Description:
 *  - This state machine is used inside FP_Rx_Cyclic().
 *  - It parses the received UART response packet one byte at a time.
 *  - Each state corresponds to one field or one stage in the packet.
 *
 * State Flow:
 *  FP_Wait_Rx_Header_H
 *      -> waiting for first packet header byte
 *
 *  FP_Wait_Rx_Header_L
 *      -> waiting for second packet header byte
 *
 *  FP_Wait_Rx_ADDR
 *      -> receiving the 4 address bytes
 *
 *  FP_Wait_Rx_Pid
 *      -> waiting for ACK packet identifier
 *
 *  FP_Wait_Rx_len_H
 *      -> waiting for packet length high byte
 *
 *  FP_Wait_Rx_len_L
 *      -> waiting for packet length low byte
 *
 *  FP_Wait_Rx_Data
 *      -> receiving payload + checksum bytes
 *
 *  FP_Wait_Rx_Frame_Complete
 *      -> full packet received and checksum verified successfully
 ******************************************************************************************/
typedef enum FP_Rx_St_e
{
	FP_Wait_Rx_Header_H ,
	FP_Wait_Rx_Header_L ,
	FP_Wait_Rx_ADDR,
	FP_Wait_Rx_Pid,
	FP_Wait_Rx_len_H,
	FP_Wait_Rx_len_L,
	FP_Wait_Rx_Data,
	FP_Wait_Rx_Frame_Complete,
}FP_Rx_St_t;


/******************************************************************************************
 *                                FP_Enroll_St_t
 *
 * Purpose:
 *  This enum defines the states of the fingerprint enrollment state machine.
 *
 * Description:
 *  - Used internally by FP_Enroll_SM().
 *  - Controls the full sequence of enrolling a new fingerprint.
 *  - Enrollment is implemented as a non-blocking cyclic state machine.
 *
 * Enrollment Sequence:
 *  1. Idle / Start
 *  2. Capture first fingerprint image
 *  3. Generate first character file
 *  4. Ask user to lift finger
 *  5. Capture second fingerprint image
 *  6. Generate second character file
 *  7. Merge both character files
 *  8. Store final template in flash
 *  9. Report success or failure
 *
 * Notes:
 *  States are split into SEND and WAIT so the driver stays non-blocking.
 *  SEND state:
 *      sends the command
 *  WAIT state:
 *      waits for and checks the module response
 ******************************************************************************************/
typedef enum FP_Enroll_St_e
{
	FP_E_Idle,
	FP_E_Start,

	// Get Finger image1 (Send command , Receive Response)
	FP_E_SEND_GET_IMG_1_CMD,
	FP_E_WAIT_GET_IMG_1_CMD,

    // Store Image 1 in CHAR 1
	FP_E_SEND_GEN_CHAR_1_CMD,
	FP_E_WAIT_GEN_CHAR_1_CMD,

	// Get Finger image2 (Send command , Receive Response)
	FP_E_SEND_GET_IMG_2_CMD,
	FP_E_WAIT_GET_IMG_2_CMD,

	// Store Image 1 in CHAR 2
	FP_E_SEND_GEN_CHAR_2_CMD,
	FP_E_WAIT_GEN_CHAR_2_CMD,

	// Check if the user lift his finger
	FP_E_SEND_LIFT_CHECK_CMD,
	FP_E_WAIT_LIFT_CHECK_CMD,

	// Merge the two image files
	FP_E_SEND_MERGE_CMD,
	FP_E_WAIT_MERGE_CMD,

	// Store the image in the flash memory
	FP_E_SEND_STORE_CMD,
	FP_E_WAIT_STORE_CMD,

	FP_E_Success,
	FP_E_Failed,
}FP_Enroll_St_t;


/******************************************************************************************
 *                                FP_Search_St_t
 *
 * Purpose:
 *  This enum defines the states of the fingerprint search state machine.
 *
 * Description:
 *  - Used internally by FP_SEARCH_SM().
 *  - Controls the full sequence of searching for a fingerprint match.
 *  - Search is implemented as a non-blocking cyclic state machine.
 *
 * Search Sequence:
 *  1. Capture fingerprint image
 *  2. Generate character file from image
 *  3. Search the stored fingerprint database
 *  4. Report match or no match
 *
 * Notes:
 *  Similar to enrollment, search uses SEND and WAIT states
 *  to avoid blocking the CPU while waiting for module responses.
 ******************************************************************************************/
typedef enum FP_Search_St_e
{
	// Get Finger image1 (Send command , Receive Response)
	FP_S_SEND_GET_IMG_CMD,
	FP_S_WAIT_GET_IMG_CMD,

    // Store Image 1 in CHAR 1
	FP_S_SEND_GEN_CHAR_1_CMD,
	FP_S_WAIT_GEN_CHAR_1_CMD,

	//Search on the Finger image
	FP_S_SEND_SEARCH_CMD,
	FP_S_WAIT_SEARCH_CMD,

	FP_S_MATCH,
	FP_S_NOT_MATCH,
}FP_Search_St_t;


/******************************************************************************************
 *                              Internal Driver APIs
 *
 * Purpose:
 *  These functions are private/internal helper APIs used only inside the
 *  fingerprint driver source files.
 *
 * Description:
 *  They are declared in FP_Prv.h because they are not intended to be called
 *  directly by the application layer.
 *
 * Functions:
 *  FP_SendCommand():
 *      Builds and sends a fingerprint command packet.
 *
 *  FP_Enroll_SM():
 *      Runs the internal enrollment state machine.
 *
 *  FP_SEARCH_SM():
 *      Runs the internal search state machine.
 *
 *  FP_CheckPacket():
 *      Checks whether a full valid response packet has been received.
 ******************************************************************************************/
FP_Err_St_t FP_SendCommand(uint8_t inst_code , uint8_t *payload , uint8_t payload_len);
void FP_Enroll_SM(void);
void FP_SEARCH_SM(void);
FP_Err_St_t FP_CheckPacket(void);


#endif /* FP_FP_PRV_H_ */
