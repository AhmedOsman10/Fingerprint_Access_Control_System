/*
 * INTERNAL_RTC.h
 *
 *  Created on: 16 Jun 2026
 *      Author: Ahmed
 *
 *  Internal RTC driver interface.
 *
 *  This driver manages the STM32 internal RTC used for sleep/wake-up.
 *  It is intentionally separated from the external RTC driver used for
 *  real-time clock/calendar logging.
 */

#ifndef INTERNAL_RTC_INTERNAL_RTC_H_
#define INTERNAL_RTC_INTERNAL_RTC_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "RTC.h"

typedef enum
{
    INTERNAL_RTC_Ok = 0,
    INTERNAL_RTC_Nok
}INTERNAL_RTC_Err_t;

/**
 * @brief Initialize STM32 internal RTC and RTC Alarm interrupt.
 *
 * @note  This RTC is used only as the MCU wake-up source from sleep mode.
 */
INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void);

/**
 * @brief Copy time from the external RTC driver to the STM32 internal RTC.
 *
 * @param external_time Pointer to time read from the external RTC module.
 */
INTERNAL_RTC_Err_t INTERNAL_RTC_SyncFromExternal(const RTC_Time_t *external_time);

/**
 * @brief Configure internal RTC Alarm A to wake the MCU at a target time.
 *
 * @param wake_hour   Target wake-up hour in 24-hour format.
 * @param wake_minute Target wake-up minute.
 */
INTERNAL_RTC_Err_t INTERNAL_RTC_SetWakeAlarm(uint8_t wake_hour, uint8_t wake_minute);

/**
 * @brief Clear old RTC alarm and EXTI pending flags before entering sleep.
 */
void INTERNAL_RTC_ClearWakeFlags(void);



void INTERNAL_RTC_EnterSleepMode(uint8_t wake_hour, uint8_t wake_minute);

/**
 * @brief HAL RTC handle used by the interrupt handler.
 */
extern RTC_HandleTypeDef hrtc;

#endif /* INTERNAL_RTC_INTERNAL_RTC_H_ */
