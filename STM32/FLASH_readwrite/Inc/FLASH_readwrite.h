/*
**  Library:			Read and write FLASH memory on STM32F401
**  Description:		Provides an interface to read and write FLASH memory.
*/

#ifndef INC_FLASH_READWRITE_H_
#define INC_FLASH_READWRITE_H_

#include <stdint.h>
#if defined(HAL_CRC_MODULE_ENABLED) && defined(STM32F401xC) && !defined(__LIBRARY_TEST)
  #include "stm32f4xx_hal.h"
#endif

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// FLASH sector memory layout for STM32F401xC
#define START_SECTOR_0		0x08000000 // 16 kBytes
#define END_SECTOR_0		0x08003FFF	
#define START_SECTOR_1		0x08004000 // 16 kBytes
#define END_SECTOR_1		0x08007FFF
#define START_SECTOR_2		0x08008000 // 16 kBytes
#define END_SECTOR_2		0x0800BFFF
#define START_SECTOR_3		0x0800C000 // 16 kBytes
#define END_SECTOR_3		0x0800FFFF 
#define START_SECTOR_4		0x08010000 // 64 kBytes
#define END_SECTOR_4		0x0801FFFF
#define START_SECTOR_5		0x08020000 // 128 kBytes
#define END_SECTOR_5		0x0803FFFF
#define START_SECTOR_6		0x08040000 // 128 kBytes
#define END_SECTOR_6		0x0805FFFF
#define START_SECTOR_7		0x08060000 // 128 kBytes
#define END_SECTOR_7		0x0807FFFF

/***************************************************************************************************
** PUBLIC FUNCTION PROTOTYPES
***************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

void writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size);
void readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size);

//#ifdef HAL_CRC_MODULE_ENABLED
    void writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
    void readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
//#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_FLASH_READWRITE_H_ */
