/*!
 * @file    LPS28DFW.c
 * @brief   Driver file for LPS28DFW pressure sensor
 * @date    07/01/2026
 * @author  Timothé Dodin
 */

#include <stdint.h>

#include "LPS28DFW.h"
#include "stm32f4xx_hal.h"
#include "time32.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MAX_TIME_MS 150  // Time out between measurements

// Register addresses
#define STATUS          0x27
#define PRESSURE_OUT_XL 0x28
#define PRESSURE_OUT_L  0x29
#define PRESSURE_OUT_H  0x2A
#define TEMP_OUT_L      0x2B
#define TEMP_OUT_H      0x2C
#define CTRL_REG1       0x10
#define CTRL_REG2       0x11
#define CTRL_REG3       0x12
#define CTRL_REG4       0x13

// Control registers custom values
#define CTRL_REG1_ODR_10HZ     0x18
#define CTRL_REG1_AVG_512      0x07
#define CTRL_REG2_FS_MODE_1260 0x00
#define CTRL_REG2_FS_MODE_4060 0x40
#define CTRL_REG2_SWRESET      0x04
#define CTRL_REG4_DRDY         0x20

// Status register mapping
#define STATUS_T_OR_Msk 0x01
#define STATUS_P_OR_Msk 0x02
#define STATUS_T_DA_Msk 0x10
#define STATUS_P_DA_Msk 0x11

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int write_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t byte);
static int read_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t *value);
static int config_registers(lps28dfw_t *dev);
static int set_range(lps28dfw_t *dev, bool high);
static int get_new_measurement(lps28dfw_t *dev);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Writes a byte on the given register
 * @param  dev Pressure device
 * @param  reg_address Register address
 * @param  byte Byte to write
 * @return 0 if OK, else < 0
 */
static int write_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t byte) {
    uint8_t message[2] = {reg_address, byte};
    if (HAL_I2C_Master_Transmit(dev->hi2c, (dev->address << 1), message, 2, 1) != HAL_OK) {
        return -1;
    }
    return 0;
}

/*!
 * @brief  Reads a byte from the given register
 * @param  dev Pressure device
 * @param  reg_address Register address
 * @param  byte Pointer to destination byte
 * @return 0 if OK, else < 0
 */
static int read_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t *byte) {
    // Reading command
    if (HAL_I2C_Master_Transmit(dev->hi2c, (dev->address << 1), &reg_address, 1, 1) != HAL_OK) {
        return -1;
    }
    // Receiving one byte
    if (HAL_I2C_Master_Receive(dev->hi2c, (dev->address << 1), byte, 1, 1) != HAL_OK) {
        return -2;
    }
    return 0;
}

/*!
 * @brief  Initial configuration of registers
 * @param  dev Pressure device
 * @return 0 if OK, else < 0
 */
static int config_registers(lps28dfw_t *dev) {
    // Software reset
    if (write_register(dev, CTRL_REG2, CTRL_REG2_SWRESET) != 0) {
        return -1;
    }
    // Sets output frequency and averaging filter length
    if (write_register(dev, CTRL_REG1, CTRL_REG1_ODR_10HZ | CTRL_REG1_AVG_512) != 0) {
        return -2;
    }
    // Setup DRDY pin
    if (write_register(dev, CTRL_REG4, CTRL_REG4_DRDY) != 0) {
        return -3;
    }
    return 0;
}

/*!
 * @brief  Sets the pressure range
 * @param  dev Pressure device
 * @param  high true for 4060 hPa, false for 1260 hPa
 * @return 0 if OK, else < 0
 */
static int set_range(lps28dfw_t *dev, bool high) {
    if (write_register(dev, CTRL_REG2, high ? CTRL_REG2_FS_MODE_4060 : CTRL_REG2_FS_MODE_1260) !=
        0) {
        return -1;
    }
    dev->fullScale = high;
    return 0;
}

/*!
 * @brief  Gets new presssure and temperature
 * @param  dev Pressure device
 * @return 0 if OK, else < 0
 */
static int get_new_measurement(lps28dfw_t *dev) {
    static const float PRESSURE_SENSITIVITY_HI = 2.048e6;  // [1/bar]  - High range
    static const float PRESSURE_SENSITIVITY_LO = 4.096e6;  // [1/bar]  - Low range
    static const float TEMPERATURE_SENSITIVITY = 100.0;    // [1/degC]

    int32_t press_out_xl = 0;
    int32_t press_out_l  = 0;
    int32_t press_out_h  = 0;
    int32_t temp_out_l   = 0;
    int32_t temp_out_h   = 0;

    if (read_register(dev, PRESSURE_OUT_XL, (uint8_t *)&press_out_xl) != 0) {
        return -1;
    }
    if (read_register(dev, PRESSURE_OUT_L, (uint8_t *)&press_out_l) != 0) {
        return -2;
    }
    if (read_register(dev, PRESSURE_OUT_H, (uint8_t *)&press_out_h) != 0) {
        return -3;
    }

    int32_t pressureAdc = (press_out_h << 16) | (press_out_l << 8) | (press_out_xl);
    dev->data.pressure  = (dev->fullScale) ? (pressureAdc / PRESSURE_SENSITIVITY_HI)
                                           : (pressureAdc / PRESSURE_SENSITIVITY_LO);

    if (read_register(dev, TEMP_OUT_L, (uint8_t *)&temp_out_l) != 0) {
        return -4;
    }
    if (read_register(dev, TEMP_OUT_H, (uint8_t *)&temp_out_h) != 0) {
        return -5;
    }

    int32_t temperatureAdc = (temp_out_h << 8) | temp_out_l;
    dev->data.temperature  = temperatureAdc / TEMPERATURE_SENSITIVITY;

    return 0;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Initializes pressure sensor
 * @param  dev Pressure device
 * @param  hi2c I2C handler
 * @param  address I2C address
 * @param  intDrdy Data ready pin
 * @return 0 if OK, else < 0
 */
int lps28dfw_init(lps28dfw_t *dev, I2C_HandleTypeDef *hi2c, uint8_t address, StmGpio *intDrdy) {
    dev->hi2c             = hi2c;
    dev->intDrdy          = intDrdy;
    dev->address          = address;
    dev->fullScale        = true;
    dev->data.pressure    = INCORRECT_VALUE;
    dev->data.temperature = INCORRECT_VALUE;
    dev->lastAdcTime      = HAL_GetTick();
    dev->error            = false;

    // Setup registers
    if (config_registers(dev) != 0) {
        dev->error = true;
        return -1;
    }
    // High range by default
    if (set_range(dev, true) != 0) {
        dev->error = true;
        return -2;
    }
    dev->error = false;
    return 0;
}

/*!
 * @brief  Pressure sensor loop
 * @return 0 if OK, else < 0
 */
int lps28dfw_loop(lps28dfw_t *dev) {
    uint32_t now = HAL_GetTick();
    if (stmGetGpio(*dev->intDrdy)) {
        dev->lastAdcTime = now;
        if (get_new_measurement(dev) != 0) {
            dev->data.pressure    = INCORRECT_VALUE;
            dev->data.temperature = INCORRECT_VALUE;
            dev->error            = true;
            return -1;
        }
    }
    else if (tdiff_u32(now, dev->lastAdcTime) > MAX_TIME_MS) {
        dev->data.pressure    = INCORRECT_VALUE;
        dev->data.temperature = INCORRECT_VALUE;
        dev->error            = true;
        return -2;
    }
    dev->error = false;
    return 0;
}
