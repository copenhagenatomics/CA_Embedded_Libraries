/*
 * FLASH_H7_interface.h
 *
 *  Created on: 22 Dec 2022
 *      Author: matias
 */

#ifndef INC_FLASH_H7_INTERFACE_H_
#define INC_FLASH_H7_INTERFACE_H_

#include <stdint.h>
#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t eraseSectors(uint32_t startAddress, uint16_t size);
uint32_t writeToFlashH7(uint32_t startAddress, uint32_t *data, uint16_t size);
void readFromFlashH7(uint32_t startAddress, uint32_t *data, uint16_t size);

/* Same interface as the STM32F4 implementation in FLASH_readwrite.h. size is in bytes */
#ifdef HAL_CRC_MODULE_ENABLED
	int writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
	int readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* INC_FLASH_H7_INTERFACE_H_ */
