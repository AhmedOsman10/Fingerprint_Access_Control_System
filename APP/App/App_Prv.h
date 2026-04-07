/*
 * App_Prv.h
 *
 *  Created on: 30 Mar 2026
 *      Author: Ahmed
 */

#ifndef APP_APP_PRV_H_
#define APP_APP_PRV_H_

#define APP_SOF   0xAA

/*==> Enroll_Req
==> ENROLL_ST
==> LOG_CMD
*/
/************************************ Log Access Frame ************************************************
 * SOF |CMD_INST | Len of the dat| Payload Data 											| Check Sum
 * === |======   |===============| ==============											|===========
 * 0xAA|0x12     | 9 			 | Acess state(denied or granted ) + user id + Time + Date	|
 *
 *******************************************************************************************************/
typedef enum APP_CMD_e{
	APP_Enroll_Req = 0x10, // PC(GUI) ===> STM  ==> Set Enroll mode
 	APP_Enroll_St  = 0x11, // STM ==> PC (GUI)  == Enroll mode status
	APP_Log_Access = 0x12 // STM ==> PC(GUI) user denied or grated with the time ==> Search mode

}APP_CMD_t;

#define APP_LOG_ACCESS_DATA_LEN  10

#endif /* APP_APP_PRV_H_ */
