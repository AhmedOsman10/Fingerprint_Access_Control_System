/******************************************************************************
 * @file    INTERNAL_RTC_Cfg.h
 * @author  Ahmed Abdelrhman
 * @brief   Configuration file for the STM32 internal RTC driver.
 *
 * @details
 * This file contains configuration macros for the internal RTC sleep/wake
 * driver. The internal RTC is used as a wake-up source, not as the main time
 * reference for logging.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef INTERNAL_RTC_INTERNAL_RTC_CFG_H_
#define INTERNAL_RTC_INTERNAL_RTC_CFG_H_

/******************************************************************************
 * @brief Select LSI as the internal RTC clock source.
 *
 * @details
 * LSI is available internally on the STM32 and can keep the RTC running during
 * low-power operation. It is suitable for wake-up scheduling in this project,
 * while the external RTC module remains better for accurate timekeeping.
 ******************************************************************************/
#define INTERNAL_RTC_USE_LSI    1U

#endif /* INTERNAL_RTC_INTERNAL_RTC_CFG_H_ */
