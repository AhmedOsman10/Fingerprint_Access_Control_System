/******************************************************************************
 * @file    INTERNAL_RTC_Prv.h
 * @author  Ahmed Abdelrhman
 * @brief   Private definitions for the STM32 internal RTC driver.
 *
 * @details
 * This file contains constants and private function prototypes used only by
 * INTERNAL_RTC.c. It keeps implementation details separated from the public
 * interface in INTERNAL_RTC.h.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef INTERNAL_RTC_INTERNAL_RTC_PRV_H_
#define INTERNAL_RTC_INTERNAL_RTC_PRV_H_

/******************************************************************************
 * @brief Dummy alarm date value.
 *
 * @details
 * The RTC alarm masks the date/weekday field, so this value is not used for
 * matching. It is still required by the HAL alarm configuration structure.
 ******************************************************************************/
#define INTERNAL_RTC_WAKE_ALARM_DATE       1U

/* NVIC preemption priority used for RTC alarm and EXTI wake-up interrupts. */
#define INTERNAL_RTC_NVIC_PREEMPT_PRIO     5U

/* NVIC sub-priority used for RTC alarm and EXTI wake-up interrupts. */
#define INTERNAL_RTC_NVIC_SUB_PRIO         0U

/******************************************************************************
 * @brief Configure PB7 / EXTI7 as an external wake-up interrupt input.
 ******************************************************************************/
static void INTERNAL_RTC_PB7_EXTI_Init(void);

#endif /* INTERNAL_RTC_INTERNAL_RTC_PRV_H_ */
