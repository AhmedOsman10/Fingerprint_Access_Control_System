/******************************************************************************
 * @file    App_Prv.h
 * @author  Ahmed Abdelrhman
 * @brief   Private definitions for the Application Layer.
 *
 * @details This file contains internal APP-layer definitions used only by
 *          App.c, including protocol command IDs, RX parser states, frame
 *          structures, and internal payload lengths.
 *
 * @note    Only comments, spacing, and layout were improved. The application
 *          logic is intentionally kept the same.
 ******************************************************************************/

#ifndef APP_APP_PRV_H_
#define APP_APP_PRV_H_

#include <stdint.h>
#include <RTC.h>
#include "App.h"


/*=============================================================================
 * Private Protocol Macros
 *============================================================================*/

/* Start Of Frame byte for the GUI protocol. */
#define APP_SOF  0xAA

/* Payload length of the access log frame sent from STM32 to GUI. */
#define APP_LOG_ACCESS_DATA_LEN         10

/* Payload length used for enrollment instruction/status frames. */
#define APP_ENROLL_INST_LEN             3

/* Payload length used when enrollment succeeds and a user ID is included. */
#define APP_ENROLL_SUCCESS_PAYLOAD_LEN  3

/* System state values sent to the GUI. */
#define APP_SYSTEM_AWAKE                0x00
#define APP_SYSTEM_SLEEP                0x01


/*=============================================================================
 * Private Protocol Types
 *============================================================================*/

/******************************************************************************
 * @brief Command IDs used in the STM32 <-> GUI protocol.
 *
 * @details Frame command meanings:
 *          - APP_Enroll_Req: GUI requests fingerprint enrollment mode.
 *          - APP_Enroll_St : STM32 sends enrollment instruction/status.
 *          - APP_Log_Access: STM32 sends access result with timestamp.
 *          - APP_Sleep_St  : STM32 sends sleep/awake state to GUI.
 ******************************************************************************/
typedef enum APP_CMD_e
{
    APP_Enroll_Req = 0x10,
    APP_Enroll_St  = 0x11,
    APP_Log_Access = 0x12,
    APP_Sleep_St   = 0x13
} APP_CMD_t;


/******************************************************************************
 * @brief RX parser states for the GUI protocol frame.
 ******************************************************************************/
typedef enum App_Rx_St_e
{
    APP_Rx_Wait_Sof,        /* Waiting for start-of-frame byte. */
    APP_Rx_Get_Cmd,         /* Waiting for command byte. */
    APP_Rx_Get_Len,         /* Waiting for payload length byte. */
    APP_Rx_Get_Data,        /* Receiving payload bytes. */
    APP_Rx_Get_Cs,          /* Waiting for checksum byte. */
    APP_Rx_Complete_Frame,  /* Valid complete frame received. */
} App_Rx_St_t;


/******************************************************************************
 * @brief Structure used to store a received GUI frame.
 ******************************************************************************/
typedef struct APP_RxFrame_s
{
    uint8_t cmd;       /* Command ID. */
    uint8_t len;       /* Payload length. */
    uint8_t data[20];  /* Payload buffer. */
} APP_RxFrame_t;


/*=============================================================================
 * Private Function Prototypes
 *============================================================================*/


/******************************************************************************
 * @brief  Build and send one application protocol frame to the GUI.
 *
 * @param  APP_CMD_Inst     Command ID to send.
 * @param  Data_payload     Pointer to payload bytes.
 * @param  Data_payload_len Number of payload bytes.
 *
 * @return APP_Err_St_t function status.
 ******************************************************************************/
static APP_Err_St_t APP_Send_Cmd(APP_CMD_t APP_CMD_Inst, uint8_t *Data_payload, uint8_t Data_payload_len);

/******************************************************************************
 * @brief  Receive and parse GUI frames using the internal RX state machine.
 *
 * @return None.
 ******************************************************************************/
static void APP_Check_RxFrame(void);

/******************************************************************************
 * @brief  Check whether a complete valid GUI frame is ready.
 *
 * @return APP_Rx_Full_Packet_ok  if a complete frame is available.
 * @return APP_Rx_Full_Packet_Nok if no complete frame is available.
 ******************************************************************************/
static APP_Err_St_t APP_Check_Response(void);

/******************************************************************************
 * @brief  Handle application sleep/wake logic.
 *
 * @param  time Pointer to current RTC time.
 *
 * @return None.
 ******************************************************************************/
void APP_HandleSleepMode(RTC_Time_t *time);


#endif /* APP_APP_PRV_H_ */
