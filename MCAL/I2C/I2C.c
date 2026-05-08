

#include "I2C.h"
#include "I2C_Prv.h"

static I2C_HandleTypeDef I2C_Handle[I2C_MAX_NUM];


/******************************************************************************************
 *                                  I2C_Init()
 *
 *  Initialize selected I2C peripheral.
 *
 *  Description:
 *    - Enables GPIO and I2C peripheral clocks.
 *    - Configures SCL and SDA pins.
 *    - Configures HAL I2C handle.
 *
 ******************************************************************************************/
I2C_Err_St_t I2C_Init(uint8_t I2C_Num)
{
	if(I2C_Num >= I2C_MAX_NUM)
	{
		return I2C_Invalid_Arg;
	}

	I2C_EnableClock(I2C_Num);
	I2C_GPIO_Init(I2C_Num);

	I2C_Handle[I2C_Num].Instance = I2C_Config[I2C_Num].Instance;

	I2C_Handle[I2C_Num].Init.ClockSpeed       = I2C_Config[I2C_Num].ClockSpeed;
	I2C_Handle[I2C_Num].Init.DutyCycle        = I2C_DUTYCYCLE_2;
	I2C_Handle[I2C_Num].Init.OwnAddress1      = 0;
	I2C_Handle[I2C_Num].Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
	I2C_Handle[I2C_Num].Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
	I2C_Handle[I2C_Num].Init.OwnAddress2      = 0;
	I2C_Handle[I2C_Num].Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
	I2C_Handle[I2C_Num].Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;

	if(HAL_I2C_Init(&I2C_Handle[I2C_Num]) != HAL_OK)
	{
		return I2C_Init_Failed;
	}

	return I2C_Init_Success;
}


/******************************************************************************************
 *                                  I2C_IsDeviceReady()
 ******************************************************************************************/
I2C_Err_St_t I2C_IsDeviceReady(uint8_t I2C_Num, uint16_t SlaveAddr)
{
	if(I2C_Num >= I2C_MAX_NUM)
	{
		return I2C_Invalid_Arg;
	}

	if(HAL_I2C_IsDeviceReady(&I2C_Handle[I2C_Num],
			                 SlaveAddr,
			                 I2C_READY_TRIALS,
			                 I2C_TIMEOUT) != HAL_OK)
	{
		return I2C_Device_Not_Ready;
	}

	return I2C_Device_Ready;
}


/******************************************************************************************
 *                                  I2C_MemWrite()
 ******************************************************************************************/
I2C_Err_St_t I2C_MemWrite(uint8_t I2C_Num,
		                  uint16_t SlaveAddr,
		                  uint16_t MemAddr,
		                  uint8_t *Data,
		                  uint16_t Size)
{
	if((I2C_Num >= I2C_MAX_NUM) || (Data == NULL) || (Size == 0))
	{
		return I2C_Invalid_Arg;
	}

	if(HAL_I2C_Mem_Write(&I2C_Handle[I2C_Num],
			             SlaveAddr,
			             MemAddr,
			             I2C_MEM_ADDR_SIZE_8BIT,
			             Data,
			             Size,
			             I2C_TIMEOUT) != HAL_OK)
	{
		return I2C_Write_Failed;
	}

	return I2C_Write_Success;
}


/******************************************************************************************
 *                                  I2C_MemRead()
 ******************************************************************************************/
I2C_Err_St_t I2C_MemRead(uint8_t I2C_Num,
		                 uint16_t SlaveAddr,
		                 uint16_t MemAddr,
		                 uint8_t *Data,
		                 uint16_t Size)
{
	if((I2C_Num >= I2C_MAX_NUM) || (Data == NULL) || (Size == 0))
	{
		return I2C_Invalid_Arg;
	}

	if(HAL_I2C_Mem_Read(&I2C_Handle[I2C_Num],
			            SlaveAddr,
			            MemAddr,
			            I2C_MEM_ADDR_SIZE_8BIT,
			            Data,
			            Size,
			            I2C_TIMEOUT) != HAL_OK)
	{
		return I2C_Read_Failed;
	}

	return I2C_Read_Success;
}


/******************************************************************************************
 *                                  I2C_EnableClock()
 ******************************************************************************************/
static void I2C_EnableClock(uint8_t I2C_Num)
{
	if(I2C_Config[I2C_Num].SCL_Port == GPIOA || I2C_Config[I2C_Num].SDA_Port == GPIOA)
	{
		__HAL_RCC_GPIOA_CLK_ENABLE();
	}

	if(I2C_Config[I2C_Num].SCL_Port == GPIOB || I2C_Config[I2C_Num].SDA_Port == GPIOB)
	{
		__HAL_RCC_GPIOB_CLK_ENABLE();
	}

	if(I2C_Config[I2C_Num].SCL_Port == GPIOC || I2C_Config[I2C_Num].SDA_Port == GPIOC)
	{
		__HAL_RCC_GPIOC_CLK_ENABLE();
	}

	if(I2C_Config[I2C_Num].Instance == I2C1)
	{
		__HAL_RCC_I2C1_CLK_ENABLE();
	}
	else if(I2C_Config[I2C_Num].Instance == I2C2)
	{
		__HAL_RCC_I2C2_CLK_ENABLE();
	}
	else if(I2C_Config[I2C_Num].Instance == I2C3)
	{
		__HAL_RCC_I2C3_CLK_ENABLE();
	}
}


/******************************************************************************************
 *                                  I2C_GPIO_Init()
 ******************************************************************************************/
static void I2C_GPIO_Init(uint8_t I2C_Num)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* Configure SCL pin */
	GPIO_InitStruct.Pin       = I2C_Config[I2C_Num].SCL_Pin;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = I2C_Config[I2C_Num].SCL_AF;

	HAL_GPIO_Init(I2C_Config[I2C_Num].SCL_Port, &GPIO_InitStruct);

	/* Configure SDA pin */
	GPIO_InitStruct.Pin       = I2C_Config[I2C_Num].SDA_Pin;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = I2C_Config[I2C_Num].SDA_AF;

	HAL_GPIO_Init(I2C_Config[I2C_Num].SDA_Port, &GPIO_InitStruct);
}
