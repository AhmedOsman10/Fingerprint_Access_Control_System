/******************************************************************************
 * @file    INTERNAL_RTC.c
 * @author  Ahmed Abdelrhman
 * @brief   STM32 internal RTC sleep/wake driver implementation.
 *
 * @details
 * This driver uses the STM32F407 internal RTC as a wake-up source for the
 * Fingerprint Access Control System. The external RTC keeps the real project
 * time, while the internal RTC is used mainly to generate Alarm A and wake the
 * MCU from low-power sleep mode.
 *
 * Wake-up sources handled in this module:
 * - RTC Alarm A
 * - PB7 external interrupt through EXTI7
 *
 * FreeRTOS sleep handling in this module:
 * - Suspend the scheduler before sleep.
 * - Suspend HAL tick / SysTick interrupt before WFI.
 * - Enter sleep mode using WFI.
 * - Resume SysTick, HAL tick, and FreeRTOS scheduler after wake-up.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#include "INTERNAL_RTC.h"
#include "INTERNAL_RTC_Prv.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_pwr.h"

#include "FreeRTOS.h"
#include "task.h"

#include "RTC.h"
#include "Sys.h"

/* STM32 HAL RTC handle used by this driver and by the RTC alarm interrupt path. */
RTC_HandleTypeDef hrtc;

/*
 * PB7 / EXTI7 wake-up flag.
 *
 * volatile is required because this variable is changed from interrupt context
 * inside HAL_GPIO_EXTI_Callback() and read from normal application context.
 */
static volatile uint8_t INTERNAL_RTC_EXTI7_WakeupFlag = 0;

/******************************************************************************
 * @brief  STM32 HAL RTC MSP initialization callback.
 *
 * @param  rtcHandle Pointer to the HAL RTC handle being initialized.
 *
 * @details
 * HAL_RTC_Init() calls this function to prepare the low-level RTC hardware.
 * This function performs the following actions:
 * - Enables the PWR peripheral clock.
 * - Enables access to the backup domain.
 * - Enables the LSI oscillator.
 * - Resets the backup domain.
 * - Selects LSI as the RTC clock source.
 * - Enables the RTC peripheral clock.
 * - Enables the RTC Alarm interrupt in the NVIC.
 ******************************************************************************/
void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{
    /* Run this configuration only for the internal RTC peripheral instance. */
    if (rtcHandle->Instance == RTC)
    {
        /* Structure used to select the clock source for the RTC peripheral. */
        RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

        /* Enable power interface clock because backup domain access needs PWR. */
        __HAL_RCC_PWR_CLK_ENABLE();

        /* Allow write access to the RTC backup domain. */
        HAL_PWR_EnableBkUpAccess();

        /* Enable the internal low-speed oscillator used as the RTC clock source. */
        __HAL_RCC_LSI_ENABLE();

        /* Wait until LSI becomes stable and ready. */
        while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSIRDY) == RESET);

        /* Reset the backup domain to allow RTC clock source reconfiguration. */
        __HAL_RCC_BACKUPRESET_FORCE();
        __HAL_RCC_BACKUPRESET_RELEASE();

        /* Select RTC peripheral clock configuration. */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;

        /* Select LSI as the clock source for the internal RTC. */
        PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;

        /* Apply the RTC peripheral clock configuration. */
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        {
            Error_Handler();
        }

        /* Enable the internal RTC peripheral clock. */
        __HAL_RCC_RTC_ENABLE();

        /* Configure and enable the RTC Alarm interrupt. */
        HAL_NVIC_SetPriority(RTC_Alarm_IRQn, INTERNAL_RTC_NVIC_PREEMPT_PRIO, INTERNAL_RTC_NVIC_SUB_PRIO);
        HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
    }
}

/******************************************************************************
 * @brief  Configure PB7 as an external interrupt wake-up input.
 *
 * @details
 * PB7 is configured as:
 * - Input with interrupt on falling edge
 * - Internal pull-up enabled
 *
 * This means the interrupt is triggered when PB7 is pulled from HIGH to LOW.
 * The interrupt is handled through EXTI9_5_IRQn because EXTI lines 5 to 9 share
 * the same NVIC interrupt vector on STM32F4.
 ******************************************************************************/
static void INTERNAL_RTC_PB7_EXTI_Init(void)
{
    /* GPIO configuration structure for PB7. */
    GPIO_InitTypeDef GPIO_Init = {0};

    /* Enable GPIOB peripheral clock before configuring PB7. */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Select PB7 as the external interrupt pin. */
    GPIO_Init.Pin = GPIO_PIN_7;

    /* Trigger interrupt on falling edge. */
    GPIO_Init.Mode = GPIO_MODE_IT_FALLING;

    /* Keep PB7 HIGH by default using the internal pull-up resistor. */
    GPIO_Init.Pull = GPIO_PULLUP;

    /* Apply the GPIO/EXTI configuration. */
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Configure and enable the shared EXTI5-to-EXTI9 interrupt in the NVIC. */
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, INTERNAL_RTC_NVIC_PREEMPT_PRIO, INTERNAL_RTC_NVIC_SUB_PRIO);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/******************************************************************************
 * @brief  Initialize the STM32 internal RTC driver.
 *
 * @retval INTERNAL_RTC_Ok  Internal RTC initialized successfully.
 * @retval INTERNAL_RTC_Nok Internal RTC initialization failed.
 *
 * @details
 * This function configures the STM32 internal RTC with:
 * - 24-hour format
 * - LSI-based prescaler values
 * - RTC output disabled
 *
 * The time/date are initialized to a known default value. Later, before sleep,
 * the internal RTC time is synchronized from the external RTC module.
 ******************************************************************************/
INTERNAL_RTC_Err_t INTERNAL_RTC_Init(void)
{
    /* RTC time structure used to set the initial internal RTC time. */
    RTC_TimeTypeDef sTime = {0};

    /* RTC date structure used to set the initial internal RTC date. */
    RTC_DateTypeDef sDate = {0};

    /* Select the STM32 internal RTC peripheral instance. */
    hrtc.Instance = RTC;

    /* Use 24-hour time format. */
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;

    /* RTC asynchronous prescaler value. */
    hrtc.Init.AsynchPrediv = 127;

    /* RTC synchronous prescaler value. */
    hrtc.Init.SynchPrediv = 255;

    /* Disable RTC output pin because this project uses RTC only internally. */
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;

    /* Keep default output polarity setting. */
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;

    /* Keep default output type setting. */
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    /* Initialize the RTC peripheral through HAL. */
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        return INTERNAL_RTC_Nok;
    }

    /* Set a known startup time: 00:00:00. */
    sTime.Hours = 0;
    sTime.Minutes = 0;
    sTime.Seconds = 0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    /* Write the startup time to the internal RTC. */
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
    {
        return INTERNAL_RTC_Nok;
    }

    /* Set a known startup date. */
    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month = RTC_MONTH_JANUARY;
    sDate.Date = 1;
    sDate.Year = 0;

    /* Write the startup date to the internal RTC. */
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
    {
        return INTERNAL_RTC_Nok;
    }

    /* Configure PB7 / EXTI7 as an additional wake-up source. */
    INTERNAL_RTC_PB7_EXTI_Init();

    /* Internal RTC initialization completed successfully. */
    return INTERNAL_RTC_Ok;
}

/******************************************************************************
 * @brief  Enter sleep mode and configure RTC Alarm A as the wake-up time.
 *
 * @param  wake_hour   Target wake-up hour in 24-hour format.
 * @param  wake_minute Target wake-up minute.
 *
 * @details
 * This function is used when the application wants to sleep now and wake at a
 * specific time. It performs the following sequence:
 *
 * 1. Configure PB3 as a debug LED output.
 * 2. Read current time from the external RTC module.
 * 3. Copy the external RTC time into the STM32 internal RTC.
 * 4. Configure internal RTC Alarm A using wake_hour and wake_minute.
 * 5. Suspend FreeRTOS scheduler and SysTick before entering sleep.
 * 6. Clear old RTC, PWR, and EXTI wake-up flags.
 * 7. Disable selected UART interrupts before WFI.
 * 8. Enter sleep mode using WFI.
 * 9. Resume interrupts, SysTick, HAL tick, and FreeRTOS after wake-up.
 * 10. Toggle PB3 as a visible wake-up/debug indication.
 ******************************************************************************/
void INTERNAL_RTC_EnterSleepMode(uint8_t wake_hour, uint8_t wake_minute)
{
    /* Time read from the external RTC driver. */
    RTC_Time_t external_rtc_time;

    /* Time format required by the STM32 HAL internal RTC driver. */
    RTC_TimeTypeDef internal_rtc_time = {0};

    /* Alarm configuration structure for RTC Alarm A. */
    RTC_AlarmTypeDef alarm = {0};

    /* GPIO configuration structure used for PB3 debug LED. */
    GPIO_InitTypeDef GPIO_Init = {0};

    /* Enable GPIOB clock before configuring PB3. */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure PB3 as a push-pull output for wake-up/debug indication. */
    GPIO_Init.Pin = GPIO_PIN_3;
    GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Make sure PB3 starts LOW before entering sleep. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

    /* Read the current project time from the external RTC module. */
    RTC_GetTime(&external_rtc_time);

    /* Copy external RTC time fields into the HAL internal RTC time structure. */
    internal_rtc_time.Hours = external_rtc_time.hours;
    internal_rtc_time.Minutes = external_rtc_time.minutes;
    internal_rtc_time.Seconds = external_rtc_time.seconds;
    internal_rtc_time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    internal_rtc_time.StoreOperation = RTC_STOREOPERATION_RESET;

    /* Synchronize the STM32 internal RTC time with the external RTC time. */
    if (HAL_RTC_SetTime(&hrtc, &internal_rtc_time, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    /* Configure the alarm time requested by the application. */
    alarm.AlarmTime.Hours = wake_hour;
    alarm.AlarmTime.Minutes = wake_minute;
    alarm.AlarmTime.Seconds = 0;
    alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

    /* Ignore date/weekday so the alarm matches based on time only. */
    alarm.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;

    /* Ignore subsecond comparison for this alarm. */
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

    /* Select date mode for the alarm date/weekday field. */
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;

    /* Dummy date value because date is masked. */
    alarm.AlarmDateWeekDay = INTERNAL_RTC_WAKE_ALARM_DATE;

    /* Use RTC Alarm A as the wake-up alarm. */
    alarm.Alarm = RTC_ALARM_A;

    /* Suspend FreeRTOS scheduling before entering low-power mode. */
    vTaskSuspendAll();

    /* Suspend HAL tick to prevent periodic SysTick activity during sleep. */
    HAL_SuspendTick();

    /* Disable the SysTick interrupt bit directly before WFI. */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    /* Clear any old RTC Alarm A flag before setting a new alarm. */
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);

    /* Clear power wake-up flag from previous wake-up events. */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Clear RTC alarm EXTI line 17 pending bit. */
    EXTI->PR = (1U << 17);

    /* Configure and enable RTC Alarm A interrupt. */
    if (HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) != HAL_OK)
    {
        Error_Handler();
    }

    /* Clear any pending PB7 EXTI interrupt before sleeping. */
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);

    /* Clear pending NVIC state for EXTI lines 5 to 9. */
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

    /* Temporarily disable UART interrupts before sleep entry. */
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_DisableIRQ(USART3_IRQn);

    /* Enter sleep mode and wait for an interrupt event to wake the CPU. */
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* Re-enable UART interrupts after waking up. */
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART3_IRQn);

    /* Re-enable SysTick interrupt after wake-up. */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    /* Resume HAL tick after wake-up. */
    HAL_ResumeTick();

    /* Resume FreeRTOS scheduler after wake-up. */
    xTaskResumeAll();

    /* Blink PB3 ten times as a visible wake-up/debug indication. */
    for (uint8_t i = 0; i < 10; i++)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Make sure PB3 is OFF after the wake-up indication finishes. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

/******************************************************************************
 * @brief  Enter sleep mode without configuring a new RTC alarm.
 *
 * @details
 * This function is used when the application wants to sleep immediately while
 * relying on already configured wake-up sources.
 *
 * Wake-up sources may include:
 * - PB7 / EXTI7
 * - Any RTC alarm that was already configured earlier
 *
 * The function follows the same sleep-entry/sleep-exit protection sequence as
 * INTERNAL_RTC_EnterSleepMode(), but it does not set a new alarm time.
 ******************************************************************************/
void INTERNAL_RTC_EnterSleepModeOnly(void)
{
    /* GPIO configuration structure used for PB3 debug LED. */
    GPIO_InitTypeDef GPIO_Init = {0};

    /* Enable GPIOB clock before configuring PB3. */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Configure PB3 as a push-pull output for wake-up/debug indication. */
    GPIO_Init.Pin = GPIO_PIN_3;
    GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_Init);

    /* Make sure PB3 starts LOW before entering sleep. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);

    /* Suspend FreeRTOS scheduling before entering low-power mode. */
    vTaskSuspendAll();

    /* Suspend HAL tick to prevent periodic SysTick activity during sleep. */
    HAL_SuspendTick();

    /* Disable the SysTick interrupt bit directly before WFI. */
    SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk;

    /* Clear power wake-up flag from previous wake-up events. */
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

    /* Clear any pending PB7 EXTI interrupt before sleeping. */
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_7);

    /* Clear pending NVIC state for EXTI lines 5 to 9. */
    NVIC_ClearPendingIRQ(EXTI9_5_IRQn);

    /* Temporarily disable UART interrupts before sleep entry. */
    NVIC_DisableIRQ(USART2_IRQn);
    NVIC_DisableIRQ(USART3_IRQn);

    /* Enter sleep mode and wait for an interrupt event to wake the CPU. */
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* Re-enable UART interrupts after waking up. */
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_EnableIRQ(USART3_IRQn);

    /* Re-enable SysTick interrupt after wake-up. */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;

    /* Resume HAL tick after wake-up. */
    HAL_ResumeTick();

    /* Resume FreeRTOS scheduler after wake-up. */
    xTaskResumeAll();

    /* Blink PB3 ten times as a visible wake-up/debug indication. */
    for (uint8_t i = 0; i < 10; i++)
    {
        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_3);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    /* Make sure PB3 is OFF after the wake-up indication finishes. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
}

/******************************************************************************
 * @brief  Read and clear the PB7 / EXTI7 wake-up flag.
 *
 * @retval 1 PB7 / EXTI7 wake-up event was detected.
 * @retval 0 No PB7 / EXTI7 wake-up event was pending.
 *
 * @details
 * This function allows the application to know whether PB7 caused the wake-up.
 * The flag is cleared immediately after reading so that the next call reports
 * only new PB7 events.
 ******************************************************************************/
uint8_t INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag(void)
{
    /* Copy the interrupt-updated flag to a local variable. */
    uint8_t flag = INTERNAL_RTC_EXTI7_WakeupFlag;

    /* Clear the stored flag after reading it. */
    INTERNAL_RTC_EXTI7_WakeupFlag = 0;

    /* Return the previous flag value to the caller. */
    return flag;
}

/******************************************************************************
 * @brief  HAL GPIO external interrupt callback.
 *
 * @param  GPIO_Pin GPIO pin number that triggered the EXTI callback.
 *
 * @details
 * This function is called by the HAL EXTI interrupt handler. In this driver,
 * it is used to detect PB7 / EXTI7 wake-up activity.
 *
 * When PB7 triggers the callback, INTERNAL_RTC_EXTI7_WakeupFlag is set to 1.
 * The application can later read and clear this flag using
 * INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag().
 ******************************************************************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    /* Check whether the interrupt source is PB7. */
    if (GPIO_Pin == GPIO_PIN_7)
    {
        /* Store the PB7 wake-up event for the application layer. */
        INTERNAL_RTC_EXTI7_WakeupFlag = 1;
    }
}
