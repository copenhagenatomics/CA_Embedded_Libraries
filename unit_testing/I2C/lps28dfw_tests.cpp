/*!
 * @file    lps28dfw_tests.cpp
 * @brief   Unit tests for lps28dfw pressure sensor driver
 * @date    13/02/2026
 * @author  Timothé Dodin
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cmath>

/* Fakes */
#include "fake_StmGpio.h"
#include "fake_stm32xxxx_hal.h"

/* Real supporting units */
#include "time32.c"

/* UUT */
#include "LPS28DFW.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class mockLPS28DFW : public stm32I2cTestDevice {
   public:
    mockLPS28DFW(I2C_TypeDef* Instance, uint16_t address) {
        bus   = I2C1;
        addr  = address << 1;
        txRet = HAL_OK;
        rxRet = HAL_OK;
        rxVal = 0;
    }

    HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size) { return txRet; }
    HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size) {
        *buf = rxVal;
        return rxRet;
    }

    HAL_StatusTypeDef txRet;
    HAL_StatusTypeDef rxRet;
    uint8_t rxVal;
};

class LPS28DFWTests : public ::testing::Test {
   protected:
    LPS28DFWTests() {}

    I2C_HandleTypeDef hi2c = {.Instance = I2C1};
    StmGpio intDrdy{};
    lps28dfw_t dev{};
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(LPS28DFWTests, testInitSuccess) {
    mockLPS28DFW mockI2C(hi2c.Instance, LPS28DFW_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error     = true;
    mockI2C.txRet = HAL_OK;
    EXPECT_EQ(lps28dfw_init(&dev, &hi2c, LPS28DFW_I2C_ADDR_0, &intDrdy), 0);
    EXPECT_EQ(dev.error, false);
}

TEST_F(LPS28DFWTests, testInitCommunicationError) {
    mockLPS28DFW mockI2C(hi2c.Instance, LPS28DFW_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error     = false;
    mockI2C.txRet = HAL_ERROR;
    EXPECT_EQ(lps28dfw_init(&dev, &hi2c, LPS28DFW_I2C_ADDR_0, &intDrdy), -1);
    EXPECT_EQ(dev.error, true);
}

TEST_F(LPS28DFWTests, testLoopSuccess) {
    stmGpioInit(&intDrdy, GPIOA, 1, STM_GPIO_INPUT);
    mockLPS28DFW mockI2C(hi2c.Instance, LPS28DFW_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);
    ASSERT_EQ(lps28dfw_init(&dev, &hi2c, LPS28DFW_I2C_ADDR_0, &intDrdy), 0);

    // No new measurement ready
    stmSetGpio(intDrdy, false);
    EXPECT_EQ(lps28dfw_loop(&dev), 0);
    EXPECT_EQ(dev.data.pressure, 10000.0);
    EXPECT_EQ(dev.data.temperature, 10000.0);

    // New measurement ready
    stmSetGpio(intDrdy, true);
    mockI2C.rxVal = 1;
    mockI2C.txRet = HAL_OK;
    mockI2C.rxRet = HAL_OK;
    stmSetGpio(intDrdy, true);
    EXPECT_EQ(lps28dfw_loop(&dev), 0);
    EXPECT_NEAR(dev.data.pressure, 0.032, 1e-3);
    EXPECT_NEAR(dev.data.temperature, 2.570, 1e-3);
}

TEST_F(LPS28DFWTests, testLoopCommunicationError) {
    stmGpioInit(&intDrdy, GPIOA, 1, STM_GPIO_INPUT);
    mockLPS28DFW mockI2C(hi2c.Instance, LPS28DFW_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);
    ASSERT_EQ(lps28dfw_init(&dev, &hi2c, LPS28DFW_I2C_ADDR_0, &intDrdy), 0);

    // New measurement ready but TX error
    dev.data.pressure    = 1.0;
    dev.data.temperature = 1.0;
    dev.error            = false;
    stmSetGpio(intDrdy, true);
    mockI2C.rxVal = 1;
    mockI2C.txRet = HAL_ERROR;
    mockI2C.rxRet = HAL_OK;
    stmSetGpio(intDrdy, true);
    EXPECT_EQ(lps28dfw_loop(&dev), -1);
    EXPECT_EQ(dev.data.pressure, 10000.0);
    EXPECT_EQ(dev.data.temperature, 10000.0);
    EXPECT_EQ(dev.error, true);

    // New measurement ready but RX error
    dev.data.pressure    = 1.0;
    dev.data.temperature = 1.0;
    dev.error            = false;
    stmSetGpio(intDrdy, true);
    mockI2C.rxVal = 1;
    mockI2C.txRet = HAL_OK;
    mockI2C.rxRet = HAL_ERROR;
    stmSetGpio(intDrdy, true);
    EXPECT_EQ(lps28dfw_loop(&dev), -1);
    EXPECT_EQ(dev.data.pressure, 10000.0);
    EXPECT_EQ(dev.data.temperature, 10000.0);
    EXPECT_EQ(dev.error, true);
}

TEST_F(LPS28DFWTests, testLoopTimeout) {
    stmGpioInit(&intDrdy, GPIOA, 1, STM_GPIO_INPUT);
    mockLPS28DFW mockI2C(hi2c.Instance, LPS28DFW_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);
    ASSERT_EQ(lps28dfw_init(&dev, &hi2c, LPS28DFW_I2C_ADDR_0, &intDrdy), 0);

    // Successful run
    forceTick(150);
    dev.data.pressure    = 1.0;
    dev.data.temperature = 1.0;
    dev.error            = false;
    dev.lastAdcTime      = 0;
    EXPECT_EQ(lps28dfw_loop(&dev), 0);
    EXPECT_EQ(dev.data.pressure, 1.0);
    EXPECT_EQ(dev.data.temperature, 1.0);
    EXPECT_EQ(dev.error, false);

    // Timeout
    forceTick(151);
    dev.data.pressure    = 1.0;
    dev.data.temperature = 1.0;
    dev.error            = false;
    EXPECT_EQ(lps28dfw_loop(&dev), -2);
    EXPECT_EQ(dev.data.pressure, 10000.0);
    EXPECT_EQ(dev.data.temperature, 10000.0);
    EXPECT_EQ(dev.error, true);
}
