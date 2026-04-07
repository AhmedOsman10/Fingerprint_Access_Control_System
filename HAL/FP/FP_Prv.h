/*
 * FP_Prv.h
 *
 *  Created on: 4 Feb 2026
 *      Author: Ahmed
 */

#ifndef FP_FP_PRV_H_
#define FP_FP_PRV_H_

#define FP_HEADER_H  0xEF
#define FP_HEADER_L	 0x01
#define FP_PID_CMD	 0x01
#define FP_PID_ACK	 0x07

#define FP_GEN_IMG_CMD        0x01
#define FP_GEN_FILE_CHAR_CMD  0x02
#define FP_MERGE_TWO_IMG_CMD  0x05  // reg model command
#define FP_STORE_IMG_CMD      0x06  // reg model command
#define FP_SEARCH_CMD         0x04  // Search command

#define CHECK_SUM_LEN   2
#define INST_CODE_LEN   1


#define FP_CONFIRM_CODE_INDX       0x00
#define FP_GET_FINGER_OK           0x00
#define FP_GET_FINGER_NOK          0x03
#define FP_NO_FINGER               0x02
#define FP_MERGE_SUCCESS           0x00
#define FP_STORE_SUCCESS           0x00
#define FP_GEN_CHAR_OK             0x00
#define FP_SEARCH_MATCH            0x00
#define FP_SEARCH_NOT_MATCH        0x09


#define FP_CHAR_FILE_1   0x01
#define FP_CHAR_FILE_2   0x02

/************************ FP Rx cyclic states **********/
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



FP_Err_St_t FP_SendCommand(uint8_t inst_code , uint8_t *payload , uint8_t payload_len);

#endif /* FP_FP_PRV_H_ */
