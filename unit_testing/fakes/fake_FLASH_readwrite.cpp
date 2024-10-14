

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

// Write data to flash_address directly. This method does not
// ensure any data integrity as data validation is performed.
void writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    if(init_flag) {
        std::fill_n(mem_sector, SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag = false;
    }

    if (size < SIZE_OF_MEM_ARRAY) {
        memcpy(&mem_sector, data, size);
    }
}

// Read from flash without checking data integrity if CRC module
// is not enabled in project.
void readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    if(init_flag) {
        std::fill_n(mem_sector, SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag = false;
    }

    if (size < SIZE_OF_MEM_ARRAY) {
        memcpy(data, &mem_sector, size);
    }
}


// If this check is not included the compiler will throw an
// error for projects where CRC module is not enabled
#ifdef HAL_CRC_MODULE_ENABLED
// Write data to FLASH_ADDR+indx including CRC.
void writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    writeToFlash(flash_address, data, size);
}


// Read from flash without checking data integrity if  CRC module
// is not enabled in project.
void readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    readFromFlash(flash_address, data, size);
}
#endif
