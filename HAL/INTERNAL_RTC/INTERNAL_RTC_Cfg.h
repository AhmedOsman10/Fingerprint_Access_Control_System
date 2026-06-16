/*
 * INTERNAL_RTC_Cfg.h
 *
 *  Configuration file for the STM32 internal RTC sleep/wake driver.
 */

#ifndef INTERNAL_RTC_INTERNAL_RTC_CFG_H_
#define INTERNAL_RTC_INTERNAL_RTC_CFG_H_

/* Internal RTC clock source currently uses LSI.
 * With LSI and the current prescalers, the alarm is good enough for
 * sleep/wake scheduling but not as accurate as the external DS3231 RTC.
 */
#define INTERNAL_RTC_USE_LSI        1U

#endif /* INTERNAL_RTC_INTERNAL_RTC_CFG_H_ */
