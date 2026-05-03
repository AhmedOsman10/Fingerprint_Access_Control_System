/*
 * App_Prv.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */

#ifndef APP_APP_PRV_H_
#define APP_APP_PRV_H_

/******************************************************************************************
 *                                  APP_SOF
 *
 * Purpose:
 *  Start Of Frame byte for the application protocol.
 *
 * Description:
 *  - This byte marks the beginning of a new packet coming from or going to the GUI.
 *  - The RX state machine waits for this value to synchronize with incoming data.
 *
 * Why it is important:
 *  - UART communication is continuous (stream of bytes)
 *  - SOF allows the receiver to detect where a valid frame starts
 *  - Helps recover from noise or corrupted data
 ******************************************************************************************/
#define APP_SOF   0xAA


/*==> Enroll_Req
==> ENROLL_ST
==> LOG_CMD
*/

/************************************ Log Access Frame ************************************************
 * Packet Format between STM32 (Application) and GUI:
 *
 *  SOF | CMD_INST | LEN | Payload Data                                      | CHECK SUM
 *  ====|==========|=====|===================================================|============
 *  0xAA| 0x12     |  9  | Access State + User ID + Time + Date              | XOR Checksum
 *
 * Payload Breakdown:
 *  -------------------------------------------------------------------------
 *  Byte 0 : Access state (APP_USER_GRANTED / APP_USER_DENIED)
 *  Byte 1 : User ID High Byte
 *  Byte 2 : User ID Low Byte
 *  Byte 3 : Hour
 *  Byte 4 : Minute
 *  Byte 5 : Second
 *  Byte 6 : Day
 *  Byte 7 : Month
 *  Byte 8 : Year High Byte
 *  Byte 9 : Year Low Byte
 *  -------------------------------------------------------------------------
 *
 * Notes:
 *  - This frame is used to log access attempts (granted/denied) to the GUI.
 *  - It is sent only when a fingerprint search result is available.
 *******************************************************************************************************/


/******************************************************************************************
 *                                  APP_CMD_t
 *
 * Purpose:
 *  Defines all command IDs used in the application-to-GUI protocol.
 *
 * Description:
 *  These values identify the type of message being sent or received.
 *  They are used inside the CMD field of the application packet.
 *
 * Commands:
 *
 *  APP_Enroll_Req:
 *      Direction: GUI → STM32
 *      Purpose:
 *          Request the system to enter fingerprint enrollment mode.
 *
 *  APP_Enroll_St:
 *      Direction: STM32 → GUI
 *      Purpose:
 *          Send current enrollment instruction/status.
 *          Example:
 *              - Place finger
 *              - Lift finger
 *              - Processing
 *              - Success / Failed
 *
 *  APP_Log_Access:
 *      Direction: STM32 → GUI
 *      Purpose:
 *          Send access result (granted/denied) along with timestamp.
 *
 * Notes:
 *  - These command values are part of the application protocol.
 *  - They should match what the GUI expects.
 ******************************************************************************************/
typedef enum APP_CMD_e{
	APP_Enroll_Req = 0x10, // PC(GUI) ===> STM  ==> Set Enroll mode
 	APP_Enroll_St  = 0x11, // STM ==> PC (GUI)  ==> Enroll mode status
	APP_Log_Access = 0x12  // STM ==> PC (GUI)  ==> Access log (search mode)
}APP_CMD_t;


/******************************************************************************************
 *                                  App_Rx_St_t
 *
 * Purpose:
 *  Defines the states of the application RX state machine.
 *
 * Description:
 *  - Used inside APP_Check_RxFrame().
 *  - This state machine reconstructs incoming GUI packets byte-by-byte.
 *
 * State Flow:
 *  APP_Rx_Wait_Sof:
 *      Waiting for Start Of Frame (APP_SOF).
 *
 *  APP_Rx_Get_Cmd:
 *      Receive command byte.
 *
 *  APP_Rx_Get_Len:
 *      Receive payload length.
 *
 *  APP_Rx_Get_Data:
 *      Receive payload data bytes.
 *
 *  APP_Rx_Get_Cs:
 *      Receive checksum byte and verify frame.
 *
 *  APP_Rx_Complete_Frame:
 *      Full valid frame received and ready for processing.
 *
 * Notes:
 *  - Designed to be non-blocking.
 *  - Works with UART byte-by-byte reception.
 ******************************************************************************************/
typedef enum App_Rx_St_e{
	APP_Rx_Wait_Sof,
	APP_Rx_Get_Cmd,
	APP_Rx_Get_Len,
	APP_Rx_Get_Data,
	APP_Rx_Get_Cs,
	APP_Rx_Complete_Frame,
}App_Rx_St_t;


/******************************************************************************************
 *                                  APP_RxFrame_t
 *
 * Purpose:
 *  Holds a complete received application frame from the GUI.
 *
 * Description:
 *  - This structure stores all fields of the received packet after parsing.
 *  - Once a frame is fully received and validated (checksum OK),
 *    this structure is used by the application logic.
 *
 * Members:
 *  cmd  : Command ID received from GUI.
 *  len  : Number of payload bytes.
 *  data : Payload buffer.
 *
 * Notes:
 *  - Maximum payload size is 20 bytes.
 *  - This must match the expected maximum size of GUI messages.
 *  - Should be protected against overflow in RX state machine.
 ******************************************************************************************/
typedef struct APP_RxFrame_s{
	uint8_t cmd;
	uint8_t len;
	uint8_t data[20];
}APP_RxFrame_t;


/******************************************************************************************
 *                          APP_LOG_ACCESS_DATA_LEN
 *
 * Purpose:
 *  Defines the payload length of the access log frame.
 *
 * Description:
 *  - Used when sending APP_Log_Access command.
 *  - Ensures consistent frame length for access log messages.
 *
 * Value:
 *  10 bytes:
 *      1 byte  -> access state
 *      2 bytes -> user ID
 *      3 bytes -> time (hh:mm:ss)
 *      4 bytes -> date (dd:mm:yyyy)
 ******************************************************************************************/
#define APP_LOG_ACCESS_DATA_LEN  10


#endif /* APP_APP_PRV_H_ */
