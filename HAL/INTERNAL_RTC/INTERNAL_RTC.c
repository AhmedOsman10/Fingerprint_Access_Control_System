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

/* Flag set when PB7 / EXTI7 wakes the MCU */
static volatile uint8_t INTERNAL_RTC_EXTI7_WakeupFlag = 0;

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

		__HAL_RCC_LSI_ENABLE();
		while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);

		__HAL_RCC_BACKUPRESET_FORCE();
		__HAL_RCC_BACKUPRESET_RELEASE();

		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_RCC_RTC_ENABLE();

		HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 5, 0);
		HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
	}
}

static void INTERNAL_RTC_PB7_EXTI_Init(void)
{
	GPIO_InitTypeDef GPIO_Init = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_Init.Pin = GPIO_PIN_7;
	GPIO_Init.Mode = GPIO_MODE_IT_FALLING;
	GPIO_Init.Pull = GPIO_PULLUP;

	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/******************************************************************************************
 *                              INTERNAL_RTC_Init()
 ******************************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void)
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

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

	sTime.Hours = 0;
	sTime.Minutes = 0;
	sTime.Seconds = 0;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if(HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		return INTERNAL_RTC_Nok;
	}

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;
	sDate.Month = RTC_MONTH_JANUARY;
	sDate.Date = 1;
	sDate.Year = 0;

	if(HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		return INTERNAL_RTC_Nok;
	}

	INTERNAL_RTC_PB7_EXTI_Init();

	return INTERNAL_RTC_Ok;
}

/******************************************************************************************
 *                         INTERNAL_RTC_EnterSleepMode()
 *
 * Used when the application wants to sleep now and wake at a specific RTC alarm time.
 ******************************************************************************************/
void INTERNAL_RTC_EnterSleepMode(uint8_t wake_hour, uint8_t wake_minute)
{
	RTC_Time_t external_rtc_time;
	RTC_TimeTypeDef internal_rtc_time = {0};
	RTC_AlarmTypeDef alarm = {0};
	GPIO_InitTypeDef GPIO_Init = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_Init.Pin = GPIO_PIN_3;
	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

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

	alarm.AlarmTime.Hours = wake_hour;
	alarm.AlarmTime.Minutes = wake_minute;
	alarm.AlarmTime.Seconds = 0;
	alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

	alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
	alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	alarm.AlarmDateWeekDay = 1;
	alarm.Alarm = RTC_ALARM_A;

	vTaskSuspendAll();
	HAL_SuspendTick();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	EXTI->PR = (1U << 17);

	if(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
	NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

	NVIC_DisableIRQ(USART2_IRQn);
	NVIC_DisableIRQ(USART3_IRQn);

	HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	NVIC_EnableIRQ(USART2_IRQn);
	NVIC_EnableIRQ(USART3_IRQn);

	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	HAL_ResumeTick();
	xTaskResumeAll();



	for(uint8_t i = 0; i < 10; i++)
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

/******************************************************************************************
 *                         INTERNAL_RTC_EnterSleepModeOnly()
 *
 * Used when the application wants to sleep immediately without setting a new RTC alarm.
 *
 * Wake-up source:
 *   - PB7 / EXTI7
 *   - Any already configured RTC alarm
 ******************************************************************************************/
void INTERNAL_RTC_EnterSleepModeOnly(void)
{
	GPIO_InitTypeDef GPIO_Init = {0};

	__HAL_RCC_GPIOB_CLK_ENABLE();

	GPIO_Init.Pin = GPIO_PIN_3;
	GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_Init.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_Init);

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

	vTaskSuspendAll();
	HAL_SuspendTick();
	SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);
	NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

	NVIC_DisableIRQ(USART2_IRQn);
	NVIC_DisableIRQ(USART3_IRQn);

	HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	NVIC_EnableIRQ(USART2_IRQn);
	NVIC_EnableIRQ(USART3_IRQn);

	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
	HAL_ResumeTick();
	xTaskResumeAll();

	for(uint8_t i = 0; i < 10; i++)
	{
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

/******************************************************************************************
 *                  INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag()
 *
 * Returns whether PB7 / EXTI7 caused a wake-up event.
 ******************************************************************************************/
uint8_t INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag(void)
{
	uint8_t flag = INTERNAL_RTC_EXTI7_WakeupFlag;

	INTERNAL_RTC_EXTI7_WakeupFlag = 0;

	return flag;
}

/******************************************************************************************
 *                         HAL_GPIO_EXTI_Callback()
 *
 * HAL callback called from EXTI9_5_IRQHandler().
 * Used only to mark that PB7 / EXTI7 triggered.
 ******************************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == GPIO_PIN_7)
	{
		INTERNAL_RTC_EXTI7_WakeupFlag = 1;
	}
}
