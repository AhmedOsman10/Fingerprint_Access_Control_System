/*
 * App_Prv.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 *
 *  Private configuration and internal definitions for the Application Layer.
 *
 *  This file is NOT intended to be used by the application user.
 *  It contains:
 *    - internal protocol definitions
 *    - RX state machine states
 *    - internal data structures
 *
 *  Design Note:
 *    This file supports the internal implementation of App.c and keeps
 *    application-layer logic clean and modular.
 */

#ifndef APP_APP_PRV_H_
#define APP_APP_PRV_H_

/******************************************************************************************
 *                                  APP_SOF
 *
 *  Start Of Frame (SOF) byte used in the application protocol.
 *
 *  APP_SOF: 0xAA → Marks the beginning of a new frame.
 *
 *  Why it is important:
 *    - UART is a continuous byte stream.
 *    - SOF allows the receiver to detect frame boundaries.
 *    - Helps recover from corrupted or misaligned data.
 *
 *  Usage:
 *    - TX: added as the first byte of every frame.
 *    - RX: state machine waits for this byte to synchronize.
 ******************************************************************************************/
#define APP_SOF   0xAA


/************************************ Log Access Frame ************************************
 *
 *  Packet Format (STM32 → GUI):
 *
 *   SOF | CMD | LEN | Payload Data                                      | CHECKSUM
 *  =====|=====|=====|===================================================|===========
 *  0xAA |0x12 | 10  | Access + User ID + Time + Date                     | XOR CS
 *
 *  Payload Layout:
 *  -------------------------------------------------------------------------
 *   [0]  Access State     (APP_USER_GRANTED / APP_USER_DENIED)
 *   [1]  User ID High
 *   [2]  User ID Low
 *   [3]  Hour
 *   [4]  Minute
 *   [5]  Second
 *   [6]  Day
 *   [7]  Month
 *   [8]  Year High
 *   [9]  Year Low
 *  -------------------------------------------------------------------------
 *
 *  Notes:
 *    - Sent only when a fingerprint result is available.
 *    - Used by GUI to display access logs.
 *
 ***************************************************************************************/


/******************************************************************************************
 *                                  APP_CMD_t
 *
 *  Command IDs used in the application protocol.
 *
 *  These values define the type of message exchanged between GUI and STM32.
 *
 *  APP_Enroll_Req (0x10):
 *    Direction: GUI → STM32
 *    Purpose: Request system to enter enrollment mode.
 *
 *  APP_Enroll_St (0x11):
 *    Direction: STM32 → GUI
 *    Purpose: Send current enrollment instruction/status.
 *
 *  APP_Log_Access (0x12):
 *    Direction: STM32 → GUI
 *    Purpose: Send access result with timestamp.
 *
 *  Notes:
 *    - Must match GUI implementation.
 *    - Used inside CMD field of the frame.
 ******************************************************************************************/
typedef enum APP_CMD_e{
    APP_Enroll_Req = 0x10,
    APP_Enroll_St  = 0x11,
    APP_Log_Access = 0x12,
    APP_Sleep_St   = 0x13
}APP_CMD_t;


/******************************************************************************************
 *                                  App_Rx_St_t
 *
 *  RX state machine states for GUI frame parsing.
 *
 *  Used inside APP_Check_RxFrame().
 *
 *  State Flow:
 *
 *    APP_Rx_Wait_Sof:
 *        Waiting for APP_SOF (start of frame).
 *
 *    APP_Rx_Get_Cmd:
 *        Receiving command byte.
 *
 *    APP_Rx_Get_Len:
 *        Receiving payload length.
 *
 *    APP_Rx_Get_Data:
 *        Receiving payload bytes.
 *
 *    APP_Rx_Get_Cs:
 *        Receiving checksum and verifying frame.
 *
 *    APP_Rx_Complete_Frame:
 *        Full valid frame received and ready for processing.
 *
 *  Notes:
 *    - Non-blocking design.
 *    - Works byte-by-byte with UART.
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
 *  Structure holding a received GUI frame.
 *
 *  Fields:
 *    cmd  : Command ID.
 *    len  : Payload length.
 *    data : Payload buffer (max 20 bytes).
 *
 *  Usage:
 *    - Filled by RX state machine.
 *    - Read by application logic after frame validation.
 *
 *  Notes:
 *    - Maximum payload size is 20 bytes.
 *    - RX logic must ensure no buffer overflow.
 ******************************************************************************************/
typedef struct APP_RxFrame_s{
	uint8_t cmd;
	uint8_t len;
	uint8_t data[20];
}APP_RxFrame_t;


/******************************************************************************************
 *                          APP_LOG_ACCESS_DATA_LEN
 *
 *  Payload length for access log message.
 *
 *  APP_LOG_ACCESS_DATA_LEN: 10 bytes
 *
 *  Breakdown:
 *    - 1 byte  → access state
 *    - 2 bytes → user ID
 *    - 3 bytes → time (hh:mm:ss)
 *    - 4 bytes → date (dd:mm:yyyy)
 *
 *  Usage:
 *    Used when sending APP_Log_Access command.
 ******************************************************************************************/
#define APP_LOG_ACCESS_DATA_LEN  10

#define APP_ENROLL_INST_LEN              3
#define APP_ENROLL_SUCCESS_PAYLOAD_LEN   3

void APP_HandleSleepMode(RTC_Time_t *time);


#endif /* APP_APP_PRV_H_ */
