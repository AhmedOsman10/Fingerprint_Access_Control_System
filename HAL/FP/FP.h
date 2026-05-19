/*
 * FP.h
 *
 *  Created on: 2 Feb 2026
 *      Author: Ahmed
 *
 *  Public API for the Fingerprint driver.
 *
 *  This header exposes the functions, types, and macros required by the
 *  application layer to use the fingerprint driver.
 *
 *  The application layer does not need access to the driver's private
 *  implementation details such as:
 *    - packet parsing states
 *    - internal enrollment states
 *    - internal search states
 *    - command packet construction
 *
 *  ----------------------------------------------------------------------------
 *  Quick Integration Example
 *  ----------------------------------------------------------------------------
 *
 *  Initialization:
 *      if(FP_Init() != FP_InitSuccess)
 *      {
 *          // handle initialization failure
 *      }
 *
 *  Cyclic Execution (Required):
 *      while(1)
 *      {
 *          FP_MainFunction_Cyclic();
 *      }
 *
 *  Search Mode Example:
 *      uint8_t match_st;
 *      uint16_t user_id;
 *
 *      FP_SetMode(FP_SEARCH_MODE);
 *
 *      if(FP_Get_User(&match_st, &user_id) == FP_GetUser_Ok)
 *      {
 *          if(match_st == FP_MATCH_ST)
 *          {
 *              // valid user detected
 *          }
 *          else
 *          {
 *              // no match
 *          }
 *      }
 *
 *  Enroll Mode Example:
 *      FP_SetMode(FP_ENROLL_MODE);
 *
 *      if(FP_GetEnroll_Instruction() == FP_E_Inst_Place_Finger)
 *      {
 *          // update LCD / GUI / terminal
 *      }
 *
 *  ----------------------------------------------------------------------------
 *  Integration Requirement
 *  ----------------------------------------------------------------------------
 *  FP_MainFunction_Cyclic() must be called periodically.
 *  If it is not called, the driver will not:
 *    - process received fingerprint packets
 *    - update search results
 *    - progress enrollment flow
 *    - refresh enrollment instructions
 */

#ifndef FP_FP_H_
#define FP_FP_H_

#include <stdint.h>


/******************************************************************************************
 *                                  FP_Err_St_t
 *
 *  Driver return type used by the public fingerprint APIs.
 *
 *  The meaning of the returned value depends on the API being called.
 ******************************************************************************************/
typedef enum FP_Err_St_e
{
	FP_InitSuccess,         /* Driver initialization completed successfully */
	FP_InitFailed,          /* Driver initialization failed */

	FP_Rx_Full_Packet_Ok,   /* A full valid packet is available */
	FP_Rx_Full_Packet_Nok,  /* No full valid packet is available */

	FP_SendCmd_Success,     /* Command packet sent successfully */
	FP_SendCmd_Failed,      /* Command packet sending failed */

	FP_GetUser_Ok,          /* A user search result was returned successfully */
	FP_GetUser_NOk,         /* No new user search result is available */
}FP_Err_St_t;


/******************************************************************************************
 *                                  FP_Search_Data_t
 *
 *  Public search result container.
 *
 *  This type represents one fingerprint search result:
 *    - match status
 *    - matched user ID
 *
 *  Members:
 *    match_st: Match status value.
 *              Use:
 *                - FP_MATCH_ST
 *                - FP_NOT_MATCH_ST
 *
 *    user_id:  Matched user ID.
 *              Valid only if match_st == FP_MATCH_ST.
 *
 *  Note:
 *  The application may use FP_Get_User() directly instead of using this type.
 ******************************************************************************************/
typedef struct FP_Search_Data_s
{
	uint8_t match_st;
	uint16_t user_id;
}FP_Search_Data_t;


/******************************************************************************************
 *                           FP_GetEnroll_Instruction_t
 *
 *  Public enrollment status / instruction type.
 *
 *  This type is used by the application layer to display the current
 *  enrollment step to the user without needing access to the driver's
 *  internal state machine.
 *
 *  Typical integration points:
 *    - LCD display
 *    - GUI status message
 *    - UART terminal output
 *
 *  Values:
 *    FP_E_Inst_Idle:               No active enrollment operation.
 *    FP_E_Inst_Place_Finger:       Ask the user to place finger on the sensor.
 *    FP_E_Inst_Lift_Finger:        Ask the user to lift finger from the sensor.
 *    FP_E_Inst_Place_Finger_Again: Ask the user to place the same finger again.
 *    FP_E_Inst_Processing:         Enrollment is being processed internally.
 *    FP_E_Inst_Success:            Enrollment completed successfully.
 *    FP_E_Inst_Failed:             Enrollment failed.
 ******************************************************************************************/
typedef enum FP_GetEnroll_Instruction_e{
	FP_E_Inst_Idle,               /* No active enrollment operation */
	FP_E_Inst_Place_Finger,       /* Ask user to place finger */
	FP_E_Inst_Lift_Finger,        /* Ask user to lift finger */
	FP_E_Inst_Place_Finger_Again, /* Ask user to place the same finger again */
	FP_E_Inst_Processing,         /* Enrollment is being processed internally */
	FP_E_Inst_Success,            /* Enrollment completed successfully */
	FP_E_Inst_Failed,             /* Enrollment failed */
}FP_GetEnroll_Instruction_t;


/******************************************************************************************
 *                                  FP_Mode_t
 *
 *  Public driver mode type.
 *
 *  The fingerprint driver supports two operating modes:
 *    - FP_SEARCH_MODE
 *    - FP_ENROLL_MODE
 *
 *  Application Use:
 *    Use this type with FP_SetMode() and FP_GetMode() to control and
 *    monitor the current driver mode.
 ******************************************************************************************/
typedef uint8_t FP_Mode_t;


/******************************************************************************************
 *                              Public Driver Modes
 *
 *  FP_SEARCH_MODE:  Driver operates in fingerprint search mode.
 *                   Use FP_Get_User() to read the latest search result.
 *
 *  FP_ENROLL_MODE:  Driver operates in fingerprint enrollment mode.
 *                   Use FP_GetEnroll_Instruction() to monitor enrollment progress.
 ******************************************************************************************/
#define FP_SEARCH_MODE  0
#define FP_ENROLL_MODE  1


/******************************************************************************************
 *                          Public Fingerprint Match Status
 *
 *  Match result values returned by FP_Get_User().
 *
 *  FP_MATCH_ST:      Fingerprint matched a stored user.
 *  FP_NOT_MATCH_ST:  Fingerprint did not match any stored user.
 *
 *  Example:
 *      if(match_st == FP_MATCH_ST)
 *      {
 *          // access granted
 *      }
 *      else
 *      {
 *          // access denied
 *      }
 ******************************************************************************************/
#define FP_MATCH_ST      1
#define FP_NOT_MATCH_ST  0


/******************************************************************************************
 *                                  FP_Init()
 *
 *  Initialize the fingerprint driver.
 *
 *  This API shall be called once before using any other public
 *  fingerprint driver API.
 *
 *  Application Use:
 *    Call this API during system initialization before starting the
 *    main application loop or RTOS task scheduling.
 *
 *  Example:
 *      if(FP_Init() != FP_InitSuccess)
 *      {
 *          // handle driver init failure
 *      }
 *
 *  Returns:
 *    FP_InitSuccess: Driver initialized successfully.
 *    FP_InitFailed:  Driver initialization failed.
 ******************************************************************************************/
FP_Err_St_t FP_Init(void);


/******************************************************************************************
 *                                FP_SimpleTesT()
 *
 *  Simple debug / bring-up API.
 *
 *  This API is intended for early communication testing only.
 *  It is not the normal application interface.
 *
 *  Typical use cases:
 *    - module communication check
 *    - early hardware bring-up
 *    - debug session
 *
 *  Production note:
 *    Normal application integration should use:
 *      - FP_Init()
 *      - FP_MainFunction_Cyclic()
 *      - FP_SetMode()
 *      - FP_Get_User()
 *      - FP_GetEnroll_Instruction()
 ******************************************************************************************/
void FP_SimpleTesT(void);


/******************************************************************************************
 *                                  FP_Get_User()
 *
 *  Read the latest fingerprint search result.
 *
 *  This API is intended to be used in FP_SEARCH_MODE.
 *
 *  Application Use:
 *    Call this API periodically to check whether the driver has produced
 *    a new fingerprint search result.
 *
 *  Parameters:
 *    match_st: Output pointer for match status.
 *              Returned value:
 *                - FP_MATCH_ST
 *                - FP_NOT_MATCH_ST
 *
 *    user_id:  Output pointer for matched user ID.
 *              Valid only if match_st == FP_MATCH_ST.
 *
 *  Example:
 *      uint8_t match_st;
 *      uint16_t user_id;
 *
 *      if(FP_Get_User(&match_st, &user_id) == FP_GetUser_Ok)
 *      {
 *          if(match_st == FP_MATCH_ST)
 *          {
 *              // valid matched user_id
 *          }
 *          else
 *          {
 *              // fingerprint not matched
 *          }
 *      }
 *
 *  Returns:
 *    FP_GetUser_Ok:  A new search result was available and copied to the output pointers.
 *    FP_GetUser_NOk: No new search result is currently available.
 *
 *  Note:
 *    FP_MainFunction_Cyclic() must be called periodically for new results
 *    to become available.
 ******************************************************************************************/
FP_Err_St_t FP_Get_User(uint8_t *match_st , uint16_t *user_id);


/******************************************************************************************
 *                              FP_MainFunction_Cyclic()
 *
 *  Main periodic function of the fingerprint driver.
 *
 *  This function shall be called periodically by the application.
 *
 *  Responsibilities:
 *    - process received data from the fingerprint module
 *    - run the internal driver state machines
 *    - update search results
 *    - update enrollment progress
 *
 *  Application Use:
 *    Call this API continuously as part of the application's cyclic execution.
 *    This is the main runtime entry point of the driver.
 *
 *  Typical integration points:
 *    - super loop
 *    - periodic scheduler
 *    - RTOS task
 *
 *  Example (super loop):
 *      while(1)
 *      {
 *          FP_MainFunction_Cyclic();
 *      }
 *
 *  Example (RTOS task):
 *      void FP_Task(void *pvParameters)
 *      {
 *          for(;;)
 *          {
 *              FP_MainFunction_Cyclic();
 *              vTaskDelay(pdMS_TO_TICKS(10));
 *          }
 *      }
 *
 *  If this function is not called periodically:
 *    - no new fingerprint data will be processed
 *    - search mode will not update results
 *    - enroll mode will not progress
 *    - enrollment status will not refresh
 ******************************************************************************************/
void FP_MainFunction_Cyclic(void);


/******************************************************************************************
 *                                  FP_SetMode()
 *
 *  Set the current driver mode.
 *
 *  Supported modes:
 *    - FP_SEARCH_MODE
 *    - FP_ENROLL_MODE
 *
 *  Application Use:
 *    Call this API to switch between normal search operation and
 *    fingerprint enrollment operation.
 *
 *  Example:
 *      FP_SetMode(FP_ENROLL_MODE);
 *
 *      // later
 *      FP_SetMode(FP_SEARCH_MODE);
 *
 *  Parameters:
 *    mode: Requested driver mode.
 ******************************************************************************************/
void FP_SetMode(FP_Mode_t mode);


/******************************************************************************************
 *                                  FP_GetMode()
 *
 *  Get the current driver mode.
 *
 *  Application Use:
 *    Use this API when the application needs to know whether the driver
 *    is currently operating in search mode or enroll mode.
 *
 *  Example:
 *      if(FP_GetMode() == FP_SEARCH_MODE)
 *      {
 *          // normal access control behavior
 *      }
 *
 *  Returns:
 *    FP_SEARCH_MODE: Driver is in search mode.
 *    FP_ENROLL_MODE: Driver is in enroll mode.
 ******************************************************************************************/
FP_Mode_t FP_GetMode(void);


/******************************************************************************************
 *                           FP_GetEnroll_Instruction()
 *
 *  Get the current enrollment instruction / status.
 *
 *  This API is intended to be used in FP_ENROLL_MODE.
 *
 *  Application Use:
 *    Call this API to update the user interface during enrollment.
 *
 *  Typical integration points:
 *    - GUI status label
 *    - LCD message
 *    - terminal output
 *
 *  Example:
 *      FP_GetEnroll_Instruction_t inst;
 *
 *      inst = FP_GetEnroll_Instruction();
 *
 *      if(inst == FP_E_Inst_Place_Finger)
 *      {
 *          // show "Place Finger"
 *      }
 *
 *  Returns:
 *    Current value of type FP_GetEnroll_Instruction_t.
 *
 *  Note:
 *    FP_MainFunction_Cyclic() must be called periodically for this status
 *    to update correctly.
 ******************************************************************************************/


FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void);

/******************************************************************************************
 *                              FP_Delete_All_Users()
 *
 *  Delete all enrolled fingerprints stored in sensor flash memory.
 *
 *  Warning:
 *    This removes all registered users permanently.
 *
 *  Returns:
 *    FP_SendCmd_Success / FP_SendCmd_Failed
 ******************************************************************************************/
FP_Err_St_t FP_Delete_All_Users(void);



uint16_t FP_Get_Curr_User_Id(void);
#endif /* FP_FP_H_ */
