/*
 * RELAY_Prv.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 */

#ifndef RELAY_RELAY_PRV_H_
#define RELAY_RELAY_PRV_H_

typedef struct Relay_Control_Time_s{
	uint32_t start_time;
	uint32_t deadline_ticks;
	uint8_t  is_active;
}Relay_Control_Time_s;


Relay_Control_Time_s Relay_Control_Time[RELAY_MAX_NUM];


#endif /* RELAY_RELAY_PRV_H_ */
