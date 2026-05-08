#ifndef MCAL_I2C_I2C_CFG_H_
#define MCAL_I2C_I2C_CFG_H_

#include "stm32f4xx_hal.h"

#define I2C_MAX_NUM     3

#define I2C_NUM_1       0
#define I2C_NUM_2       1
#define I2C_NUM_3       2

typedef struct
{
	I2C_TypeDef *Instance;

	GPIO_TypeDef *SCL_Port;
	uint16_t      SCL_Pin;
	uint32_t      SCL_AF;

	GPIO_TypeDef *SDA_Port;
	uint16_t      SDA_Pin;
	uint32_t      SDA_AF;

	uint32_t ClockSpeed;

}I2C_Config_t;

extern const I2C_Config_t I2C_Config[I2C_MAX_NUM];

#endif
