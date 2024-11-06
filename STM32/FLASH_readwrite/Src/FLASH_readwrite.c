
/*
 *  Library:			Read and write FLASH memory on STM32F401xC
 *  Description:		Provides an interface to read and write FLASH memory.
*/

#if defined(STM32F401xC)
#include <FLASH_readwrite.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** PRIVATE FUNCTION PROTOTYPES
***************************************************************************************************/

static uint32_t getFlashSector(uint32_t address);
static int isWriteWithinSector(uint32_t address, uint32_t size);
static int isProgramMemory(uint32_t address, uint32_t size);
static int isAddressValid(uint32_t flash_address, uint32_t size, uint32_t *flash_sector);

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Gets the FLASH sector of an address.
** @return Returns the FLASH sector if valid and 10 if a non-valid address is supplied.
*/
static uint32_t getFlashSector(uint32_t address)
{
    uint32_t address_sectors_start[VALID_SECTORS] = {START_SECTOR_1, START_SECTOR_2, START_SECTOR_3,
                                                     START_SECTOR_4, START_SECTOR_5};

    uint32_t address_sectors_end[VALID_SECTORS] = {END_SECTOR_1, END_SECTOR_2, END_SECTOR_3,
                                                   END_SECTOR_4, END_SECTOR_5};                                         
    
    for (int i = 0; i < VALID_SECTORS; i++)
    {
        if (address >= address_sectors_start[i] &&
            address <= address_sectors_end[i])
        {
            return i+1;
        }
    }
    // This should never occur - return non-existing sector.
    return 10;
}

/*!
** @brief Checks that the data to be stored is contained within a single FLASH sector.
** @return Returns 0 if the data is contained within a single FLASH sector, otherwise 1.
*/
static int isWriteWithinSector(uint32_t address, uint32_t size)
{
    uint32_t sector_start = getFlashSector(address);
    uint32_t sector_end = getFlashSector(address + size);

    // Ensure that the start and end addresses belong to the same FLASH sector.
    if (sector_start != sector_end)
    {
        return 0;
    }
    return 1;
}

/*!
** @brief Checks whether the user is trying to write to a memory range containing program memory.
** @return Returns 1 if the memory range contains program memory, otherwise 0.
*/
static int isProgramMemory(uint32_t address, uint32_t size)
{
    // Start address is within program memory
    if (address >= PROGRAM_START_ADDR && address < PROGRAM_END_ADDR)
    {
        return 1;
    }

    // End address is within program memory
    if ((address + size) >= PROGRAM_START_ADDR && (address + size) < PROGRAM_END_ADDR)
    {
        return 1;
    }

    // Address range holds program memory
    if (address < PROGRAM_START_ADDR && (address + size) >= PROGRAM_END_ADDR)
    {
        return 1;
    }

    return 0;
}

/*!
** @brief Checks whether the address is valid for FLASH operations
** @return Returns 1 if the memory range is valid for FLASH operations, otherwise 0.
*/
static int isAddressValid(uint32_t flash_address, uint32_t size, uint32_t *flash_sector)
{
    // User is trying to write to an address containing program data
    if (isProgramMemory(flash_address, size)) { return 0; }

    *flash_sector = getFlashSector(flash_address);

    // User is trying to write to non-valid sector
    if (*flash_sector == 10) { return 0; }

    // User is trying to write to more than one sector
    if (!isWriteWithinSector(flash_address, size)){ return 0; }

    return 1;
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
** @brief Store data in FLASH memory.
** @return Returns 0 on success and -1 on error.
** @note Does not store a CRC so data integrity verification is not possible when reading the 
**       data at later stage.
*/
int writeToFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    uint32_t flash_sector;
    if (!isAddressValid(flash_address, size, &flash_sector)) { return -1; }

    // If !HAL_OK FLASH unlock failed and cannot be erased or written to
    if (HAL_FLASH_Unlock() != HAL_OK) { return -1; }

    __HAL_RCC_WWDG_CLK_DISABLE();
    // Erase the sector before write
    FLASH_Erase_Sector(flash_sector, FLASH_VOLTAGE_RANGE_3);
    
    // Store data
    int ret = 0;
    for(uint32_t i=0; i<size; i++)
    {
        // If the FLASH write operation fails we should still lock the FLASH and re-enable the watch dog. 
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address+i , data[i]) != HAL_OK) { ret = -1; }
    }

    HAL_FLASH_Lock();
    __HAL_RCC_WWDG_CLK_ENABLE();
    return ret;
}

/*!
** @brief Read data from FLASH memory at flash_adress.
** @return Returns 0 on success and -1 on error.
*/
int readFromFlash(uint32_t flash_address, uint8_t *data, uint32_t size)
{
    uint32_t flash_sector = getFlashSector(flash_address);
    // User is trying to read from a non-valid sector
    if (flash_sector == 10) { return -1; }

    memcpy(data, (void*) flash_address, size);
    return 0;
}


/* If this check is not included the compiler will throw an
** error for projects where CRC module is not enabled */
#ifdef HAL_CRC_MODULE_ENABLED

/*!
** @brief Compute the CRC of the input data.
** @return CRC value
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
** @return Returns 0 on success and -1 on error.
*/
int writeToFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{    
    uint32_t flash_sector;
    if (!isAddressValid(flash_address, size, &flash_sector)) { return -1; }

    // If !HAL_OK FLASH unlock failed and cannot be erased or written to
    if (HAL_FLASH_Unlock() != HAL_OK) { return -1; }

    __HAL_RCC_WWDG_CLK_DISABLE();
    // Erase the sector before write.
    FLASH_Erase_Sector(flash_sector, FLASH_VOLTAGE_RANGE_3);

    // Compute CRC of data to be saved.
    uint32_t crcVal = computeCRC(hcrc, data, size);

    // Store data
    int ret = 0;
    for(uint32_t i=0; i<size; i++)
    {
        // If the FLASH write operation fails we should still lock the FLASH and re-enable the watch dog.
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address+i , data[i]) != HAL_OK) { ret = -1; }
    }

    // Store CRC value
    if (ret == 0)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address+size , crcVal) != HAL_OK) { ret = -1; }
    }

    HAL_FLASH_Lock();
    __HAL_RCC_WWDG_CLK_ENABLE();
    return ret;
}


/*!
** @brief Read data from FLASH memory at flash_adress and verify data with stored CRC.
** @return Returns 0 on success and -1 on error. *data is unchanged on error.
*/
int readFromFlashCRC(CRC_HandleTypeDef *hcrc, uint32_t flash_address, uint8_t *data, uint32_t size)
{
    uint32_t flash_sector = getFlashSector(flash_address);
    // User is trying to read from a non-valid sector
    if (flash_sector == 10) { return -1; }

    // Create temporary storage for FLASH data
    uint8_t dataTmp[size];
    memcpy(&dataTmp, (void*) flash_address, size);

    // Compute CRC of data read.
    uint32_t crcVal = computeCRC(hcrc, dataTmp, size);

    // Retrieve CRC value stored in FLASH.
    uint32_t crcStored = 0;
    memcpy(&crcStored, (void*) (flash_address+size), sizeof(uint32_t));

    // If computed and stored CRC value does not match return without setting *data
    if (crcStored != crcVal)
        return -1;

    // Write to user buffer
    memcpy(data, &dataTmp, size);
    return 0;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
