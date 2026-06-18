/******************************************************************************
 * @file    App.c
 * @author  Ahmed Abdelrhman
 * @brief   Application layer implementation for the Fingerprint Access Control
 *          System.
 *
 * @details This module coordinates the high-level system behavior:
 *          - GUI command reception and parsing
 *          - Fingerprint enrollment request handling
 *          - Fingerprint search result logging
 *          - RTC timestamp collection
 *          - Relay activation for authorized users
 *          - Sleep/wake status reporting to the GUI
 *
 * @note    Only comments, spacing, and layout were improved. The application
 *          logic is intentionally kept the same.
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>

#include "stm32f4xx.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include <stm32f4xx_hal_pwr.h>

#include "FreeRTOS.h"
#include "task.h"

#include <USART.h>
#include <RTC.h>
#include <FP.h>

#include "Sys.h"
#include "RELAY.h"
#include "INTERNAL_RTC.h"
#include "App_Cfg.h"
#include "App_Prv.h"
#include "App.h"


/*=============================================================================
 * Private Variables
 *============================================================================*/

/* Current state of the GUI RX frame parser. */
static App_Rx_St_t App_Rx_St = APP_Rx_Wait_Sof;

/* Storage for the last valid frame received from the GUI. */
static APP_RxFrame_t APP_RxFrame;



/*=============================================================================
 * Public Functions
 *============================================================================*/

/******************************************************************************
 * @brief  Initialize the application layer.
 *
 * @details Initializes all modules required by the application:
 *          - USART driver for GUI communication
 *          - RTC driver for date/time logging
 *          - Fingerprint driver for search/enrollment operations
 *          - Relay driver for door/access output control
 *
 * @return APP_Init_Success if all required modules initialize correctly.
 * @return APP_Init_Failed  if any required module fails to initialize.
 ******************************************************************************/
APP_Err_St_t APP_Init(void)
{
    /* Initialize USART used for STM32 <-> GUI communication. */
    USART_Err_St_t USART_Err_St = USART_Init(APP_USART_NUM_);

    if (USART_Err_St == USART_InitFailed)
    {
        return APP_Init_Failed;
    }

    /* Initialize RTC used for timestamping access events. */
    RTC_Err_St_t RTC_Err_St = RTC_Init();

    if (RTC_Err_St == RTC_Init_Failed)
    {
        return APP_Init_Failed;
    }

    /* Initialize fingerprint driver used for search and enrollment. */
    FP_Err_St_t FP_Err_St = FP_Init();

    if (FP_Err_St == FP_InitFailed)
    {
        return APP_Init_Failed;
    }

    /* Initialize relay output used when access is granted. */
    RELAY_Init(RELAY_NUM_1);

    return APP_Init_Success;
}


/******************************************************************************
 * @brief  Main cyclic function of the application layer.
 *
 * @details This function must be called periodically. It performs the main
 *          application-level tasks:
 *          1. Reads current RTC time and handles sleep-mode logic.
 *          2. Receives and parses GUI frames.
 *          3. Processes GUI enrollment requests.
 *          4. Sends access logs when fingerprint search results are available.
 *          5. Sends enrollment status/instruction updates to the GUI.
 *
 * @return None.
 ******************************************************************************/
void APP_Cyclic(void)
{
    uint8_t match_st;
    uint16_t user_id;
    uint16_t id_to_send;

    RTC_Time_t time;
    RTC_Date_t date;

    /* Previous enrollment instruction sent to GUI.
     * This prevents sending the same enrollment instruction continuously.
     */
    static FP_GetEnroll_Instruction_t prev_inst = FP_E_Inst_Idle;

    /* Read current time first because sleep handling depends on it. */
    RTC_GetTime(&time);

    /* Handle scheduled sleep mode and temporary EXTI wake-up mode. */
    APP_HandleSleepMode(&time);

    /* Receive and parse any available GUI bytes. */
    APP_Check_RxFrame();

    /* Process a complete and valid GUI frame, if one is available. */
    if (APP_Check_Response() == APP_Rx_Full_Packet_ok)
    {
        /* GUI requested fingerprint enrollment mode. */
        if (APP_RxFrame.cmd == APP_Enroll_Req)
        {
            FP_SetMode(FP_ENROLL_MODE);
        }
    }

    /*--------------------------------------------------------------------------
     * Fingerprint Search Mode
     *--------------------------------------------------------------------------
     * In search mode, the fingerprint driver looks for a matching user. When a
     * result is available, this layer builds an access-log frame and sends it
     * to the GUI.
     */
    if (FP_GetMode() == FP_SEARCH_MODE)
    {
        uint8_t user_payload[APP_LOG_ACCESS_DATA_LEN];

        if (FP_Get_User(&match_st, &user_id) == FP_GetUser_Ok)
        {
            /* Get timestamp for the access log. */
            RTC_GetDate(&date);
            RTC_GetTime(&time);

            /* Payload byte 0: access result. */
            user_payload[0] = ((match_st == FP_MATCH_ST) ? APP_USER_GRANTED : APP_USER_DENIED);

            /* Send real user ID only when access is granted. */
            id_to_send = ((match_st == FP_MATCH_ST) ? user_id : 0);

            /* Payload bytes 1-2: user ID. */
            user_payload[1] = (id_to_send >> 8);
            user_payload[2] = (id_to_send & 0xFF);

            /* Payload bytes 3-5: time. */
            user_payload[3] = time.hours;
            user_payload[4] = time.minutes;
            user_payload[5] = time.seconds;

            /* Payload bytes 6-9: date. */
            user_payload[6] = date.Day;
            user_payload[7] = date.month;
            user_payload[8] = date.year >> 8;
            user_payload[9] = date.year & 0xFF;

            /* Send complete access log frame to the GUI. */
            APP_Send_Cmd(APP_Log_Access, user_payload, APP_LOG_ACCESS_DATA_LEN);

            /* Activate relay for 5 seconds if fingerprint matched. */
            if (match_st == FP_MATCH_ST)
            {
                RELAY_On_With_Time(RELAY_NUM_1, 5000);
            }

            /* Reset previous enrollment instruction for the next enrollment cycle. */
            prev_inst = FP_E_Inst_Idle;
        }
    }

    /*--------------------------------------------------------------------------
     * Fingerprint Enrollment Mode
     *--------------------------------------------------------------------------
     * In enrollment mode, the fingerprint driver controls the low-level sensor
     * sequence. The APP layer only reads the current instruction/status and
     * forwards it to the GUI.
     */
    else if (FP_GetMode() == FP_ENROLL_MODE)
    {
        FP_GetEnroll_Instruction_t current_inst = FP_GetEnroll_Instruction();

        /* Send instruction/status only when it changes. */
        if (current_inst != prev_inst)
        {
            uint8_t enroll_success_payload[APP_ENROLL_SUCCESS_PAYLOAD_LEN];

            if (current_inst == FP_E_Inst_Success)
            {
                uint16_t curr_user_id = FP_Get_Curr_User_Id();

                /* Success payload contains instruction + new user ID. */
                enroll_success_payload[0] = current_inst;
                enroll_success_payload[1] = (curr_user_id >> 8);
                enroll_success_payload[2] = (curr_user_id & 0xFF);

                APP_Send_Cmd(APP_Enroll_St, enroll_success_payload, APP_ENROLL_SUCCESS_PAYLOAD_LEN);
            }
            else
            {
                /* Non-success enrollment messages do not include a valid user ID. */
                enroll_success_payload[0] = current_inst;
                enroll_success_payload[1] = 0;
                enroll_success_payload[2] = 0;

                APP_Send_Cmd(APP_Enroll_St, enroll_success_payload, APP_ENROLL_INST_LEN);
            }

            prev_inst = current_inst;
        }
    }
}


/******************************************************************************
 * @brief  Handle application sleep and wake-up behavior.
 *
 * @details This function manages two sleep/wake cases:
 *          1. Temporary wake-up using EXTI7.
 *          2. Scheduled RTC-based sleep mode.
 *
 *          It also sends sleep/awake status frames to the GUI so the PC display
 *          can reflect the current system state.
 *
 * @param  time Pointer to the current RTC time.
 *
 * @return None.
 ******************************************************************************/
void APP_HandleSleepMode(RTC_Time_t *time)
{
    static uint8_t exti_awake_mode = 0;
    static TickType_t exti_start_tick = 0;
    static uint8_t has_slept_today = 0;

    /* Check whether EXTI7 caused a temporary wake-up. */
    if (INTERNAL_RTC_GetAndClear_EXTI7WakeupFlag())
    {
        exti_awake_mode = 1;
        exti_start_tick = xTaskGetTickCount();

        APP_SendSleepStatus(APP_SYSTEM_AWAKE);
    }

    /* If the system was temporarily awakened by EXTI7, keep it awake for 10 s. */
    if (exti_awake_mode)
    {
        if ((xTaskGetTickCount() - exti_start_tick) >= pdMS_TO_TICKS(10UL * 1000UL))
        {
            exti_awake_mode = 0;

            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepModeOnly();
        }
    }

    /* Scheduled sleep window 1. */
    if (time->hours == 0 && time->minutes == 25)
    {
        if (has_slept_today == 0)
        {
            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepMode(0, 26);

            APP_SendSleepStatus(APP_SYSTEM_AWAKE);

            has_slept_today = 1;
        }
    }

    /* Scheduled sleep window 2. */
    else if (time->hours == 3 && time->minutes == 33)
    {
        if (has_slept_today == 0)
        {
            APP_SendSleepStatus(APP_SYSTEM_SLEEP);
            vTaskDelay(pdMS_TO_TICKS(100));

            INTERNAL_RTC_EnterSleepMode(3, 34);

            APP_SendSleepStatus(APP_SYSTEM_AWAKE);

            has_slept_today = 1;
        }
    }
    else
    {
        has_slept_today = 0;
    }
}


/******************************************************************************
 * @brief  Send current sleep/awake state to the GUI.
 *
 * @param  state APP_SYSTEM_AWAKE or APP_SYSTEM_SLEEP.
 *
 * @return None.
 ******************************************************************************/
void APP_SendSleepStatus(uint8_t state)
{
    uint8_t payload[1];

    payload[0] = state;

    APP_Send_Cmd(APP_Sleep_St, payload, 1);
}


/*=============================================================================
 * Private Functions
 *============================================================================*/

/******************************************************************************
 * @brief  Build and send one application protocol frame to the GUI.
 *
 * @details Frame format:
 *
 *          [SOF][CMD][LEN][DATA...][CHECKSUM]
 *
 *          Checksum rule:
 *          CHECKSUM = CMD ^ LEN ^ DATA[0] ^ DATA[1] ^ ... ^ DATA[n]
 *
 * @param  APP_CMD_Inst     Command ID to send.
 * @param  Data_payload     Pointer to payload bytes.
 * @param  Data_payload_len Number of payload bytes.
 *
 * @return Current implementation does not return a value.
 ******************************************************************************/
static APP_Err_St_t APP_Send_Cmd(APP_CMD_t APP_CMD_Inst, uint8_t *Data_payload, uint8_t Data_payload_len)
{
    uint8_t app_tx_buff[30];
    uint8_t indx = 0;
    uint8_t check_sum = 0;

    /* Build frame header. */
    app_tx_buff[0] = APP_SOF;
    app_tx_buff[1] = APP_CMD_Inst;
    app_tx_buff[2] = Data_payload_len;

    /* SOF is not included in checksum. */
    check_sum = app_tx_buff[1] ^ app_tx_buff[2];

    /* Copy payload and update checksum. */
    for (indx = 0; indx < Data_payload_len; indx++)
    {
        app_tx_buff[3 + indx] = Data_payload[indx];
        check_sum ^= Data_payload[indx];
    }

    /* Add checksum byte after payload. */
    app_tx_buff[3 + Data_payload_len] = check_sum;

    /* Send SOF + CMD + LEN + DATA + CHECKSUM. */
    for (uint8_t i = 0; i < 4 + Data_payload_len; i++)
    {
        USART_SendByte(APP_USART_NUM_, app_tx_buff[i]);
    }
}


/******************************************************************************
 * @brief  Receive and parse GUI frames using a byte-by-byte state machine.
 *
 * @details Expected frame format:
 *
 *          [SOF][CMD][LEN][DATA...][CHECKSUM]
 *
 *          The function is non-blocking. It reads all currently available UART
 *          bytes, updates the parser state, and marks the frame complete only
 *          when checksum verification succeeds.
 *
 * @return None.
 ******************************************************************************/
static void APP_Check_RxFrame(void)
{
    uint8_t rx_byte;

    /* Static because a full frame can arrive across multiple cyclic calls. */
    static uint8_t data_indx = 0;

    while (USART_ReceiveByte(APP_USART_NUM_, &rx_byte) == USART_Rx_Ok)
    {
        switch (App_Rx_St)
        {
        case APP_Rx_Wait_Sof:
        {
            /* Ignore bytes until the start-of-frame byte is detected. */
            if (rx_byte == APP_SOF)
            {
                App_Rx_St = APP_Rx_Get_Cmd;
            }
            break;
        }

        case APP_Rx_Get_Cmd:
        {
            /* Store command byte. */
            APP_RxFrame.cmd = rx_byte;
            App_Rx_St = APP_Rx_Get_Len;
            break;
        }

        case APP_Rx_Get_Len:
        {
            /* Store payload length byte. */
            APP_RxFrame.len = rx_byte;

            if (APP_RxFrame.len == 0)
            {
                App_Rx_St = APP_Rx_Get_Cs;
            }
            else
            {
                data_indx = 0;
                App_Rx_St = APP_Rx_Get_Data;
            }
            break;
        }

        case APP_Rx_Get_Data:
        {
            /* Store payload bytes until LEN bytes are received. */
            APP_RxFrame.data[data_indx] = rx_byte;
            data_indx++;

            if (data_indx >= APP_RxFrame.len)
            {
                App_Rx_St = APP_Rx_Get_Cs;
            }
            break;
        }

        case APP_Rx_Get_Cs:
        {
            uint8_t received_cs = rx_byte;
            uint8_t calc_cs = APP_RxFrame.cmd ^ APP_RxFrame.len;

            /* Calculate checksum over CMD, LEN, and payload. */
            for (uint8_t i = 0; i < APP_RxFrame.len; i++)
            {
                calc_cs ^= APP_RxFrame.data[i];
            }

            if (received_cs == calc_cs)
            {
                App_Rx_St = APP_Rx_Complete_Frame;
            }
            else
            {
                App_Rx_St = APP_Rx_Wait_Sof;
            }
            break;
        }

        case APP_Rx_Complete_Frame:
        {
            /* Keep the valid frame untouched until APP_Check_Response() consumes it. */
            break;
        }
        }
    }
}


/******************************************************************************
 * @brief  Check whether a complete valid GUI frame is ready.
 *
 * @return APP_Rx_Full_Packet_ok  if a valid frame is available.
 * @return APP_Rx_Full_Packet_Nok if no valid frame is available.
 ******************************************************************************/
static APP_Err_St_t APP_Check_Response(void)
{
    APP_Err_St_t APP_Err_St = APP_Rx_Full_Packet_Nok;

    if (App_Rx_St == APP_Rx_Complete_Frame)
    {
        APP_Err_St = APP_Rx_Full_Packet_ok;
        App_Rx_St = APP_Rx_Wait_Sof;
    }

    return APP_Err_St;
}
