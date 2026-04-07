/*
 * TASKS.h
 *
 *  Created on: Dec 19, 2025
 *      Author: Ahmed
 */

#ifndef TASKS_H_
#define TASKS_H_

void TASKS_Init(void);
void TASKS_USART_30ms(void *pram);
void TASKS_Print_Usart_Rx_Msg(void *pram);
void TASKS_5ms(void *pram);
void TASKS_1s(void *pram);

void TASKS_Send_Data(void *pram);
void TASKS_USART_Tx_Cyclic(void *pram);
void TASKS_RTC_Update(void *pram);
void FP_Test(void *pram);
void FP_Main_Cyclic(void *pram);
void FP_Simple_Search_Task(void *argument);
void FP_System_Task(void *argument);
#endif /* TASKS_H_ */

