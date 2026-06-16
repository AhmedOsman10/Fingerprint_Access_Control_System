/*
 * INTERNAL_RTC.c
 *
 * Internal RTC driver for STM32 sleep/wake-up handling.
 */

#include "INTERNAL_RTC.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_pwr.h"
#include "FreeRTOS.h"
#include "task.h"
#include "RTC.h"
#include "Sys.h"

/* Internal RTC handle owned only by this driver */
RTC_HandleTypeDef hrtc;


/******************************************************************************************
 *                              HAL_RTC_MspInit()
 ******************************************************************************************/
void HAL_RTC_MspInit(RTC_HandleTypeDef* rtcHandle)
{
	if(rtcHandle->Instance == RTC)
	{
		RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

		__HAL_RCC_PWR_CLK_ENABLE();
		HAL_PWR_EnableBkUpAccess();

		/* Enable LSI oscillator first */
		__HAL_RCC_LSI_ENABLE();

		/* Select LSI as RTC clock source */
		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

		if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			return;
		}

		__HAL_RCC_RTC_ENABLE();

		HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
		HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
	}
}

/******************************************************************************************
 *                              INTERNAL_RTC_Init()
 ******************************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void)
{
	hrtc.Instance = RTC;
	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	hrtc.Init.AsynchPrediv = 127;
	hrtc.Init.SynchPrediv = 255;
	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	if(HAL_RTC_Init(&hrtc) != HAL_OK)
	{
		return INTERNAL_RTC_Nok;
	}

	return INTERNAL_RTC_Ok;
}


/******************************************************************************************
 *                         INTERNAL_RTC_EnterSleepMode()
 *
 * APP layer calls this function only.
 * Internal RTC driver handles:
 *   - RTC time sync
 *   - RTC alarm setup
 *   - RTC flag clearing
 *   - FreeRTOS tick suspension/restoration
 *   - Sleep mode entry
 ******************************************************************************************/
void INTERNAL_RTC_EnterSleepMode(uint8_t wake_hour, uint8_t wake_minute)
{
	RTC_Time_t external_rtc_time;
	RTC_TimeTypeDef internal_rtc_time = {0};
	RTC_AlarmTypeDef alarm = {0};

	GPIO_InitTypeDef GPIO_Init = {0};

	/* LED setup for sleep/wake test */
	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_Init.Pin = GPIO_PIN_3;
	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	/* Turn off LED before sleeping */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

	/* 1. Synchronize internal RTC with external RTC */
	RTC_GetTime(&external_rtc_time);

	internal_rtc_time.Hours = external_rtc_time.hours;
	internal_rtc_time.Minutes = external_rtc_time.minutes;
	internal_rtc_time.Seconds = external_rtc_time.seconds;
	internal_rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	internal_rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

	if(HAL_RTC_SetTime(&hrtc, &internal_rtc_time, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	/* 2. Configure RTC Alarm A */
	alarm.AlarmTime.Hours = wake_hour;
	alarm.AlarmTime.Minutes = wake_minute;
	alarm.AlarmTime.Seconds = 0;
	alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

	/* Ignore date, compare only time */
	alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
	alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	alarm.AlarmDateWeekDay = 1;
	alarm.Alarm = RTC_ALARM_A;

	/* 3. Suspend FreeRTOS and HAL tick */
	vTaskSuspendAll();
	HAL_SuspendTick();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

	/* 4. Clear old wake-up / interrupt flags */
	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
	NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	/* Clear EXTI line 17 pending flag for RTC Alarm */
	EXTI->PR = (1U << 17);

	/* 5. Set RTC alarm interrupt */
	if(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	/* 6. Disable interrupts that can wake CPU earlier than RTC alarm */
	NVIC_DisableIRQ(USART2_IRQn);
	NVIC_DisableIRQ(USART3_IRQn);
	NVIC_DisableIRQ(EXTI9_5_IRQn);

	/* 7. Enter Sleep Mode */
	HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	/* ==========================================================
	 * Wake-up point
	 * CPU continues from here after RTC alarm interrupt
	 * ========================================================== */

	/* 8. Re-enable interrupts */
	NVIC_EnableIRQ(USART2_IRQn);
	NVIC_EnableIRQ(USART3_IRQn);
	NVIC_EnableIRQ(EXTI9_5_IRQn);

	/* 9. Restore HAL tick and FreeRTOS scheduler */
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	HAL_ResumeTick();
	xTaskResumeAll();

	/* 10. Wake-up indication LED blink */
	for(uint8_t i = 0; i < 10; i++)
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}
