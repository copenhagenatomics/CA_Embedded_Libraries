/*!
** @file   flash_tests.cpp
** @author Matias
** @date   31/10/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

extern "C" {
    uint32_t _ProgramMemoryStart = 0x08004000;
    uint32_t _ProgramMemoryEnd = 0x0800BFFF;
}

#define PROGRAM_START_ADDR  ((uintptr_t) 0x08004000)
#define PROGRAM_END_ADDR    ((uintptr_t) 0x0800BFFF)


/* Fakes */
#include "fake_stm32xxxx_hal.h"
/* Real supporting units */

/* UUT */
#include "FLASH_readwrite.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class FlashTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        FlashTest() {}
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(FlashTest, testGetSector)
{
    // Use test addresses on the border and inside of FLASH sectors
    // Sectors 1,2,3,4,5
    uint32_t test_addresses[5] = {0x08004040, 0x08008000, 0x0800CC00, 0x08010100, 0x08020000};

    uint32_t sector = 0;
    for (int i = 0; i<5; i++)
    {
        sector = getFlashSector(test_addresses[i]);
        EXPECT_EQ(sector, i+1);
    }

    // Test invalid address
    uint32_t test_address = 0x20000000;
    sector = getFlashSector(test_address);
    EXPECT_EQ(sector, 10);

    // Test sector that is outside STM32F401xC/D range
    test_address = 0x08040000;
    sector = getFlashSector(test_address);
    EXPECT_EQ(sector, 10);
}

TEST_F(FlashTest, testIsWriteWithinSector)
{
    // Use test addresses on the border and inside of FLASH sectors
    // Sectors 1,2,3,4,5 with sizes 16kB, 16kB, 16kB, 64kB, 128kB
    uint32_t test_addresses[5] = {START_SECTOR_1, START_SECTOR_2, START_SECTOR_3, START_SECTOR_4, START_SECTOR_5};
    
    // Write size of 8kB, 16kB, 32kB, 64kB, 128kB, 
    uint32_t writeSizes[5] = {0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF, 0x1FFFF};

    /* The return values are the expected values per comparison
    ** Explanation: START_SECTOR_1 has a size of 16kB. Hence, if a user tries to write
    **              anything below or equal to 16kB the function should return 0.
    **              Anything above it should return 1. This denotes the first row of 
    **              returnValues. 
    */
    int returnValues[25] = {0, 0, 1, 1, 1,
                            0, 0, 1, 1, 1,
                            0, 0, 1, 1, 1,
                            0, 0, 0, 0, 1,
                            0, 0, 0, 0, 0};
    int ret = 0;
    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 5; j++)
        {
            ret = isWriteWithinSector(test_addresses[i], writeSizes[j]);
            EXPECT_EQ(ret, returnValues[i*5+j]);
        }
    }
}

TEST_F(FlashTest, testIsProgramMemory)
{
    uint32_t legal_addresses[3] = {START_SECTOR_3, START_SECTOR_4, START_SECTOR_5};
    uint32_t writeSize = 0x3FFF; 

    int ret = 0;
    for (int i = 0; i < 3; i++)
    {
        ret = isProgramMemory(legal_addresses[i], writeSize);
        EXPECT_EQ(ret, 0);
    }

    // Test that when starting in program memory, but ending in user memory
    // the function throws an error
    uint32_t illegal_start_address = 0x08005000;
    uint32_t legal_end = 0x7FFF;
    ret = isProgramMemory(illegal_start_address, legal_end);
    EXPECT_EQ(ret, 1);

    // Test that when starting in user memory, but ending in program memory
    // the function throws an error 
    uint32_t legal_start_address = 0x08002000;
    uint32_t illegal_end = 0x3FFF;
    ret = isProgramMemory(legal_start_address, illegal_end);
    EXPECT_EQ(ret, 1);
}
