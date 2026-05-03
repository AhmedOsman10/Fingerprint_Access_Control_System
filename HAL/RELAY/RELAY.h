/*
 * RELAY.h
 *
 *  Created on: 28 Apr 2026
 *      Author: Ahmed
 */

#ifndef RELAY_RELAY_H_
#define RELAY_RELAY_H_

#define RELAY_NUM_1   0

typedef enum  RELAY_Err_St_e{

	RELAY_Init_Success,
	RELAY_Init_Failed,

	RELAY_On_Ok,
	RELAY_On_Nok,

	RELAY_Off_Ok,
	RELAY_Off_Nok,

	RElAY_Invalid_Args

}RELAY_Err_St_t;


RELAY_Err_St_t RELAY_Init(uint8_t Relay_num);
RELAY_Err_St_t RELAY_On(uint8_t Relay_num);
RELAY_Err_St_t RELAY_Off(uint8_t Relay_num);

RELAY_Err_St_t RELAY_On_With_Time(uint8_t Relay_num , uint32_t Relay_Time_ms);

void RELAY_Time_Manager_Cyclic(void);

#endif /* RELAY_RELAY_H_ */
