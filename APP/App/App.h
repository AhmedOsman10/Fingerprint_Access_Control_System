/*
 * App.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */

#ifndef APP_APP_H_
#define APP_APP_H_


#define APP_USER_DENIED  0x00
#define APP_USER_GRANTED 0x01
typedef enum APP_Err_St_e
{
	APP_Init_Success,
	APP_Init_Failed,

	APP_Rx_Full_Packet_Nok,
	APP_Rx_Full_Packet_ok,
}APP_Err_St_t;



APP_Err_St_t APP_Init(void );
void APP_Cylic(void);


#endif /* APP_APP_H_ */
