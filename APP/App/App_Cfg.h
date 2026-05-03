/*
 * App_Cfg.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 *
 *  Configuration file for the Application Layer.
 *
 *  This file contains the configurable hardware-related settings used by
 *  the application layer.
 *
 *  Design Rule:
 *    App.c shall not use hardcoded hardware selections directly.
 *    All such selections shall be defined here.
 *
 *  Benefits:
 *    - improves portability
 *    - simplifies hardware changes
 *    - keeps application logic clean
 */

#ifndef APP_APP_CFG_H_
#define APP_APP_CFG_H_

/******************************************************************************************
 *                                  APP_USART_NUM_
 *
 *  USART instance used by the application layer for GUI communication.
 *
 *  APP_USART_NUM_: Selects the UART/USART channel connected to the GUI or PC.
 *
 *  Current Configuration:
 *    USART_NUM_3
 *
 *  Application Use:
 *    This macro is used internally by App.c when:
 *      - receiving GUI frames
 *      - sending access logs
 *      - sending enrollment status updates
 *
 *  Notes:
 *    - If the hardware connection changes, only this macro should be updated.
 *    - No change is required in the application logic itself.
 ******************************************************************************************/
#define APP_USART_NUM_     USART_NUM_3


#endif /* APP_APP_CFG_H_ */
