
/*
 *  Library:			Read and write FLASH memory on STM32F411
 *  Description:		Provides an interface to read and write FLASH memory.
*/
#if defined(STM32F401xC)
#include <FLASH_readwrite.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t getFlashSector(uint32_t address)
{
    uint32_t address_offset = address - START_SECTOR_0;
    uint32_t address_sectors_start[6] = {START_SECTOR_0, START_SECTOR_1, START_SECTOR_2, START_SECTOR_3,
                                         START_SECTOR_4, START_SECTOR_5};

    uint32_t address_sectors_end[6] = {END_SECTOR_0, END_SECTOR_1, END_SECTOR_2, END_SECTOR_3,
                                       END_SECTOR_4, END_SECTOR_5};                                         
    for (int i = 0; i < 6; i++)
    {
        if (address_offset >= address_sectors_start[i]-START_SECTOR_0 &&
            address_offset <= address_sectors_end[i]-START_SECTOR_0)
        {
            return i;
        }
    }
    // This should never occur - return non-existing sector.
    return 10;
}

/*!
** @brief Store data in FLASH memory.
** @note Does not store a CRC so data integrity verification is not possible when reading the 
**       data at later stage.
*/
void writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    __HAL_RCC_WWDG_CLK_DISABLE();
    uint32_t flash_sector = getFlashSector(flash_address);
    // Erase the sector before write
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(flash_sector, FLASH_VOLTAGE_RANGE_3);
    HAL_FLASH_Lock();

    // Write to sector
    HAL_FLASH_Unlock();

    for(uint32_t i=0; i<size; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address+i , data[i]);
    }
    HAL_FLASH_Lock();
    __HAL_RCC_WWDG_CLK_ENABLE();
}

/*!
** @brief Read data from FLASH memory at flash_adress.
*/
void readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    for(uint32_t i=0; i<size; i++)
    {
        *(data + i) = *( (uint8_t *)(flash_address+i) );
    }
}


/* If this check is not included the compiler will throw an
** error for projects where CRC module is not enabled */
#ifdef HAL_CRC_MODULE_ENABLED

/*!
** @brief Compute the CRC of the input data.
*/
uint32_t computeCRC(CRC_HandleTypeDef *hcrc, uint8_t *data, uint32_t size)
{
    // Convert data to uint32_t as needed for the CRC computation
    uint32_t crcData[size/sizeof(uint32_t)];
    memcpy(&crcData, data, size);

    // Compute CRC of stored data
    uint32_t crcVal = HAL_CRC_Calculate(hcrc, crcData, sizeof(crcData)/sizeof(uint32_t));
    return crcVal;
}

/*!
** @brief Store data + computed CRC of the data in FLASH memory.
*/
void writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    __HAL_RCC_WWDG_CLK_DISABLE();
    uint32_t flash_sector = getFlashSector(flash_address);

    // Erase the sector before write
    HAL_FLASH_Unlock();
    FLASH_Erase_Sector(flash_sector, FLASH_VOLTAGE_RANGE_3);
    HAL_FLASH_Lock();
    __HAL_RCC_WWDG_CLK_ENABLE();

    // Compute CRC of data to be saved.
    uint32_t crcVal = computeCRC(hcrc, data, size);

    // Append CRC converted to uint8_t to data
    uint8_t crcData[4] = {0};
    for (uint8_t i = 0; i < 4; i++)
    {
        uint8_t convertedCrcTemp = (crcVal >> 8*i) & 0xFF;
        //*(data + size + i) = convertedCrcTemp;
        crcData[i] = convertedCrcTemp;
    }

    __HAL_RCC_WWDG_CLK_DISABLE();
    // Write to sector
    HAL_FLASH_Unlock();
    // Save data supplied as input
    for(uint32_t i=0; i<size; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address+i , data[i]);
    }

    // Save CRC value
    for(uint32_t i=0; i<4; i++)
    {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address+size+i , crcData[i]);
    }
    HAL_FLASH_Lock();
    __HAL_RCC_WWDG_CLK_ENABLE();
}


/*!
** @brief Read data from FLASH memory at flash_adress and verify data with stored CRC.
*/
void readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    // Read data including CRC value
    for(uint32_t i=0; i<size; i++)
    {
        *(data + i) = *( (uint8_t *)(flash_address+i) );
    }

    // Compute CRC of data read.
    uint32_t crcVal = computeCRC(hcrc, data, size);

    // Retrieve stored CRC value in flash.
    uint32_t crcStored = 0;
    memcpy(&crcStored, (uint8_t *)(flash_address+size), sizeof(uint32_t));

    // If computed and stored CRC value does not match
    // set data[0]=0xFF which resembles a clean FLASH i.e.
    // default values are loaded and stored in boards.
    if (crcStored != crcVal)
        *data = 0xFF;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
