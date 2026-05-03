/*
 * FP.h
 *
 *  Created on: 2 Feb 2026
 *      Author: Ahmed
 */

#ifndef FP_FP_H_
#define FP_FP_H_

#include <stdint.h>

/******************************************************************************************
 *                                  FP_Err_St_t
 *
 * Purpose:
 *  This enum defines the general return/error states used by the fingerprint driver.
 *
 * Description:
 *  - It is used as the return type for multiple driver APIs.
 *  - It tells the caller whether an operation succeeded or failed.
 *  - Using enum values makes the code easier to read and more meaningful than
 *    returning raw numbers like 0 or 1.
 *
 * Notes:
 *  - The same enum is reused by different functions in the same driver.
 *  - The exact meaning of each enum value depends on the function using it.
 *
 * Values:
 *  FP_InitSuccess       : Driver initialization completed successfully.
 *  FP_InitFailed        : Driver initialization failed.
 *
 *  FP_Rx_Full_Packet_Ok : A full valid packet was received successfully.
 *  FP_Rx_Full_Packet_Nok: No complete valid packet is available yet.
 *
 *  FP_SendCmd_Success   : Command packet was sent successfully.
 *  FP_SendCmd_Failed    : Command packet sending failed.
 *
 *  FP_GetUser_Ok        : A fingerprint search result was read successfully.
 *  FP_GetUser_NOk       : No fingerprint search result is available.
 ******************************************************************************************/
typedef enum FP_Err_St_e
{
	FP_InitSuccess,
	FP_InitFailed,

	FP_Rx_Full_Packet_Ok,
	FP_Rx_Full_Packet_Nok,

	FP_SendCmd_Success,
	FP_SendCmd_Failed,

	FP_GetUser_Ok,
	FP_GetUser_NOk,
}FP_Err_St_t;


/******************************************************************************************
 *                                  FP_Search_Data_t
 *
 * Purpose:
 *  This structure holds the result of a fingerprint search operation.
 *
 * Description:
 *  - It is used to pass fingerprint match information from the internal search
 *    state machine to the application layer.
 *  - It is also the data type stored in the FreeRTOS queue used by the driver.
 *
 * Members:
 *  match_st : Match status.
 *             - FP_MATCH_ST     -> fingerprint matched a stored user
 *             - FP_NOT_MATCH_ST -> fingerprint did not match
 *
 *  user_id  : The ID/page of the matched fingerprint user.
 *             This value is meaningful only when match_st indicates a match.
 ******************************************************************************************/
typedef struct FP_Search_Data_s
{
	uint8_t match_st;
	uint16_t user_id;
}FP_Search_Data_t;


/******************************************************************************************
 *                           FP_GetEnroll_Instruction_t
 *
 * Purpose:
 *  This enum provides user-friendly enrollment instructions/status values.
 *
 * Description:
 *  - It is used by the application layer to know what message should be shown
 *    to the user during the fingerprint enrollment process.
 *  - It hides the detailed internal enrollment state machine and provides
 *    simple high-level instructions.
 *
 * Example Usage:
 *  - LCD display
 *  - UART terminal message
 *  - GUI text
 *
 * Values:
 *  FP_E_Inst_Idle               : No active enrollment instruction.
 *  FP_E_Inst_Place_Finger       : User should place finger on the sensor.
 *  FP_E_Inst_Lift_Finger        : User should remove finger from the sensor.
 *  FP_E_Inst_Place_Finger_Again : User should place the same finger again.
 *  FP_E_Inst_Processing         : Module is processing the enrollment steps.
 *  FP_E_Inst_Success            : Enrollment completed successfully.
 *  FP_E_Inst_Failed             : Enrollment failed.
 ******************************************************************************************/
typedef enum FP_GetEnroll_Instruction_e{
	FP_E_Inst_Idle,
	FP_E_Inst_Place_Finger,
	FP_E_Inst_Lift_Finger,
	FP_E_Inst_Place_Finger_Again,
	FP_E_Inst_Processing,
	FP_E_Inst_Success,
	FP_E_Inst_Failed,
}FP_GetEnroll_Instruction_t;


/******************************************************************************************
 *                                  FP_Mode_t
 *
 * Purpose:
 *  This data type represents the operating mode of the fingerprint driver.
 *
 * Description:
 *  - The driver can operate in one of two modes:
 *      1. Search mode
 *      2. Enroll mode
 *  - This type is defined as uint8_t for lightweight storage.
 *
 * Notes:
 *  - The actual mode values are defined by:
 *      FP_SEARCH_MODE
 *      FP_ENROLL_MODE
 ******************************************************************************************/
typedef uint8_t FP_Mode_t;

/* Driver operating modes */
#define FP_SEARCH_MODE  0   /* Normal mode: continuously search for matching fingerprints */
#define FP_ENROLL_MODE  1   /* Enrollment mode: add a new fingerprint into module memory */


/******************************************************************************************
 *                              Fingerprint Match Status
 *
 * Purpose:
 *  These macros define the result of a fingerprint search comparison.
 *
 * Values:
 *  FP_MATCH_ST     : Fingerprint matched a stored user.
 *  FP_NOT_MATCH_ST : Fingerprint did not match any stored user.
 ******************************************************************************************/
#define FP_MATCH_ST      1
#define FP_NOT_MATCH_ST  0


/******************************************************************************************
 *                                  FP_Init()
 *
 * Purpose:
 *  Initializes the fingerprint driver.
 *
 * Description:
 *  - Initializes the communication interface and internal driver resources.
 *  - Must be called before using the other fingerprint APIs.
 *
 * Returns:
 *  - FP_InitSuccess : Initialization completed successfully.
 *  - FP_InitFailed  : Initialization failed.
 ******************************************************************************************/
FP_Err_St_t FP_Init(void);


/******************************************************************************************
 *                                FP_SimpleTesT()
 *
 * Purpose:
 *  Runs a very simple test for fingerprint communication.
 *
 * Description:
 *  - Used mainly for debugging and early testing.
 *  - It sends a basic command to the fingerprint module and checks the response.
 *
 * Notes:
 *  - This is not the main application API.
 *  - Mostly useful during bring-up and debugging.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_SimpleTesT(void);


/******************************************************************************************
 *                                  FP_Get_User()
 *
 * Purpose:
 *  Returns the latest fingerprint search result to the application layer.
 *
 * Description:
 *  - Reads one search result from the internal queue.
 *  - If a result exists, it provides:
 *      - match status
 *      - user ID
 *
 * Parameters:
 *  match_st : Pointer to store match status.
 *  user_id  : Pointer to store matched user ID.
 *
 * Returns:
 *  - FP_GetUser_Ok  : Search result was available and copied successfully.
 *  - FP_GetUser_NOk : No new search result is available.
 ******************************************************************************************/
FP_Err_St_t FP_Get_User(uint8_t *match_st , uint16_t *user_id);


/******************************************************************************************
 *                              FP_MainFunction_Cyclic()
 *
 * Purpose:
 *  Main cyclic function of the fingerprint driver.
 *
 * Description:
 *  - This function should be called periodically.
 *  - It handles:
 *      - receiving and parsing packets
 *      - running the current internal state machine
 *        (search mode or enrollment mode)
 *
 * Notes:
 *  - This is the main periodic entry point for the driver.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_MainFunction_Cyclic(void);


/******************************************************************************************
 *                                  FP_SetMode()
 *
 * Purpose:
 *  Sets the current mode of the fingerprint driver.
 *
 * Description:
 *  - Switches the driver between search mode and enrollment mode.
 *  - Used by the application to control driver behavior.
 *
 * Parameters:
 *  mode : Desired fingerprint mode.
 *         Example:
 *           - FP_SEARCH_MODE
 *           - FP_ENROLL_MODE
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void FP_SetMode(FP_Mode_t mode);


/******************************************************************************************
 *                                  FP_GetMode()
 *
 * Purpose:
 *  Returns the current operating mode of the fingerprint driver.
 *
 * Returns:
 *  - Current fingerprint mode:
 *      FP_SEARCH_MODE or FP_ENROLL_MODE
 ******************************************************************************************/
FP_Mode_t FP_GetMode(void);


/******************************************************************************************
 *                           FP_GetEnroll_Instruction()
 *
 * Purpose:
 *  Returns the current enrollment instruction/status.
 *
 * Description:
 *  - Used by the application layer to display a user-friendly enrollment message.
 *  - This function converts the internal enrollment state into a simple public status.
 *
 * Returns:
 *  - A value of type FP_GetEnroll_Instruction_t
 ******************************************************************************************/
FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void);


#endif /* FP_FP_H_ */
