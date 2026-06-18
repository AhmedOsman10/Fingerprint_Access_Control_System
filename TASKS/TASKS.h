/******************************************************************************
 * @file    TASKS.h
 * @author  Ahmed Abdelrhman
 * @brief   Public interface for the FreeRTOS task layer.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This header exposes the task initialization function and the FreeRTOS task
 * entry functions used by the system startup code.
 *
 * @note    Only comments, spacing, and readability were updated for GitHub.
 *          The function declarations are intentionally unchanged.
 ******************************************************************************/

#ifndef TASKS_H_
#define TASKS_H_

/******************************************************************************
 * @brief Initialize application modules before starting task execution.
 ******************************************************************************/
void TASKS_Init(void);

/******************************************************************************
 * @brief FreeRTOS task entry for the fingerprint cyclic handler.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 ******************************************************************************/
void FP_Main_Cyclic(void *pram);

/******************************************************************************
 * @brief FreeRTOS task entry for the application cyclic handler.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 ******************************************************************************/
void TASKS_APP_Cyclic(void *pram);

/******************************************************************************
 * @brief FreeRTOS task entry for the relay timing cyclic handler.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 ******************************************************************************/
void TASKS_RELAY_Cyclic(void *pram);

/******************************************************************************
 * @brief FreeRTOS task entry for LED toggle testing.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 ******************************************************************************/
void TASKS_Led_Toggle(void *pram);

#endif /* TASKS_H_ */
