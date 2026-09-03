/*!
 * @file    MMA8451Q.c
 * @date    03/09/2026
 * @author  Luke Walker
 *
 * @brief  This file contains the implementation of the driver for the MMA8451Q accelerometer, which
 *         is a 3-axis, 14-bit digital accelerometer. The driver provides functions to initialize 
 *         the device (in the highest precision and lowest data rate mode) and read acceleration 
 *         data from the sensor.
 * 
 *         Datasheet: https://cdn-shop.adafruit.com/datasheets/MMA8451Q-1.pdf
 */

#include <stdint.h>

#include "MMA8451Q.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Register addresses
#define STATUS       0x00
#define OUT_X_MSB    0x01
#define WHO_AM_I     0x0D
#define XYZ_DATA_CFG 0x0E
#define CTRL_REG1    0x2A
#define CTRL_REG2    0x2B

// Expected content of the WHO_AM_I register
#define WHO_AM_I_VALUE 0x1A

// STATUS: set when a new X/Y/Z sample is available since the last read
#define STATUS_ZYXDR_Msk 0x08

// XYZ_DATA_CFG: full scale range selection
#define FS_MODE_2G 0x00

// CTRL_REG1: DR2:DR0 output data rate selection. 0b111 = 1.56 Hz, the lowest selectable rate.
#define CTRL_REG1_DR_1_56HZ (0x07 << 3)
#define CTRL_REG1_LNOISE    (0x01 << 2)
#define CTRL_REG1_ACTIVE    0x01
#define CTRL_REG1_STANDBY   0x00

// CTRL_REG2: MODS1:MODS0 power mode selection. 0b10 = high resolution oversampling mode.
#define CTRL_REG2_MODS_HIGH_RES 0x02

// Sensitivity of the 14-bit output data in the 2g range [counts/g]
#define SENSITIVITY_2G 4096.0f

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int write_register(mma8451q_t* dev, uint8_t reg_address, uint8_t byte);
static int read_registers(mma8451q_t* dev, uint8_t reg_address, uint8_t* buf, uint16_t size);
static int config_registers(mma8451q_t* dev);
static int get_new_measurement(mma8451q_t* dev);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Writes a byte on the given register
 * @param  dev Accelerometer device
 * @param  reg_address Register address
 * @param  byte Byte to write
 * @return 0 if OK, else < 0
 */
static int write_register(mma8451q_t* dev, uint8_t reg_address, uint8_t byte) {
    if (HAL_I2C_Mem_Write(dev->hi2c, (dev->address << 1), reg_address, I2C_MEMADD_SIZE_8BIT, &byte,
                          1, 10) != HAL_OK) {
        return -1;
    }
    return 0;
}

/*!
 * @brief  Reads consecutive registers, starting from the given register
 * @param  dev Accelerometer device
 * @param  reg_address Register address
 * @param  buf Pointer to destination buffer
 * @param  size Number of bytes to read
 * @return 0 if OK, else < 0
 */
static int read_registers(mma8451q_t* dev, uint8_t reg_address, uint8_t* buf, uint16_t size) {
    if (HAL_I2C_Mem_Read(dev->hi2c, (dev->address << 1), reg_address, I2C_MEMADD_SIZE_8BIT, buf,
                         size, 10) != HAL_OK) {
        return -1;
    }
    return 0;
}

/*!
 * @brief  Initial configuration of registers: high resolution mode, lowest output data rate
 * @param  dev Accelerometer device
 * @return 0 if OK, else < 0
 */
static int config_registers(mma8451q_t* dev) {
    // The device must be in standby mode to change most control registers
    if (write_register(dev, CTRL_REG1, CTRL_REG1_STANDBY) != 0) {
        return -1;
    }
    // 2g full scale range
    if (write_register(dev, XYZ_DATA_CFG, FS_MODE_2G) != 0) {
        return -2;
    }
    // High resolution oversampling mode
    if (write_register(dev, CTRL_REG2, CTRL_REG2_MODS_HIGH_RES) != 0) {
        return -3;
    }
    // Lowest output data rate, then activate the device
    uint8_t ctrlReg1Value = CTRL_REG1_DR_1_56HZ | CTRL_REG1_LNOISE | CTRL_REG1_ACTIVE;
    if (write_register(dev, CTRL_REG1, ctrlReg1Value) != 0) {
        return -4;
    }
    return 0;
}

/*!
 * @brief  Gets new X, Y and Z acceleration
 * @param  dev Accelerometer device
 * @return 0 if OK, else < 0
 */
static int get_new_measurement(mma8451q_t* dev) {
    uint8_t raw[6] = {0};

    // OUT_X_MSB, OUT_X_LSB, OUT_Y_MSB, OUT_Y_LSB, OUT_Z_MSB and OUT_Z_LSB are consecutive registers
    if (read_registers(dev, OUT_X_MSB, raw, sizeof(raw)) != 0) {
        return -1;
    }

    // Data is 14-bit, left-justified two's complement. Shifting right by 2 sign-extends it.
    int16_t rawX = (int16_t)(((int16_t)raw[0] << 8) | raw[1]) >> 2;
    int16_t rawY = (int16_t)(((int16_t)raw[2] << 8) | raw[3]) >> 2;
    int16_t rawZ = (int16_t)(((int16_t)raw[4] << 8) | raw[5]) >> 2;

    dev->data.x = rawX / SENSITIVITY_2G;
    dev->data.y = rawY / SENSITIVITY_2G;
    dev->data.z = rawZ / SENSITIVITY_2G;

    return 0;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Initializes accelerometer in high resolution mode at the lowest output data rate
 * @param  dev Accelerometer device
 * @param  hi2c I2C handler
 * @param  address I2C address
 * @return 0 if OK, else < 0. -10 if WHO_AM_I could not be read/matched. Otherwise the return code
 *         of config_registers() is propagated as-is (-1: standby write, -2: XYZ_DATA_CFG write,
 *         -3: CTRL_REG2 write, -4: final CTRL_REG1/activate write) so the caller can tell exactly
 *         which configuration step failed.
 */
int mma8451q_init(mma8451q_t* dev, I2C_HandleTypeDef* hi2c, uint8_t address) {
    dev->hi2c    = hi2c;
    dev->address = address;
    dev->data.x  = 0.0f;
    dev->data.y  = 0.0f;
    dev->data.z  = 0.0f;
    dev->error   = false;

    // Confirm communication is established before configuring the device
    uint8_t whoAmI = 0;
    if (read_registers(dev, WHO_AM_I, &whoAmI, 1) != 0) {
        dev->error = true;
        return -10;
    }

    if (whoAmI != WHO_AM_I_VALUE) {
        dev->error = true;
        return -11;
    }

    int cfgResult = config_registers(dev);
    if (cfgResult != 0) {
        dev->error = true;
        return cfgResult;
    }

    dev->error = false;
    return 0;
}

/*!
 * @brief  Reads the latest X, Y and Z acceleration from the sensor
 * @param  dev Accelerometer device
 * @return 0 if OK, else < 0
 */
int mma8451q_loop(mma8451q_t* dev) {
    if (get_new_measurement(dev) != 0) {
        dev->error = true;
        return -1;
    }
    dev->error = false;
    return 0;
}
