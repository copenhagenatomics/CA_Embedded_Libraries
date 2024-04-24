/*!
** @file   goertzel_tests.cpp
** @author Matias
** @date   23/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */
#include "fake_stm32xxxx_hal.h"
#include "fake_StmGpio.h"

/* Real supporting units */

/* UUT */
#include "LTC2512-24.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class LTC2512Test: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        LTC2512Test() {
            ltc2512.hspia = hspia;
            ltc2512.hspib = hspib;

            // GPIO outputs
            stmGpioInit(&ltc2512.SYNC, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0004), STM_GPIO_OUTPUT);
            stmGpioInit(&ltc2512.RDLA, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0008), STM_GPIO_OUTPUT);
            stmGpioInit(&ltc2512.RDLB, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0012), STM_GPIO_OUTPUT);
            stmGpioInit(&ltc2512.SEL0, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0016), STM_GPIO_OUTPUT);
            stmGpioInit(&ltc2512.SEL1, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0020), STM_GPIO_OUTPUT);

            // GPIO inputs
            stmGpioInit(&ltc2512.BUSY, ((GPIO_TypeDef *) 0), ((uint16_t) 0x0024), STM_GPIO_INPUT);

            // Disable channels
            stmSetGpio(ltc2512.RDLA, true);
            stmSetGpio(ltc2512.RDLB, true);

            stmSetGpio(ltc2512.SYNC, false);
        }

    LTC2512Device ltc2512;
    SPI_HandleTypeDef *hspia;
    SPI_HandleTypeDef *hspib;
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(LTC2512Test, testDownsamplingFactor)
{   
    // Initialise the unit with a downsampling factor of 4
    LTC2512Init(&ltc2512, DF_4_SELECT);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL0),false);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL1),false);

    // Initialise the unit with a downsampling factor of 8
    LTC2512Init(&ltc2512, DF_8_SELECT);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL0),true);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL1),false);

    // Initialise the unit with a downsampling factor of 16
    LTC2512Init(&ltc2512, DF_16_SELECT);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL0),false);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL1),true);

    // Initialise the unit with a downsampling factor of 32
    LTC2512Init(&ltc2512, DF_32_SELECT);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL0),true);
    ASSERT_EQ(stmGetGpio(ltc2512.SEL1),true);

    // Initialise the unit with a downsampling factor of 32
    LTC2512Init(&ltc2512, DF_4_SELECT);
    ASSERT_NE(stmGetGpio(ltc2512.SEL0),true);
    ASSERT_NE(stmGetGpio(ltc2512.SEL1),true);
}

TEST_F(LTC2512Test, testTransform)
{   
    
}