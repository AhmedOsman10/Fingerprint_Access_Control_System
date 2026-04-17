/*
 * FP.h
 *
 *  Created on: 2 Feb 2026
 *      Author: Ahmed
 */

#ifndef FP_FP_H_
#define FP_FP_H_


typedef enum FP_Err_St_e
{
	FP_InitSuccess,
	FP_InitFailed,

	FP_Rx_Full_Packet_Ok,
	FP_Rx_Full_Packet_Nok,

	FP_SendCmd_Success,
	FP_SendCmd_Failed,

	FP_GetUser_Ok,
	FP_GetUser_NOk,
}FP_Err_St_t;


typedef struct FP_Search_Data_s
{
	uint8_t match_st;
	uint16_t user_id;
}FP_Search_Data_t;


typedef enum FP_GetEnroll_Instruction_e{
	FP_E_Inst_Idle,
	FP_E_Inst_Place_Finger,
	FP_E_Inst_Lift_Finger,
	FP_E_Inst_Place_Finger_Again,
	FP_E_Inst_Processing,
	FP_E_Inst_Success,
	FP_E_Inst_Failed,
}FP_GetEnroll_Instruction_t;

typedef uint8_t FP_Mode_t;
#define FP_SEARCH_MODE  0
#define FP_ENROLL_MODE  1




#define FP_MATCH_ST      1
#define FP_NOT_MATCH_ST  0


FP_Err_St_t FP_Init(void);
void FP_SimpleTesT(void);
FP_Err_St_t FP_Get_User(uint8_t *match_st , uint16_t *user_id);
void FP_MainFunction_Cyclic(void);

void FP_SetMode(FP_Mode_t mode);
FP_Mode_t FP_GetMode(void);

FP_GetEnroll_Instruction_t FP_GetEnroll_Instruction(void);


#endif /* FP_FP_H_ */
