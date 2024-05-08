/*!
** @file    stub_FLASH_readwrite.cpp
** @author  Matias
** @date    03/05/2024
**/

#include "fake_stm32xxxx_hal.h"

void writeToFlash(uint32_t indx, uint32_t size, uint8_t *data)
{

}

// Read from flash without checking data integrity if CRC module
// is not enabled in project.
void readFromFlash(uint32_t indx, uint32_t size, uint8_t *data)
{

}


// Write data to FLASH_ADDR+indx including CRC.
void writeToFlashSafe(CRC_HandleTypeDef *hcrc, uint32_t indx, uint32_t size, uint8_t *data)
{

}


void readFromFlashSafe(CRC_HandleTypeDef *hcrc, uint32_t indx, uint32_t size, uint8_t *data)
{
    *data = 0xFF;
}