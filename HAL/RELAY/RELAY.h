/******************************************************************************
 * @file    RELAY.h
 * @author  Ahmed Abdelrhman
 * @brief   Public interface for the relay driver.
 *
 * @details
 * This header exposes the relay APIs used by the application layer and relay
 * FreeRTOS task. The driver supports direct ON/OFF control and non-blocking
 * timed relay control.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef RELAY_RELAY_H_
#define RELAY_RELAY_H_

#include <stdint.h>

/* Logical relay index used by the application for the first configured relay. */
#define RELAY_NUM_1    0

/******************************************************************************
 * @brief Relay driver return/error status values.
 *
 * @details
 * These values allow the caller to know whether a relay operation succeeded,
 * failed, or was rejected because of invalid arguments.
 ******************************************************************************/
typedef enum RELAY_Err_St_e
{
    /* Relay GPIO initialization completed successfully. */
    RELAY_Init_Success,

    /* Relay GPIO initialization failed or has not completed successfully. */
    RELAY_Init_Failed,

    /* Relay ON command completed successfully. */
    RELAY_On_Ok,

    /* Relay ON command failed. */
    RELAY_On_Nok,

    /* Relay OFF command completed successfully. */
    RELAY_Off_Ok,

    /* Relay OFF command failed. */
    RELAY_Off_Nok,

    /* The selected relay number is outside the configured range. */
    RElAY_Invalid_Args

} RELAY_Err_St_t;

/******************************************************************************
 * @brief  Initialize the selected relay GPIO pin.
 * @param  Relay_num Relay index from the relay configuration table.
 * @return Relay driver status.
 ******************************************************************************/
RELAY_Err_St_t RELAY_Init(uint8_t Relay_num);

/******************************************************************************
 * @brief  Turn ON the selected relay.
 * @param  Relay_num Relay index from the relay configuration table.
 * @return Relay driver status.
 ******************************************************************************/
RELAY_Err_St_t RELAY_On(uint8_t Relay_num);

/******************************************************************************
 * @brief  Turn OFF the selected relay.
 * @param  Relay_num Relay index from the relay configuration table.
 * @return Relay driver status.
 ******************************************************************************/
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num);

/******************************************************************************
 * @brief  Turn ON the selected relay for a specific time without blocking.
 * @param  Relay_num     Relay index from the relay configuration table.
 * @param  Relay_Time_ms Relay ON duration in milliseconds.
 * @return Relay driver status.
 ******************************************************************************/
RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num, uint32_t Relay_Time_ms);

/******************************************************************************
 * @brief Periodic relay timing manager used to turn OFF timed relays.
 ******************************************************************************/
void RELAY_Time_Manager_Cyclic(void);

#endif /* RELAY_RELAY_H_ */
