/*
 * RELAY_Prv.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 *
 *  Private definitions for the Relay driver.
 *
 *  This file contains internal data structures used by RELAY.c.
 *  It is not intended for direct use by the application layer.
 */

#ifndef RELAY_RELAY_PRV_H_
#define RELAY_RELAY_PRV_H_

#include <stdint.h>

/******************************************************************************************
 *                              Relay_Control_Time_s
 *
 *  Internal timing control structure for timed relay operation.
 *
 *  Fields:
 *    start_time:      RTOS tick count when relay was turned ON.
 *    deadline_ticks:  Required ON duration converted to RTOS ticks.
 *    is_active:       Indicates whether timed relay control is active.
 *
 *  Internal Use:
 *    Used by RELAY_On_With_Time() and RELAY_Time_Manager_Cyclic().
 ******************************************************************************************/
typedef struct Relay_Control_Time_s
{
	uint32_t start_time;
	uint32_t deadline_ticks;
	uint8_t  is_active;
}Relay_Control_Time_s;


/******************************************************************************************
 *                              Relay_Control_Time[]
 *
 *  Internal timing table for all configured relays.
 *
 *  Each relay has one timing control entry.
 ******************************************************************************************/
Relay_Control_Time_s Relay_Control_Time[RELAY_MAX_NUM];

#endif /* RELAY_RELAY_PRV_H_ */
