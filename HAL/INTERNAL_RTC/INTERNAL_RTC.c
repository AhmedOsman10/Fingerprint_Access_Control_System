/*
 * INTERNAL_RTC.c
 *
 *  Created on: 16 Jun 2026
 *      Author: Ahmed
 *
 *  STM32 internal RTC driver implementation.
 *
 *  Responsibilities:
 *    - initialize the STM32 internal RTC peripheral
 *    - configure RTC Alarm A interrupt
 *    - synchronize internal RTC time from the external RTC module
 *    - prepare RTC alarm flags before entering sleep mode
 *
 *  Layering note:
 *    APP layer decides WHEN the system should sleep.
 *    This driver handles HOW the internal RTC wakes the MCU.
 */

#include "INTERNAL_RTC.h"
#include "INTERNAL_RTC_Cfg.h"
#include "INTERNAL_RTC_Prv.h"
#include "main.h"

RTC_HandleTypeDef hrtc;

/**
  * @brief RTC MSP Initialization.
  *
  * This function is called by HAL_RTC_Init().
  * It enables the backup domain, selects LSI as RTC clock source,
  * enables the RTC peripheral clock, and enables the RTC Alarm IRQ.
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtc_handle)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    if(rtc_handle->Instance == RTC)
    {
        __HAL_RCC_PWR_CLK_ENABLE();
        HAL_PWR_EnableBkUpAccess();

#if (INTERNAL_RTC_USE_LSI == 1U)
        __HAL_RCC_LSI_ENABLE();
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);

        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();

        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif

        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            Error_Handler();
        }

        __HAL_RCC_RTC_ENABLE();

        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, INTERNAL_RTC_NVIC_PREEMPT_PRIO, INTERNAL_RTC_NVIC_SUB_PRIO);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    }
}

INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void)
{
    INTERNAL_RTC_Err_t err_st = INTERNAL_RTC_Ok;
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        err_st = INTERNAL_RTC_Nok;
    }
    else
    {
        /* Default time/date. The real time is synchronized before sleeping. */
        sTime.Hours = 0x0;
        sTime.Minutes = 0x0;
        sTime.Seconds = 0x0;
        sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        sTime.StoreOperation = RTC_STOREOPERATION_RESET;

        if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
        {
            err_st = INTERNAL_RTC_Nok;
        }

        sDate.WeekDay = RTC_WEEKDAY_MONDAY;
        sDate.Month = RTC_MONTH_JANUARY;
        sDate.Date = 0x1;
        sDate.Year = 0x0;

        if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
        {
            err_st = INTERNAL_RTC_Nok;
        }
    }

    return err_st;
}

INTERNAL_RTC_Err_t INTERNAL_RTC_SyncFromExternal(const RTC_Time_t *external_time)
{
    INTERNAL_RTC_Err_t err_st = INTERNAL_RTC_Ok;
    RTC_TimeTypeDef internal_time = {0};

    if(external_time == NULL)
    {
        err_st = INTERNAL_RTC_Nok;
    }
    else
    {
        internal_time.Hours   = external_time->hours;
        internal_time.Minutes = external_time->minutes;
        internal_time.Seconds = external_time->seconds;
        internal_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
        internal_time.StoreOperation = RTC_STOREOPERATION_RESET;

        if(HAL_RTC_SetTime(&hrtc, &internal_time, RTC_FORMAT_BIN) != HAL_OK)
        {
            err_st = INTERNAL_RTC_Nok;
        }
    }

    return err_st;
}

INTERNAL_RTC_Err_t INTERNAL_RTC_SetWakeAlarm(uint8_t wake_hour, uint8_t wake_minute)
{
    INTERNAL_RTC_Err_t err_st = INTERNAL_RTC_Ok;
    RTC_AlarmTypeDef alarm = {0};

    alarm.AlarmTime.Hours   = wake_hour;
    alarm.AlarmTime.Minutes = wake_minute;
    alarm.AlarmTime.Seconds = 0;
    alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

    /* Ignore date. Alarm triggers when hour/minute/second match. */
    alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay = INTERNAL_RTC_WAKE_ALARM_DATE;
    alarm.Alarm = RTC_ALARM_A;

    if(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
    {
        err_st = INTERNAL_RTC_Nok;
    }

    return err_st;
}

void INTERNAL_RTC_ClearWakeFlags(void)
{
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* RTC Alarm uses EXTI line 17 on STM32F4. */
    EXTI->PR = (1U << 17);
}
