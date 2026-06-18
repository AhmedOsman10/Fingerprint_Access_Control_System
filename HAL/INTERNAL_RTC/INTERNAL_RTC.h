/******************************************************************************
 * @file    INTERNAL_RTC.h
 * @author  Ahmed Abdelrhman
 * @brief   Public interface for the STM32 internal RTC sleep/wake driver.
 *
 * @details
 * This driver uses the STM32F407 internal RTC as a wake-up source for sleep
 * mode. The external RTC module remains the main time reference for the system,
 * while the internal RTC is synchronized before sleep to generate wake alarms.
 *
 * Main responsibilities:
 * - Initialize the STM32 internal RTC.
 * - Configure RTC Alarm A wake-up time.
 * - Enter sleep mode safely with FreeRTOS.
 * - Support PB7 / EXTI7 wake-up detection.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef INTERNAL_RTC_INTERNAL_RTC_H_
#define INTERNAL_RTC_INTERNAL_RTC_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "RTC.h"

/******************************************************************************
 * @brief Internal RTC driver status values.
 ******************************************************************************/
typedef enum
{
    /* Operation completed successfully. */
    INTERNAL_RTC_Ok = 0,

    /* Operation failed. */
    INTERNAL_RTC_Nok

} INTERNAL_RTC_Err_t;

/******************************************************************************
 * @brief  Initialize STM32 internal RTC and wake-up interrupt sources.
 * @return Internal RTC driver status.
 ******************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void);

/******************************************************************************
 * @brief  Copy time from the external RTC driver to the STM32 internal RTC.
 * @param  external_time Pointer to time read from the external RTC module.
 * @return Internal RTC driver status.
 *
 * @note   Prototype kept unchanged from the project interface.
 ******************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_SyncFromExternal(const RTC_Time_t *external_time);

/******************************************************************************
 * @brief  Configure internal RTC Alarm A to wake the MCU at a target time.
 * @param  wake_hour   Target wake-up hour in 24-hour format.
 * @param  wake_minute Target wake-up minute.
 * @return Internal RTC driver status.
 *
 * @note   Prototype kept unchanged from the project interface.
 ******************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_SetWakeAlarm(uint8_t wake_hour, uint8_t wake_minute);

/******************************************************************************
 * @brief Clear old RTC alarm, power, and EXTI wake-up flags before sleep.
 *
 * @note  Prototype kept unchanged from the project interface.
 ******************************************************************************/
void INTERNAL_RTC_ClearWakeFlags(void);

/******************************************************************************
 * @brief Enter sleep mode and configure RTC Alarm A as the wake-up time.
 * @param wake_hour   Target wake-up hour in 24-hour format.
 * @param wake_minute Target wake-up minute.
 ******************************************************************************/
void INTERNAL_RTC_EnterSleepMode(uint8_t wake_hour, uint8_t wake_minute);

/******************************************************************************
 * @brief Enter sleep mode without configuring a new RTC alarm.
 ******************************************************************************/
void INTERNAL_RTC_EnterSleepModeOnly(void);

/******************************************************************************
 * @brief  Read and clear the PB7 / EXTI7 wake-up flag.
 * @return 1 if PB7 wake-up happened, otherwise 0.
 ******************************************************************************/
uint8_t INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag(void);

/******************************************************************************
 * @brief HAL RTC handle used by the RTC driver and interrupt handler.
 ******************************************************************************/
extern RTC_HandleTypeDef hrtc;

#endif /* INTERNAL_RTC_INTERNAL_RTC_H_ */
