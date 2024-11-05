/*
 * HAL_H7_otp.c
 *
 *  Created on: 28 Dec 2022
 *      Author: matias
 */

// Only include content of file if project targets an STM32H7
#if defined(STM32H753xx)

#include <string.h>
#include <stdbool.h>

#include "HAL_otp.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx.h"
#include "FLASH_H7_interface.h"

// Section 4 of Memory Bank 1 is write protected by default and is therefore dedicated
// to OTP memory. This part of the memory is therefore not allowed to write to during
// normal FLASH storing.
#define FLASH_OTP_BASE		0x08080000
#define OB_WRP_SECTOR       OB_WRP_SECTOR_4
#define FLASH_WORD_SIZE		32 // 32 bytes


// Check whether OTP data is already programmed on board.
static int isOTPAvailable(FLASH_OBProgramInitTypeDef *OB)
{
    if (OB->WRPSector & OB_WRP_SECTOR)
        return 0;

    // OTP sector is write-protected
    return 1;
}

// Remove existing OTP data 
int removeOTP(FLASH_OBProgramInitTypeDef *OB, const BoardInfo *boardInfo)
{
    if (HAL_FLASH_Unlock() != HAL_OK)
        return OTP_WRITE_FAIL;

    // Unlock Flash options bytes register that allows removal of write protection of FLASH section
    if (HAL_FLASH_OB_Unlock() != HAL_OK)
        return OTP_WRITE_FAIL;

    // Disable write-protection for OB_WRP_SECTOR in FLASH_BANK_1
    OB->OptionType = OPTIONBYTE_WRP;
    OB->WRPState = OB_WRPSTATE_DISABLE;
    OB->Banks = FLASH_BANK_1;
    OB->WRPSector = OB_WRP_SECTOR;

    // Write and update new settings to the FLASH memory option bytes
    HAL_FLASHEx_OBProgram(OB);
    HAL_FLASH_OB_Launch();

    if (eraseSectors(FLASH_OTP_BASE, sizeof(boardInfo->data)/sizeof(uint32_t)) != 0)
            return OTP_WRITE_FAIL;

    return OTP_SUCCESS;
}

// Read the current content of the OTP data.
// @param boardInfo pointer to struct BoardInfo.
// Return 0 on success else OTP_EMPTY and boardInfo will be unchanged.
int HAL_otpRead(BoardInfo *boardInfo)
{
    static FLASH_OBProgramInitTypeDef OB;

    /* Fetch all OB type configuration (we need WRP sectors)
     * This step is to check if write protection is already set
     */
    HAL_FLASHEx_OBGetConfig(&OB);
    if (!isOTPAvailable(&OB))
        return OTP_EMPTY; // Nothing has been written to OTP area.

    // Valid section found. Copy data to pointer since user should NOT get direct access to OTP area.
    void* otpArea = (void*) FLASH_OTP_BASE;
    memcpy(boardInfo->data, otpArea, sizeof(boardInfo->data)/sizeof(uint32_t));

    return OTP_SUCCESS;
}

// Write BoardInfo to OTP flash memory.
// @param boardInfo pointer to struct BoardInfo.
// Return 0 on success else OTP_WRITE_FAIL if all OTP sections is written.
const int HAL_otpWrite(const BoardInfo *boardInfo)
{
    static FLASH_OBProgramInitTypeDef OB;

    /* Fetch all OB type configuration (we need WRP sectors)
     * This step is to check if write protection is already set
     */
    HAL_FLASHEx_OBGetConfig(&OB);

    // Check the version of the Board info
    if (boardInfo->otpVersion > OTP_VERSION || boardInfo->otpVersion == 0) {
        return OTP_WRITE_FAIL;
    }

    // If sector has already been written to erase first.
    if (isOTPAvailable(&OB))
    {
        removeOTP(&OB, boardInfo);
    }

    if (HAL_FLASH_Unlock() != HAL_OK)
        return OTP_WRITE_FAIL;

    // Unlock Flash options bytes register that allows removal of write protection of FLASH section
    if (HAL_FLASH_OB_Unlock() != HAL_OK)
        return OTP_WRITE_FAIL;

    // Write board info to sector using word size.
    uint32_t otpStartAddress = FLASH_OTP_BASE;
    for (int i=0; i<sizeof(boardInfo->data)/sizeof(uint32_t); i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, otpStartAddress, (uint32_t) &boardInfo->data[i]) != HAL_OK)
        {
            return OTP_WRITE_FAIL;
        }
        otpStartAddress += FLASH_WORD_SIZE;
    }

    // Enable write-protection for OB_WRP_SECTOR in FLASH_BANK_1
    OB.OptionType = OPTIONBYTE_WRP;
    OB.WRPState = OB_WRPSTATE_ENABLE;
    OB.Banks = FLASH_BANK_1;
    OB.WRPSector = OB_WRP_SECTOR;

    // Write and update new settings to the FLASH memory option bytes
    HAL_FLASHEx_OBProgram(&OB);
    HAL_FLASH_OB_Launch();

    if (HAL_FLASH_OB_Lock() != HAL_OK)
        return OTP_WRITE_FAIL;

    HAL_FLASH_Lock();
    return OTP_SUCCESS;
}
#endif