/*
 * FP_Cfg.h
 *
 *  Created on: 4 Feb 2026
 *      Author: Ahmed
 */

#ifndef FP_FP_CFG_H_
#define FP_FP_CFG_H_

/******************************************************************************************
 *                                  FP Configuration File
 *
 * Purpose:
 *  This file contains configuration macros for the fingerprint driver.
 *
 * Description:
 *  - This file allows you to configure hardware-related parameters
 *    without modifying the driver source code.
 *  - It is part of the driver abstraction concept:
 *        Driver logic (FP.c)  → fixed
 *        Configuration (FP_Cfg.h) → changeable
 *
 * Notes:
 *  - Any hardware-specific selection (USART, pins, etc.) should be placed here.
 *  - This makes the driver reusable across different projects.
 ******************************************************************************************/


/******************************************************************************************
 *                              FP_USART_NUM_
 *
 * Purpose:
 *  Selects which USART peripheral is used to communicate with the fingerprint module.
 *
 * Description:
 *  - This macro maps the fingerprint driver to a specific USART instance.
 *  - The value should match one of the USART identifiers defined in the USART driver.
 *
 * Example:
 *  USART_NUM_1
 *  USART_NUM_2
 *  USART_NUM_3
 *
 * Current Configuration:
 *  - Fingerprint module is connected to USART2
 *
 * Notes:
 *  - If you change the hardware connection, update this macro only.
 *  - No need to modify FP.c or other driver files.
 ******************************************************************************************/
#define FP_USART_NUM_     USART_NUM_2


#endif /* FP_FP_CFG_H_ */
