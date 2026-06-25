/* USER CODE BEGIN Header */
/******************************************************************************
 * @file    main.c
 * @author  Ahmed Abdelrhman
 * @brief   Main entry point for the Fingerprint Access Control System.
 *
 * @project Fingerprint Access Control System - STM32F407
 *
 * @details
 * This file initializes the hardware, configures the system services,
 * creates the application FreeRTOS tasks, and starts the scheduler.
 *
 * Created Tasks:
 *  - FP_Main_Cyclic      : Fingerprint processing task
 *  - TASKS_APP_Cyclic    : Application layer task
 *  - TASKS_RELAY_Cyclic  : Relay control task
 *
 * Application behavior and logic are intentionally unchanged.
 ******************************************************************************/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"
#include "Sys.h"
#include "TASKS.h"
#include "FreeRTOS.h"
#include "task.h"

/******************************************************************************
 * @brief  Application entry point.
 *
 * Initializes the system, creates all required FreeRTOS tasks,
 * and starts the scheduler. Execution should never return from
 * vTaskStartScheduler() under normal operation.
 *
 * @return int
 ******************************************************************************/
int main(void)
{
    BaseType_t ret_st;

    /* Initialize hardware and application modules */
    Sys_Init();
    TASKS_Init();

    /* Create fingerprint processing task */
    ret_st = xTaskCreate(FP_Main_Cyclic, "FP_Main_Cyclic", 100, NULL, 2, NULL);
    configASSERT(ret_st == pdPASS);

    /* Create application task */
    ret_st = xTaskCreate(TASKS_APP_Cyclic, "TASKS_APP_Cyclic", 400, NULL, 2, NULL);
    configASSERT(ret_st == pdPASS);

    /* Create relay control task */
    ret_st = xTaskCreate(TASKS_RELAY_Cyclic, "TASKS_RELAY_Cyclic", 400, NULL, 2, NULL);
    configASSERT(ret_st == pdPASS);

    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never be reached unless scheduler fails to start */
    while (1)
    {
        MX_USB_HOST_Process();
    }
}
