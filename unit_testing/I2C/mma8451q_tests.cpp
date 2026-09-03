/*!
 * @file    mma8451q_tests.cpp
 * @brief   Unit tests for MMA8451Q accelerometer driver
 * @date    03/09/2026
 * @author  Luke Walker
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstring>

/* Fakes */
#include "fake_stm32xxxx_hal.h"

/* UUT */
#include "MMA8451Q.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class mockMMA8451Q : public stm32I2cTestDevice {
   public:
    mockMMA8451Q(uint16_t address) {
        bus   = I2C1;
        addr  = address << 1;
        txRet = HAL_OK;
        rxRet = HAL_OK;
        lastReg = 0;
        memset(regs, 0, sizeof(regs));
        regs[0x0D] = 0x1A;  // WHO_AM_I
    }

    HAL_StatusTypeDef transmit(uint8_t *buf, uint8_t size) {
        if (size >= 1) {
            lastReg = buf[0];
        }
        // A 2-byte transmit is a register write. Optionally fail one specific write (1-indexed) to
        // simulate a specific step of config_registers() failing on the bus.
        if (size == 2) {
            writeCount++;
            if (writeCount == failOnWrite) {
                return HAL_ERROR;
            }
        }
        return txRet;
    }

    HAL_StatusTypeDef recv(uint8_t *buf, uint8_t size) {
        for (int i = 0; i < size; i++) {
            buf[i] = regs[(uint8_t)(lastReg + i)];
        }
        return rxRet;
    }

    // Writes a 14-bit raw acceleration value into the given axis' MSB/LSB registers
    void setAxis(uint8_t msbReg, int16_t raw14) {
        uint16_t leftJustified = ((uint16_t)raw14) << 2;
        regs[msbReg]     = (leftJustified >> 8) & 0xFF;
        regs[msbReg + 1] = leftJustified & 0xFF;
    }

    HAL_StatusTypeDef txRet;
    HAL_StatusTypeDef rxRet;
    uint8_t lastReg;
    uint8_t regs[256];
    int writeCount  = 0;
    int failOnWrite = -1;  // 1-indexed register write to fail on; -1 means never
};

class MMA8451QTests : public ::testing::Test {
   protected:
    MMA8451QTests() {}

    I2C_HandleTypeDef hi2c = {.Instance = I2C1};
    mma8451q_t dev{};
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(MMA8451QTests, testInitSuccess) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error = true;
    EXPECT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), 0);
    EXPECT_EQ(dev.error, false);
}

TEST_F(MMA8451QTests, testInitWrongWhoAmI) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    mockI2C.regs[0x0D] = 0x00;
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error = false;
    EXPECT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), -11);
    EXPECT_EQ(dev.error, true);
}

TEST_F(MMA8451QTests, testInitCommunicationError) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    mockI2C.txRet = HAL_ERROR;
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error = false;
    EXPECT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), -10);
    EXPECT_EQ(dev.error, true);
}

TEST_F(MMA8451QTests, testInitActivateWriteFailure) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    // config_registers() makes 4 writes in order: standby, XYZ_DATA_CFG, CTRL_REG2, activate.
    // Fail the 4th (the final write that puts the device in active mode).
    mockI2C.failOnWrite = 4;
    fakeHAL_I2C_addDevice(&mockI2C);

    dev.error = false;
    EXPECT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), -4);
    EXPECT_EQ(dev.error, true);
}

TEST_F(MMA8451QTests, testLoopSuccess) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);
    ASSERT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), 0);

    // 1g, -1g and 0.5g on X, Y and Z respectively
    mockI2C.setAxis(0x01, 4096);
    mockI2C.setAxis(0x03, -4096);
    mockI2C.setAxis(0x05, 2048);

    EXPECT_EQ(mma8451q_loop(&dev), 0);
    EXPECT_NEAR(dev.data.x, 1.0, 1e-3);
    EXPECT_NEAR(dev.data.y, -1.0, 1e-3);
    EXPECT_NEAR(dev.data.z, 0.5, 1e-3);
    EXPECT_EQ(dev.error, false);
}

TEST_F(MMA8451QTests, testLoopCommunicationError) {
    mockMMA8451Q mockI2C(MMA8451Q_I2C_ADDR_0);
    fakeHAL_I2C_addDevice(&mockI2C);
    ASSERT_EQ(mma8451q_init(&dev, &hi2c, MMA8451Q_I2C_ADDR_0), 0);

    mockI2C.rxRet = HAL_ERROR;
    EXPECT_EQ(mma8451q_loop(&dev), -1);
    EXPECT_EQ(dev.error, true);
}
