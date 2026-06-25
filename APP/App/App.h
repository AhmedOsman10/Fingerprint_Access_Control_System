/******************************************************************************
 * @file    App.h
 * @author  Ahmed Abdelrhman
 * @brief   Public interface for the Application Layer.
 *
 * @details This file exposes the public APIs and public application-level
 *          definitions used by the Fingerprint Access Control System.
 *
 *          The application layer is responsible for:
 *          - Initializing application modules
 *          - Running the cyclic application logic
 *          - Sending sleep/awake status to the GUI
 *          - Reporting access result values to the GUI
 *
 * @note    Only comments, spacing, and layout were improved. The application
 *          logic is intentionally kept the same.
 ******************************************************************************/

#ifndef APP_APP_H_
#define APP_APP_H_

#include <stdint.h>


/*=============================================================================
 * Public Macros
 *============================================================================*/

/* Access denied value sent to the GUI. */
#define APP_USER_DENIED   0x00

/* Access granted value sent to the GUI. */
#define APP_USER_GRANTED  0x01


/*=============================================================================
 * Public Types
 *============================================================================*/

/******************************************************************************
 * @brief Application-layer error/status type.
 ******************************************************************************/
typedef enum APP_Err_St_e
{
    APP_Init_Success,        /* Application initialization completed successfully. */
    APP_Init_Failed,         /* Application initialization failed. */

    APP_Rx_Full_Packet_Nok,  /* No complete valid GUI frame is available. */
    APP_Rx_Full_Packet_ok,   /* A complete valid GUI frame is available. */
} APP_Err_St_t;


/*=============================================================================
 * Public Function Prototypes
 *============================================================================*/

/******************************************************************************
 * @brief  Initialize the application layer.
 *
 * @details Initializes the modules required by the APP layer, including USART,
 *          RTC, fingerprint driver, and relay driver.
 *
 * @return APP_Init_Success if initialization succeeds.
 * @return APP_Init_Failed  if initialization fails.
 ******************************************************************************/
APP_Err_St_t APP_Init(void);


/******************************************************************************
 * @brief  Main periodic function of the application layer.
 *
 * @details Must be called periodically from the main loop or an RTOS task.
 *          It handles GUI frames, fingerprint results, enrollment updates,
 *          access logs, relay control, and sleep/wake behavior.
 *
 * @return None.
 ******************************************************************************/
void APP_Cyclic(void);


/******************************************************************************
 * @brief  Send system sleep/awake status to the GUI.
 *
 * @param  state APP_SYSTEM_AWAKE or APP_SYSTEM_SLEEP.
 *
 * @return None.
 ******************************************************************************/
void APP_SendSleepStatus(uint8_t state);


#endif /* APP_APP_H_ */
