/*
 * RELAY.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 *
 *  Public API for the Relay driver.
 *
 *  This module provides simple relay control services:
 *    - initialize relay GPIO
 *    - turn relay ON
 *    - turn relay OFF
 *    - turn relay ON for a defined time
 *    - manage relay timeout cyclically
 */

#ifndef RELAY_RELAY_H_
#define RELAY_RELAY_H_

#include <stdint.h>

/******************************************************************************************
 *                              Public Relay Numbers
 *
 *  RELAY_NUM_1: First configured relay.
 *
 *  Application Use:
 *    Use these macros when calling relay APIs instead of using raw numbers.
 ******************************************************************************************/
#define RELAY_NUM_1   0


/******************************************************************************************
 *                                  RELAY_Err_St_t
 *
 *  Return type used by Relay APIs.
 *
 *  Values:
 *    RELAY_Init_Success: Relay initialized successfully.
 *    RELAY_Init_Failed:  Relay initialization failed.
 *
 *    RELAY_On_Ok:        Relay turned ON successfully.
 *    RELAY_On_Nok:       Relay ON operation failed.
 *
 *    RELAY_Off_Ok:       Relay turned OFF successfully.
 *    RELAY_Off_Nok:      Relay OFF operation failed.
 *
 *    RElAY_Invalid_Args: Invalid relay number passed to API.
 ******************************************************************************************/
typedef enum  RELAY_Err_St_e
{
	RELAY_Init_Success,
	RELAY_Init_Failed,

	RELAY_On_Ok,
	RELAY_On_Nok,

	RELAY_Off_Ok,
	RELAY_Off_Nok,

	RElAY_Invalid_Args

}RELAY_Err_St_t;


/******************************************************************************************
 *                                  RELAY_Init()
 *
 *  Initialize selected relay GPIO.
 *
 *  Parameters:
 *    Relay_num: Relay number.
 *
 *  Application Use:
 *    Call once before using RELAY_On(), RELAY_Off(), or RELAY_On_With_Time().
 *
 *  Example:
 *      RELAY_Init(RELAY_NUM_1);
 *
 *  Returns:
 *    RELAY_Init_Success: Relay initialized successfully.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_Init(uint8_t Relay_num);


/******************************************************************************************
 *                                  RELAY_On()
 *
 *  Turn selected relay ON.
 *
 *  Parameters:
 *    Relay_num: Relay number.
 *
 *  Example:
 *      RELAY_On(RELAY_NUM_1);
 *
 *  Returns:
 *    RELAY_On_Ok:        Relay turned ON.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_On(uint8_t Relay_num);


/******************************************************************************************
 *                                  RELAY_Off()
 *
 *  Turn selected relay OFF.
 *
 *  Parameters:
 *    Relay_num: Relay number.
 *
 *  Example:
 *      RELAY_Off(RELAY_NUM_1);
 *
 *  Returns:
 *    RELAY_Off_Ok:       Relay turned OFF.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num);


/******************************************************************************************
 *                              RELAY_On_With_Time()
 *
 *  Turn selected relay ON for a defined time.
 *
 *  Parameters:
 *    Relay_num:      Relay number.
 *    Relay_Time_ms:  ON duration in milliseconds.
 *
 *  Application Use:
 *    After calling this API, RELAY_Time_Manager_Cyclic() must be called periodically
 *    so the relay can be turned OFF when the requested time expires.
 *
 *  Example:
 *      RELAY_On_With_Time(RELAY_NUM_1, 3000);
 *
 *  Returns:
 *    RELAY_On_Ok:        Relay turned ON and timer started.
 *    RElAY_Invalid_Args: Invalid relay number.
 ******************************************************************************************/
RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num , uint32_t Relay_Time_ms);


/******************************************************************************************
 *                          RELAY_Time_Manager_Cyclic()
 *
 *  Main cyclic time manager for timed relay control.
 *
 *  Application Use:
 *    Call periodically from:
 *      - super loop
 *      - scheduler
 *      - RTOS task
 *
 *  Purpose:
 *    Checks active timed relays and turns them OFF when their deadline expires.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void RELAY_Time_Manager_Cyclic(void);

#endif /* RELAY_RELAY_H_ */
