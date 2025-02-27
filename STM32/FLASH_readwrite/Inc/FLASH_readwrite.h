/*
**  Library:			Read and write FLASH memory on STM32F401
**  Description:		Provides an interface to read and write FLASH memory.
*/

#ifndef INC_FLASH_READWRITE_H_
#define INC_FLASH_READWRITE_H_

#include <stdint.h>
#if !defined(__LIBRARY_TEST)
  #include "stm32f4xx_hal.h"
#endif

// NOTE: extern values must be defined in the .ld linker script
extern uint32_t _ProgramMemoryStart;   // Starting address of main program in FLASH
extern uint32_t _ProgramMemoryEnd;     // Ending address of main program in FLASH

/***************************************************************************************************
** DEFINES
***************************************************************************************************/
#ifndef PROGRAM_START_ADDR
  #define PROGRAM_START_ADDR ((uintptr_t) &_ProgramMemoryStart)
#endif

#ifndef PROGRAM_END_ADDR
  #define PROGRAM_END_ADDR ((uintptr_t) &_ProgramMemoryEnd)
#endif

#define VALID_SECTORS 5

/* FLASH sector memory layout for STM32F401xC
** NOTE: Sector 0 is not defined here, as it will always be occupied by the ISR 
**       and should never be overwritten by a user. */

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

/***************************************************************************************************
** PUBLIC FUNCTION PROTOTYPES
***************************************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

int writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size);
int readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size);

#ifdef HAL_CRC_MODULE_ENABLED
    int writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
    int readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_FLASH_READWRITE_H_ */
