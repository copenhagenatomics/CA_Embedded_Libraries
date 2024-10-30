/*!
** @file   arraymath_tests.cpp
** @author Luke W
** @date   17/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

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
    uint32_t test_address[6] = {0x08000000, 0x08004040, 0x08008000, 0x0800CC00,
                                0x08010100, 0x08020000};

    for (int i = 0; i<6; i++)
    {
        uint32_t sector = getFlashSector(test_address[i]);
        EXPECT_EQ(sector, i);
    }
}
