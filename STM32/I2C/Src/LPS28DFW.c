/*!
 * @file    LPS28DFW.c
 * @brief   Driver file for LPS28DFW pressure sensor
 * @date    07/01/2026
 * @author  Timothé Dodin
 */

#include <stdint.h>

#include "LPS28DFW.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define WRITE_CMD 0x01
#define READ_CMD  0x00

#define PRESSURE_OUT_XL 0x28
#define PRESSURE_OUT_L  0x29
#define PRESSURE_OUT_H  0x2A
#define TEMP_OUT_L      0x2B
#define TEMP_OUT_H      0x2C

#define CTRL_REG1 0x10
#define CTRL_REG2 0x11
#define CTRL_REG3 0x12
#define CTRL_REG4 0x13

#define CTRL_REG1_ODR_AVG      0x00
#define CTRL_REG2_FS_MODE_1260 0x00
#define CTRL_REG2_FS_MODE_4060 0x40
#define CTRL_REG2_SWRESET      0x04
#define CTRL_REG4_DRDY         0x20

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void write_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t value);
static uint8_t read_register(lps28dfw_t *dev, uint8_t reg_address);
static void config_registers(lps28dfw_t *dev);
static void get_new_measurement(lps28dfw_t *dev);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

static void write_register(lps28dfw_t *dev, uint8_t reg_address, uint8_t value) {
    HAL_I2C_Master_Transmit(dev->hi2c, dev->address | WRITE_CMD, &value, 1, 1);
}

static uint8_t read_register(lps28dfw_t *dev, uint8_t reg_address) {return 1;}

static void config_registers(lps28dfw_t *dev) {
    write_register(dev, CTRL_REG1, CTRL_REG1_ODR_AVG);
    write_register(dev, CTRL_REG4, CTRL_REG4_DRDY);
}

static void get_new_measurement(lps28dfw_t *dev) {
    static const float PRESSURE_SENSITIVITY_HI = 2.048;  // [1/bar]  - High range
    static const float PRESSURE_SENSITIVITY_LO = 4.096;  // [1/bar]  - Low range
    static const float TEMPERATURE_SENSITIVITY = 100.0;  // [1/degC]

    int32_t press_out_xl = (int32_t)read_register(dev, PRESSURE_OUT_XL);
    int32_t press_out_l  = (int32_t)read_register(dev, PRESSURE_OUT_L);
    int32_t press_out_h  = (int32_t)read_register(dev, PRESSURE_OUT_H);

    int32_t pressureAdc = (press_out_h << 16) | (press_out_l << 8) | (press_out_xl);
    dev->data.pressure  = (dev->fullScale) ? (pressureAdc / PRESSURE_SENSITIVITY_HI)
                                           : (pressureAdc / PRESSURE_SENSITIVITY_LO);

    int32_t temp_out_l = (int32_t)read_register(dev, TEMP_OUT_L);
    int32_t temp_out_h = (int32_t)read_register(dev, TEMP_OUT_H);

    int32_t temperatureAdc = (temp_out_h << 8) | temp_out_l;
    dev->data.temperature  = temperatureAdc / TEMPERATURE_SENSITIVITY;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

void lps28dfw_init(lps28dfw_t *dev, I2C_HandleTypeDef *hi2c, uint8_t address) {
    dev->hi2c    = hi2c;
    dev->address = address;

    config_registers(dev);
}

void lps28dfw_loop(lps28dfw_t *dev) {
    if (stmGetGpio(*dev->intDrdy)) {
        get_new_measurement(dev);
    }
}
