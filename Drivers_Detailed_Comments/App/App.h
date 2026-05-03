/*
 * App.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */

#ifndef APP_APP_H_
#define APP_APP_H_

/******************************************************************************************
 *                          Application Access Result Macros
 *
 * Purpose:
 *  These macros define the access result values sent from the application
 *  to the GUI after fingerprint checking.
 *
 * Description:
 *  - APP_USER_DENIED:
 *      Used when fingerprint search did not find a valid user match.
 *
 *  - APP_USER_GRANTED:
 *      Used when fingerprint search found a valid stored user.
 *
 * Notes:
 *  - These values are part of the application-to-GUI protocol payload.
 *  - They are higher-level meanings for the GUI, unlike raw FP driver values.
 ******************************************************************************************/
#define APP_USER_DENIED  0x00
#define APP_USER_GRANTED 0x01


/******************************************************************************************
 *                                  APP_Err_St_t
 *
 * Purpose:
 *  This enum defines the return/error states used by the application layer APIs.
 *
 * Description:
 *  - It is used to inform the caller whether an App-layer operation succeeded
 *    or failed.
 *  - Using enum values makes the code more readable and self-explanatory.
 *
 * Values:
 *  APP_Init_Success:
 *      Application and all required dependent modules initialized successfully.
 *
 *  APP_Init_Failed:
 *      Application initialization failed because one or more required modules
 *      failed to initialize.
 *
 *  APP_Rx_Full_Packet_Nok:
 *      No full valid GUI frame has been received yet.
 *
 *  APP_Rx_Full_Packet_ok:
 *      A full valid GUI frame has been received and is ready to be processed.
 ******************************************************************************************/
typedef enum APP_Err_St_e
{
	APP_Init_Success,
	APP_Init_Failed,

	APP_Rx_Full_Packet_Nok,
	APP_Rx_Full_Packet_ok,
}APP_Err_St_t;


/******************************************************************************************
 *                                  APP_Init()
 *
 * Purpose:
 *  Initializes the application layer and all modules it depends on.
 *
 * Description:
 *  - This function performs the top-level initialization required by the system.
 *  - It initializes:
 *      - USART driver for GUI communication
 *      - RTC driver for date/time logging
 *      - Fingerprint driver for access control functions
 *
 * Notes:
 *  - This function should be called once at system startup.
 *  - If one required module fails, the whole application initialization fails.
 *
 * Returns:
 *  - APP_Init_Success : initialization completed successfully
 *  - APP_Init_Failed  : initialization failed
 ******************************************************************************************/
APP_Err_St_t APP_Init(void );


/******************************************************************************************
 *                                  APP_Cylic()
 *
 * Purpose:
 *  Main cyclic function of the application layer.
 *
 * Description:
 *  - This function should be called periodically from the super loop or scheduler.
 *  - It handles:
 *      - checking received commands from the GUI
 *      - reacting to enrollment requests
 *      - reading fingerprint search results
 *      - collecting date/time from RTC
 *      - sending access log data to the GUI
 *      - sending enrollment instruction updates to the GUI
 *
 * Important Design Idea:
 *  - This function coordinates system behavior.
 *  - It does not directly handle low-level hardware details.
 *  - It uses the lower drivers (USART, RTC, FP) to do the actual hardware work.
 *
 * Returns:
 *  - None.
 ******************************************************************************************/
void APP_Cylic(void);


#endif /* APP_APP_H_ */
