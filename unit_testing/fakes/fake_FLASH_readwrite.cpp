

/*!
** @brief Fake interface for reading/writing to flash memory
** 
** @author Luke W
** @date   12/07/2024
*/

#include <cstdint>
#include <algorithm>
#include <cstring>

#include "fake_stm32xxxx_hal.h"
#include "FLASH_readwrite.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define SIZE_OF_MEM_ARRAY 256

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static bool init_flag = true;
static uint8_t mem_sector[SIZE_OF_MEM_ARRAY] = {0};

// Write data to FLASH_ADDR+indx directly. This method does not
// ensure any data integrity as data validation is performed.
void writeToFlash(uint32_t indx, uint32_t size, uint8_t *data)
{
    if(init_flag) {
        std::fill_n(mem_sector, SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag = false;
    }

    if((indx + size) < SIZE_OF_MEM_ARRAY) {
        memcpy(&mem_sector[indx], data, size);
    }
}

// Read from flash without checking data integrity if CRC module
// is not enabled in project.
void readFromFlash(uint32_t indx, uint32_t size, uint8_t *data)
{
    if(init_flag) {
        std::fill_n(mem_sector, SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag = false;
    }

    if((indx + size) < SIZE_OF_MEM_ARRAY) {
        memcpy(data, &mem_sector[indx], size);
    }
}


// If this check is not included the compiler will throw an
// error for projects where CRC module is not enabled
#ifdef HAL_CRC_MODULE_ENABLED
// Write data to FLASH_ADDR+indx including CRC.
void writeToFlashSafe(CRC_HandleTypeDef *hcrc, uint32_t indx, uint32_t size, uint8_t *data)
{
    writeToFlash(indx, size, data);
}


// Read from flash without checking data integrity if CRC module
// is not enabled in project.
void readFromFlashSafe(CRC_HandleTypeDef *hcrc, uint32_t indx, uint32_t size, uint8_t *data)
{
    readFromFlash(indx, size, data);
}
#endif
