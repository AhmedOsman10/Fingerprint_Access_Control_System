#include "I2C_Cfg.h"

const I2C_Config_t I2C_Config[I2C_MAX_NUM] =
{
	/* I2C1 - not used currently */
	{
		.Instance   = I2C1,
		.SCL_Port   = GPIOB,
		.SCL_Pin    = GPIO_PIN_6,
		.SCL_AF     = GPIO_AF4_I2C1,
		.SDA_Port   = GPIOB,
		.SDA_Pin    = GPIO_PIN_7,
		.SDA_AF     = GPIO_AF4_I2C1,
		.ClockSpeed = 100000
	},

	/* I2C2 - not used currently */
	{
		.Instance   = I2C2,
		.SCL_Port   = GPIOB,
		.SCL_Pin    = GPIO_PIN_10,
		.SCL_AF     = GPIO_AF4_I2C2,
		.SDA_Port   = GPIOB,
		.SDA_Pin    = GPIO_PIN_11,
		.SDA_AF     = GPIO_AF4_I2C2,
		.ClockSpeed = 100000
	},

	/* I2C3 - used by RTC */
	{
		.Instance   = I2C3,
		.SCL_Port   = GPIOA,
		.SCL_Pin    = GPIO_PIN_8,
		.SCL_AF     = GPIO_AF4_I2C3,
		.SDA_Port   = GPIOC,
		.SDA_Pin    = GPIO_PIN_9,
		.SDA_AF     = GPIO_AF4_I2C3,
		.ClockSpeed = 100000
	}
};
