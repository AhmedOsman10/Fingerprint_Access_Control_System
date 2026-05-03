/*
 * App.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 *
 *  Public API for the Application Layer.
 *
 *  This header exposes the functions, types, and macros required to use the
 *  application layer of the access control system.
 *
 *  The application layer is responsible for:
 *    - receiving commands from the GUI
 *    - controlling the fingerprint driver mode
 *    - reading fingerprint results
 *    - reading date/time from the RTC
 *    - sending access logs and enrollment status to the GUI
 *
 *  ----------------------------------------------------------------------------
 *  Quick Integration Example
 *  ----------------------------------------------------------------------------
 *
 *  Initialization:
 *      if(APP_Init() != APP_Init_Success)
 *      {
 *          // handle initialization failure
 *      }
 *
 *  Cyclic Execution:
 *      while(1)
 *      {
 *          APP_Cyclic();
 *      }
 *
 *  ----------------------------------------------------------------------------
 *  Integration Requirement
 *  ----------------------------------------------------------------------------
 *  APP_Cyclic() must be called periodically.
 *  If it is not called, the application will not:
 *    - process received GUI frames
 *    - react to enrollment requests
 *    - send access log updates
 *    - send enrollment status updates
 */

#ifndef APP_APP_H_
#define APP_APP_H_

/******************************************************************************************
 *                          Application Access Result Macros
 *
 *  These macros define the access result values sent from the application
 *  layer to the GUI.
 *
 *  APP_USER_DENIED:   Fingerprint search did not find a valid user match.
 *  APP_USER_GRANTED:  Fingerprint search found a valid stored user.
 *
 *  Application Use:
 *    These values are included in the GUI log-access payload to indicate
 *    whether access should be treated as granted or denied.
 ******************************************************************************************/
#define APP_USER_DENIED  0x00
#define APP_USER_GRANTED 0x01


/******************************************************************************************
 *                                  APP_Err_St_t
 *
 *  Application-layer return type.
 *
 *  The meaning of the returned value depends on the API being called.
 *
 *  Values:
 *    APP_Init_Success:       Application and all required modules initialized successfully.
 *    APP_Init_Failed:        Application initialization failed.
 *
 *    APP_Rx_Full_Packet_Nok: No full valid GUI frame is available.
 *    APP_Rx_Full_Packet_ok:  A full valid GUI frame is available.
 ******************************************************************************************/
typedef enum APP_Err_St_e
{
	APP_Init_Success,         /* Application initialization completed successfully */
	APP_Init_Failed,          /* Application initialization failed */

	APP_Rx_Full_Packet_Nok,   /* No full valid GUI frame is available */
	APP_Rx_Full_Packet_ok,    /* A full valid GUI frame is available */
}APP_Err_St_t;


/******************************************************************************************
 *                                  APP_Init()
 *
 *  Initialize the application layer.
 *
 *  Description:
 *    - Initializes all modules required by the application layer.
 *    - This includes:
 *        - USART driver for GUI communication
 *        - RTC driver for date/time logging
 *        - Fingerprint driver for access control
 *
 *  Application Use:
 *    Call this API once during system startup before starting the main loop
 *    or scheduler execution.
 *
 *  Example:
 *      if(APP_Init() != APP_Init_Success)
 *      {
 *          // handle initialization error
 *      }
 *
 *  Returns:
 *    APP_Init_Success: Application initialized successfully.
 *    APP_Init_Failed:  Application initialization failed.
 ******************************************************************************************/
APP_Err_St_t APP_Init(void);


/******************************************************************************************
 *                                  APP_Cyclic()
 *
 *  Main periodic function of the application layer.
 *
 *  Description:
 *    - Processes received GUI frames.
 *    - Handles GUI requests such as enrollment request.
 *    - Reads fingerprint search results from the FP driver.
 *    - Reads timestamp information from the RTC driver.
 *    - Sends access log frames to the GUI.
 *    - Sends enrollment instruction/status frames to the GUI.
 *
 *  Application Use:
 *    Call this API continuously as part of the main application execution.
 *
 *  Typical integration points:
 *    - super loop
 *    - periodic scheduler
 *    - RTOS task
 *
 *  Example (super loop):
 *      while(1)
 *      {
 *          APP_Cyclic();
 *      }
 *
 *  Example (RTOS task):
 *      void APP_Task(void *pvParameters)
 *      {
 *          for(;;)
 *          {
 *              APP_Cyclic();
 *              vTaskDelay(pdMS_TO_TICKS(10));
 *          }
 *      }
 *
 *  Note:
 *    The application behavior depends on this function being called periodically.
 *
 *  Returns:
 *    None.
 ******************************************************************************************/
void APP_Cyclic(void);


#endif /* APP_APP_H_ */
