/******************************************************************************
 * @file    RELAY_Prv.h
 * @author  Ahmed Abdelrhman
 * @brief   Private relay driver definitions.
 *
 * @details
 * This file contains private data types and internal variables used by the
 * relay timing manager. It is included by RELAY.c and is not intended to be
 * used directly by the application layer.
 *
 * @note
 * GitHub cleanup version: comments and formatting were improved only.
 * Application logic and behavior are intentionally unchanged.
 ******************************************************************************/

#ifndef RELAY_RELAY_PRV_H_
#define RELAY_RELAY_PRV_H_

#include <stdint.h>
#include "RELAY_Cfg.h"

/******************************************************************************
 * @brief Runtime timing information for one relay channel.
 *
 * @details
 * This structure is used only for non-blocking timed relay operation.
 * Each configured relay has one object of this type.
 ******************************************************************************/
typedef struct Relay_Control_Time_s
{
    /* FreeRTOS tick count captured when the relay was turned ON. */
    uint32_t start_time;

    /* Requested relay ON duration converted from milliseconds to RTOS ticks. */
    uint32_t deadline_ticks;

    /* True when the relay is currently managed by RELAY_Time_Manager_Cyclic(). */
    uint8_t is_active;

} Relay_Control_Time_s;

/* Runtime timing control table for all configured relays. */
Relay_Control_Time_s Relay_Control_Time[RELAY_MAX_NUM];

#endif /* RELAY_RELAY_PRV_H_ */
