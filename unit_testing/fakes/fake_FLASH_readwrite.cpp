

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

static bool init_flag[VALID_SECTORS] = {true, true, true, true, true};
static uint8_t mem_sectors[VALID_SECTORS][SIZE_OF_MEM_ARRAY] = {0}; 
static uint32_t addresses[VALID_SECTORS] = {0}; 
static int num_addresses_used = 0;

// Get the flash_address index to be used in mem_sectors
// Assigns a "sector" in the mem_sectors array if not already initialised.
int getFakeSector(uint32_t flash_address)
{
    for (int i = 0; i < VALID_SECTORS; i++)
    {
        if (addresses[i] == flash_address)
        {
            return i;
        }
    }

    // Add the flash addresses to array of flash_address already indexed.
    addresses[num_addresses_used] = flash_address;
    return num_addresses_used++;
}

// Write data to flash_address directly. This method does not
// ensure any data integrity as data validation is performed.
int writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    uint32_t sector = getFakeSector(flash_address);
    if(init_flag[sector]) {
        std::fill_n(mem_sectors[sector], SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag[sector] = false;
    }

    if (size < SIZE_OF_MEM_ARRAY) {
        memcpy(mem_sectors[sector], data, size);
    }
    return 0;
}

// Read from flash without checking data integrity if CRC module
// is not enabled in project.
int readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    uint32_t sector = getFakeSector(flash_address);
    if(init_flag[sector]) {
        std::fill_n(mem_sectors[sector], SIZE_OF_MEM_ARRAY, 0xFF);
        init_flag[sector] = false;
        return 1;
    }

    if (size < SIZE_OF_MEM_ARRAY) {
        memcpy(data, mem_sectors[sector], size);
    }
    return 0;
}


// If this check is not included the compiler will throw an
// error for projects where CRC module is not enabled
#ifdef HAL_CRC_MODULE_ENABLED
// Write data to FLASH_ADDR+indx including CRC.
int writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    int ret = writeToFlash(flash_address, data, size);
    return ret;
}


// Read from flash without checking data integrity if  CRC module
// is not enabled in project.
int readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    int ret = readFromFlash(flash_address, data, size);
    return ret;
}
#endif
