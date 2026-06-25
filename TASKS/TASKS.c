/******************************************************************************
 * @file    TASKS.c
 * @author  Ahmed Abdelrhman
 * @brief   FreeRTOS task layer implementation.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * This module connects the application modules to the FreeRTOS scheduler.
 * Each task calls one cyclic function periodically:
 *  - Fingerprint task  : Handles fingerprint sensor state machine.
 *  - Application task  : Handles GUI frames, access decisions, and logging.
 *  - Relay task        : Handles relay timing and automatic relay shutdown.
 *
 * @note    Only comments, spacing, and readability were updated for GitHub.
 *          The task behavior and application logic are intentionally unchanged.
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "TASKS.h"
#include "RELAY.h"
#include "FP.h"
#include "Sys.h"
#include "App.h"
#include "INTERNAL_RTC.h"

/******************************************************************************
 * @brief Initialize modules required before starting the scheduler.
 *
 * This function initializes the high-level application layer and the internal
 * RTC module before the FreeRTOS tasks begin cyclic execution.
 ******************************************************************************/
void TASKS_Init(void)
{
    APP_Init();
    INTERNAL_RTC_Init();
}

/******************************************************************************
 * @brief FreeRTOS task for the fingerprint module.
 *
 * This task periodically calls the fingerprint cyclic function. The fingerprint
 * driver handles its own internal state machine, including search/enrollment
 * processing and UART communication with the fingerprint sensor.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 *
 * @note The task period is 400 ms.
 ******************************************************************************/
void FP_Main_Cyclic(void *pram)
{
    TickType_t last_wake = xTaskGetTickCount();

    /* Optional debug/test mode selection kept disabled. */
    //    FP_SetMode(FP_ENROLL_MODE);

    while (1)
    {
        FP_MainFunction_Cyclic();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(400));
    }
}

/******************************************************************************
 * @brief FreeRTOS task for the main application layer.
 *
 * This task periodically executes the application cyclic function. The APP layer
 * handles GUI protocol parsing, enrollment requests, access results, RTC-based
 * decisions, and communication with other modules.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 *
 * @note The task period is 400 ms.
 ******************************************************************************/
void TASKS_APP_Cyclic(void *pram)
{
    TickType_t last_wake = xTaskGetTickCount();

    /* Optional debug/test mode selection kept disabled. */
    //    FP_SetMode(FP_ENROLL_MODE);

    while (1)
    {
        APP_Cyclic();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(400));
    }
}

/******************************************************************************
 * @brief FreeRTOS task for relay timing management.
 *
 * This task runs frequently so the relay module can update its timing logic and
 * turn the relay off after the configured activation period.
 *
 * @param pram FreeRTOS task parameter. Currently unused.
 *
 * @note The task period is 1 ms.
 ******************************************************************************/
void TASKS_RELAY_Cyclic(void *pram)
{
    TickType_t last_wake = xTaskGetTickCount();

    /* Optional debug/test mode selection kept disabled. */
    //    FP_SetMode(FP_ENROLL_MODE);

    while (1)
    {
        RELAY_Time_Manager_Cyclic();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
    }
}
