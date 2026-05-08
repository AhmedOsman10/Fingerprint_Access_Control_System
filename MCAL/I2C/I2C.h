#ifndef MCAL_I2C_I2C_H_
#define MCAL_I2C_I2C_H_

#include <stdint.h>
#include "I2C_Cfg.h"

typedef enum
{
	I2C_Init_Success,
	I2C_Init_Failed,
	I2C_Invalid_Arg,
	I2C_Device_Ready,
	I2C_Device_Not_Ready,
	I2C_Write_Success,
	I2C_Write_Failed,
	I2C_Read_Success,
	I2C_Read_Failed

}I2C_Err_St_t;

I2C_Err_St_t I2C_Init(uint8_t I2C_Num);

I2C_Err_St_t I2C_IsDeviceReady(uint8_t I2C_Num, uint16_t SlaveAddr);

I2C_Err_St_t I2C_MemWrite(uint8_t I2C_Num,
		                  uint16_t SlaveAddr,
		                  uint16_t MemAddr,
		                  uint8_t *Data,
		                  uint16_t Size);

I2C_Err_St_t I2C_MemRead(uint8_t I2C_Num,
		                 uint16_t SlaveAddr,
		                 uint16_t MemAddr,
		                 uint8_t *Data,
		                 uint16_t Size);

#endif
