/*
 * EEPROM.h
 *
 *  Created on: 7 May 2026
 *      Author: Ahmed
 */

#ifndef EEPROM_EEPROM_H_
#define EEPROM_EEPROM_H_



typedef enum EEPROM_Err_St_e{

	EEPROM_Init_Success,
	EEPROM_Init_Failed,

	EEPROM_Write_Success,
	EEPROM_Write_Failed,

	EEPROM_Read_Success,
	EEPROM_Read_Failed,

	EEPROM_Erase_Success,
	EEPROM_Erase_Failed,

	EEPROM_Invalid_Page_Num_Arg,
	EEPROM_Invalid_Mem_Addr_Arg,
	EEPROM_Invalid_Page_Len_Arg,
	EEPROM_Invalid_Data_Arg,
}EEPROM_Err_St_t;


EEPROM_Err_St_t EEPROM_Init(void);

EEPROM_Err_St_t EEPROM_Write_Byte(uint16_t EEPROM_Mem_Addr , uint8_t byte_val);
EEPROM_Err_St_t  EEPROM_Read_Byte(uint16_t EEPROM_Mem_Addr , uint8_t *read_val);

EEPROM_Err_St_t EEPROM_Write_Page(uint16_t Page_Num , uint8_t *data , uint8_t lenth);
EEPROM_Err_St_t EEPROM_Read_Page(uint16_t Page_Num , uint8_t *data , uint8_t lenth);


EEPROM_Err_St_t EEPROM_Erase_All(void);
EEPROM_Err_St_t EEPROM_Erase_Page(uint16_t Page_Num);
EEPROM_Err_St_t EEPROM_Erase_Byte(uint16_t EEPROM_Mem_Addr);

EEPROM_Err_St_t EEPROM_Write(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth);
EEPROM_Err_St_t EEPROM_Read(uint16_t EEPROM_Mem_Addr, uint8_t *data, uint8_t lenth);

#endif /* EEPROM_EEPROM_H_ */
