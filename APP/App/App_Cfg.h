/******************************************************************************
 * @file    App_Cfg.h
 * @author  Ahmed Abdelrhman
 * @brief   Configuration file for the Application Layer.
 *
 * @details This file contains configurable application settings. Hardware or
 *          communication channel selections should be changed here instead of
 *          being hardcoded inside App.c.
 *
 * @note    Only comments, spacing, and layout were improved. The application
 *          logic is intentionally kept the same.
 ******************************************************************************/

#ifndef APP_APP_CFG_H_
#define APP_APP_CFG_H_


/*=============================================================================
 * Application Communication Configuration
 *============================================================================*/

/* USART instance used for STM32 <-> GUI communication. */
#define APP_USART_NUM_  USART_NUM_3


#endif /* APP_APP_CFG_H_ */
