/*!
 * @file    tci_tests.cpp
 * @brief   Unit tests for TCI H2 sensor driver
 * @date    23/04/2026
 * @author  Timothé Dodin
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>
#include <cmath>

/* Fakes */
#include "fake_stm32xxxx_hal.h"

/* Real supporting units */
#include "crc.c"
#include "time32.c"

/* UUT */
#include "TCI.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class mockTCI : public stm32I2cTestDevice {
   public:
    mockTCI(I2C_TypeDef* Instance) {
        addr  = I2C_ADDRESS << 1;
        bus   = I2C1;
        txRet = HAL_OK;
        rxRet = HAL_OK;
    }

    HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size) { return txRet; }
    HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size) {
        for (uint32_t i = 0; i < size; i++) {
            buf[i] = rxVal[i];
        }
        return rxRet;
    }

    HAL_StatusTypeDef txRet;
    HAL_StatusTypeDef rxRet;
    uint8_t rxVal[50];
};

class tci_tests : public ::testing::Test {
   protected:
    tci_tests() {}

    I2C_HandleTypeDef hi2c = {.Instance = I2C1};
    tci_t dev{};
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(tci_tests, testInitSuccess) {
    mockTCI mockI2C(hi2c.Instance);
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error     = true;
    mockI2C.txRet = HAL_OK;

    uint8_t id[4]                = {0x11, 0x22, 0x33, 0x44};
    uint8_t crc[2]               = {0x40, 0x94};
    uint8_t idMessageWithCrc[12] = {0x00, id[0], id[1], id[2], id[3],  0x00,
                                    0x00, 0x00,  0x00,  0x00,  crc[0], crc[1]};
    memcpy(mockI2C.rxVal, idMessageWithCrc, 12);

    EXPECT_EQ(tci_init(&dev, &hi2c), 0);
    EXPECT_EQ(dev.error, false);
}

TEST_F(tci_tests, testInitCommunicationError) {
    mockTCI mockI2C(hi2c.Instance);
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error     = false;
    mockI2C.txRet = HAL_ERROR;

    uint8_t id[4]                = {0x11, 0x22, 0x33, 0x44};
    uint8_t crc[2]               = {0x40, 0x94};
    uint8_t idMessageWithCrc[12] = {0x00, id[0], id[1], id[2], id[3],  0x00,
                                    0x00, 0x00,  0x00,  0x00,  crc[0], crc[1]};
    memcpy(mockI2C.rxVal, idMessageWithCrc, 12);

    EXPECT_EQ(tci_init(&dev, &hi2c), -1);
    EXPECT_EQ(dev.error, true);
}
