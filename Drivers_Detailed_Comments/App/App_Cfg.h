/*
 * App_Cfg.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */

#ifndef APP_APP_CFG_H_
#define APP_APP_CFG_H_

/******************************************************************************************
 *                              Application Configuration File
 *
 * Purpose:
 *  This file contains configurable parameters for the Application Layer.
 *
 * Description:
 *  - This file allows hardware-dependent or project-specific settings
 *    to be modified without changing the application logic (App.c).
 *  - It supports clean separation between:
 *        - Application logic (WHAT the system does)
 *        - Configuration (HOW the system is connected)
 *
 * Why this is important:
 *  - Improves portability of the code
 *  - Makes the application reusable across different hardware setups
 *  - Reduces risk of breaking logic when changing peripherals
 *
 * Design Rule:
 *  ---------------------------------------------------
 *  App.c should NEVER use hardcoded hardware values.
 *  All hardware selections must come from this file.
 ******************************************************************************************/


/******************************************************************************************
 *                              APP_USART_NUM_
 *
 * Purpose:
 *  Selects which USART peripheral is used for communication with the GUI (PC).
 *
 * Description:
 *  - The application communicates with the external GUI using a custom protocol.
 *  - This macro defines which USART instance is used for that communication.
 *
 * Current Configuration:
 *  - USART3 is used for GUI communication.
 *
 * Example Alternatives:
 *  USART_NUM_1
 *  USART_NUM_2
 *  USART_NUM_3
 *
 * Notes:
 *  - If the hardware connection changes, only this macro needs to be updated.
 *  - No need to modify App.c or protocol logic.
 *  - This follows good embedded design practice (configurable drivers).
 ******************************************************************************************/
#define APP_USART_NUM_     USART_NUM_3


#endif /* APP_APP_CFG_H_ */
