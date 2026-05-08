#ifndef MCAL_I2C_I2C_PRV_H_
#define MCAL_I2C_I2C_PRV_H_

#include "I2C_Cfg.h"

#define I2C_TIMEOUT                 3000
#define I2C_READY_TRIALS            3

#define I2C_MEM_ADDR_SIZE_8BIT      I2C_MEMADD_SIZE_8BIT

static void I2C_EnableClock(uint8_t I2C_Num);
static void I2C_GPIO_Init(uint8_t I2C_Num);

#endif
