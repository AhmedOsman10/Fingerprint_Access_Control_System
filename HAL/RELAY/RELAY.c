/******************************************************************************
 * @file    RELAY.c
 * @author  Ahmed Abdelrhman
 * @brief   Relay driver implementation.
 *
 * @details
 * This driver controls the relay output used by the Fingerprint Access Control
 * System. The relay can be switched ON, switched OFF, or switched ON for a
 * limited time using the non-blocking cyclic time manager.
 *
 * Important design note:
 * - RELAY_On_With_Time() does not block the CPU.
 * - It stores the start time and target duration.
 * - RELAY_Time_Manager_Cyclic() must be called periodically from a task.
 * - When the requested time expires, the relay is turned OFF automatically.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_gpio.h"

#include "FreeRTOS.h"
#include "task.h"

#include "RELAY_Cfg.h"
#include "RELAY_Prv.h"
#include "RELAY.h"

/******************************************************************************
 * @brief  Initialize one relay GPIO pin as an output.
 *
 * @param  Relay_num Relay index selected from the relay configuration table.
 *
 * @retval RELAY_Init_Success  Relay GPIO initialized successfully.
 * @retval RElAY_Invalid_Args  Relay index is outside the configured range.
 * @retval RELAY_Init_Failed   Default state before successful initialization.
 *
 * @details
 * The relay pin information is taken from RELAY_Config[] in RELAY_Cfg.h.
 * For each relay, the configuration table stores:
 * - GPIO port
 * - GPIO pin
 *
 * The pin is configured as:
 * - Output push-pull
 * - No internal pull-up / pull-down
 * - Medium speed
 ******************************************************************************/
RELAY_Err_St_t RELAY_Init(uint8_t Relay_num)
{
    /* Start with a safe default error state until initialization succeeds. */
    RELAY_Err_St_t RELAY_Err_St = RELAY_Init_Failed;

    /* Check that the requested relay number exists in the configuration table. */
    if (Relay_num >= RELAY_MAX_NUM)
    {
        RELAY_Err_St = RElAY_Invalid_Args;
    }
    else
    {
        /* Prepare the HAL GPIO configuration structure for the relay output pin. */
        GPIO_InitTypeDef GPIO_Init = {
            .Pin   = RELAY_Config[Relay_num].Pin_Num,
            .Mode  = GPIO_MODE_OUTPUT_PP,
            .Pull  = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_MEDIUM
        };

        /* Apply the GPIO configuration to the selected relay port. */
        HAL_GPIO_Init(RELAY_Config[Relay_num].Port_Num, &GPIO_Init);

        /* Report successful initialization. */
        RELAY_Err_St = RELAY_Init_Success;
    }

    /* Return the final initialization result to the caller. */
    return RELAY_Err_St;
}

/******************************************************************************
 * @brief  Turn ON the selected relay.
 *
 * @param  Relay_num Relay index selected from the relay configuration table.
 *
 * @retval RELAY_On_Ok        Relay output was set successfully.
 * @retval RElAY_Invalid_Args Relay index is outside the configured range.
 *
 * @details
 * The driver writes GPIO_PIN_SET to the configured relay pin.
 * In this project, the relay circuit is driven from the configured STM32 GPIO.
 ******************************************************************************/
RELAY_Err_St_t RELAY_On(uint8_t Relay_num)
{
    /* Reject invalid relay numbers before accessing the configuration table. */
    if (Relay_num >= RELAY_MAX_NUM)
    {
        return RElAY_Invalid_Args;
    }

    /* Set the relay GPIO pin HIGH to activate the relay output. */
    HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_SET);

    /* Inform the caller that the relay ON command was accepted. */
    return RELAY_On_Ok;
}

/******************************************************************************
 * @brief  Turn OFF the selected relay.
 *
 * @param  Relay_num Relay index selected from the relay configuration table.
 *
 * @retval RELAY_Off_Ok       Relay output was cleared successfully.
 * @retval RElAY_Invalid_Args Relay index is outside the configured range.
 *
 * @details
 * The driver writes GPIO_PIN_RESET to the configured relay pin.
 ******************************************************************************/
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num)
{
    /* Reject invalid relay numbers before accessing the configuration table. */
    if (Relay_num >= RELAY_MAX_NUM)
    {
        return RElAY_Invalid_Args;
    }

    /* Set the relay GPIO pin LOW to deactivate the relay output. */
    HAL_GPIO_WritePin(RELAY_Config[Relay_num].Port_Num, RELAY_Config[Relay_num].Pin_Num, GPIO_PIN_RESET);

    /* Inform the caller that the relay OFF command was accepted. */
    return RELAY_Off_Ok;
}

/******************************************************************************
 * @brief  Turn ON the selected relay for a specific time without blocking.
 *
 * @param  Relay_num      Relay index selected from the relay configuration table.
 * @param  Relay_Time_ms  Requested ON duration in milliseconds.
 *
 * @retval RELAY_On_Ok        Timed relay operation was started successfully.
 * @retval RElAY_Invalid_Args Relay index is outside the configured range.
 *
 * @details
 * This function starts a timed relay operation by:
 * 1. Turning the relay ON immediately.
 * 2. Marking the relay timer as active.
 * 3. Saving the current FreeRTOS tick count as the start time.
 * 4. Converting the requested duration from milliseconds to RTOS ticks.
 *
 * The relay is not turned OFF inside this function. Instead,
 * RELAY_Time_Manager_Cyclic() checks the elapsed time and turns the relay OFF
 * when the deadline is reached. This keeps the system non-blocking.
 ******************************************************************************/
RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num, uint32_t Relay_Time_ms)
{
    /* Reject invalid relay numbers before accessing timing/control arrays. */
    if (Relay_num >= RELAY_MAX_NUM)
    {
        return RElAY_Invalid_Args;
    }

    /* Turn the relay ON immediately. */
    RELAY_On(Relay_num);

    /* Mark this relay as currently controlled by the timing manager. */
    Relay_Control_Time[Relay_num].is_active = true;

    /* Store the current RTOS tick count as the start time. */
    Relay_Control_Time[Relay_num].start_time = xTaskGetTickCount();

    /* Convert the requested time from milliseconds to FreeRTOS ticks. */
    Relay_Control_Time[Relay_num].deadline_ticks = pdMS_TO_TICKS(Relay_Time_ms);

    /* Inform the caller that the timed ON operation was started. */
    return RELAY_On_Ok;
}

/******************************************************************************
 * @brief  Cyclic relay timing manager.
 *
 * @details
 * This function must be called periodically from a FreeRTOS task.
 * It checks all configured relays and turns OFF any relay whose timed ON period
 * has expired.
 *
 * Timing method:
 * - curr_time is the current FreeRTOS tick count.
 * - start_time is the tick count saved when the relay was turned ON.
 * - elapsed_time is calculated using subtraction.
 * - deadline_ticks is the requested ON duration converted to ticks.
 *
 * This design avoids blocking delays, so other application tasks can continue
 * running while the relay remains ON.
 ******************************************************************************/
void RELAY_Time_Manager_Cyclic(void)
{
    /* Read the current FreeRTOS tick count once for this cyclic execution. */
    uint32_t curr_time = xTaskGetTickCount();

    /* Check every configured relay channel. */
    for (uint8_t Relay_num = 0; Relay_num < RELAY_MAX_NUM; Relay_num++)
    {
        /* Only process relays that currently have an active timed operation. */
        if (Relay_Control_Time[Relay_num].is_active == true)
        {
            /* Calculate how long this relay has been ON in RTOS ticks. */
            uint32_t elapsed_time = curr_time - Relay_Control_Time[Relay_num].start_time;

            /* If the elapsed time exceeded the requested duration, turn relay OFF. */
            if (elapsed_time > Relay_Control_Time[Relay_num].deadline_ticks)
            {
                /* Deactivate the relay output. */
                RELAY_Off(Relay_num);

                /* Stop timing this relay until RELAY_On_With_Time() is called again. */
                Relay_Control_Time[Relay_num].is_active = false;
            }
        }
    }
}
