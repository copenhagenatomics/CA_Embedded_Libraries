/*!
** @brief Driver file for the SM4291 Pressure/Temperature sensor from TE connectivity
**
** https://www.te.com/en/product-4291-HGE-S-500-000.html
**
** @author Luke W
** @date   12/12/2024
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "sm4291.h"
#include "crc.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* These values were supplied from TE connectivity in an email. They cannot be found in the 
** datasheet: 
** * Scale: 0.002578
** * Offset: 42
**
** Note: these are close to the numbers obtained if you fit the full range of the temperature 
** register dsb_t (-32768 - 32767) to the absolute maximum storage temperature range from the
** datasheet.
**
** There is a significant offset difference between the output of this equation and the output of a 
** thermocouple placed inside the box. The pressure sensor is ~5.5 degC higher. This is put down to 
** the thermal resistivity of the package, and hopefully reflects the actual temperature of the 
** pressure sensor. It probably shouldn't be used as a reflection of the temperature of the inside 
** of the LL box.
**
** Experimentation yielded the numbers below, although they don't match quite so well:
** * Scale: 3.1471e-3
** * Offset: 40
*/
#define TEMP_ADC_SCALAR     2.578e-3
#define TEMP_ADC_OFFSET     42

/* All devices have a fixed address */
#define TEMP_I2C_NO_CRC_ADDR    0x6C
#define TEMP_I2C_CRC_ADDR       0x6D

/* Register addresses */
#define ADDR_CMD            0x22U
#define ADDR_DSP_T          0x2EU
#define ADDR_DSP_P          0x30U
#define ADDR_STATUS_SYNC    0x32U
#define ADDR_STATUS         0x36U
#define ADDR_SER0           0x50U
#define ADDR_SER1           0x52U

/* CMDs */
#define CMD_SLEEP   0x6C32
#define CMD_RESET   0xB169

#define PRESS_RANGE 52429U

#define CRC8_POLY 0xD5U
#define CRC8_INIT 0xFFU
#define CRC4_INIT 0x0FU
#define CRC4_POLY 0x03U

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int sm4291ReadReg(sm4291_i2c_handle_t* i2c, uint8_t reg_addr, uint16_t* result);
static int sm4291WriteReg(sm4291_i2c_handle_t* i2c, uint8_t reg_addr, uint16_t value);

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
** @brief Initialise the sm4291 device on the given bus.
**
** @param[in] hi2c Handle to the STM32 I2C peripheral
** @param[in] crc  Whether to use CRC protection on communications
** @param[in] press_min Minimum pressure reading of device (unitless)
** @param[in] press_max Maximum pressure reading of device (unitless)
**
** @return A handle for future I2C communications, or null if an error is detected
*/
sm4291_i2c_handle_t* sm4291Init(I2C_HandleTypeDef* hi2c, bool crc, double press_min, double press_max) {
    assert_param(hi2c);

    /* Temporary device handle while we check things */
    sm4291_i2c_handle_t temp = {hi2c, crc};

    /* Check the serial is non-zero to see if the I2C bus is working */
    uint32_t serial = 0;
    int res = sm4291GetSerial(&temp, &serial);
    if(res == 0 && serial != 0 && serial != 0xFFFFFFFF) {
        temp.serial = serial;

        /* Pressure transformation */
        temp.press_scaler = (press_max - press_min) / PRESS_RANGE;
        temp.press_offset = press_max - temp.press_scaler * (PRESS_RANGE / 2U);

        void* ret = malloc(sizeof(temp));

        if(ret) {
            ret = memcpy(ret, &temp, sizeof(temp));
        }

        return (sm4291_i2c_handle_t*) ret;
    }
    else {
        return NULL;
    }
}

/*!
** @brief Closes the open driver associated with a given handle
*/
void sm4291Close(sm4291_i2c_handle_t* i2c) {
    assert_param(i2c);

    free(i2c);
}

/*!
** @brief Get the temperature from the sensor
*/
int sm4291GetTemp(sm4291_i2c_handle_t* i2c, double* result) {
    int16_t temp;
    if(0 != sm4291ReadReg(i2c, ADDR_DSP_T, (uint16_t*)&temp)) {
        return -1;
    };

    *result = temp * TEMP_ADC_SCALAR + TEMP_ADC_OFFSET;
    return 0;
}

/*!
** @brief Get the pressure from the sensor
*/
int sm4291GetPressure(sm4291_i2c_handle_t* i2c, double* result) {
    assert_param(i2c);

    int16_t press;
    if(0 != sm4291ReadReg(i2c, ADDR_DSP_P, (uint16_t*)&press)) {
        return -1;
    };

    *result = press * i2c->press_scaler + i2c->press_offset;
    return 0;
}

/*!
** @brief Get the serial number from the sensor
*/
int sm4291GetSerial(sm4291_i2c_handle_t* i2c, uint32_t* result) {
    uint16_t ser0;
    uint16_t ser1;

    if(0 != sm4291ReadReg(i2c, ADDR_SER0, &ser0)) {
        return -1;
    };

    if(0 != sm4291ReadReg(i2c, ADDR_SER1, &ser1)) {
        return -2;
    };

    *result = (((uint32_t)ser1 << 16U) & 0xFFFF0000U) | ((uint32_t)ser0 & 0xFFFFU);
    return 0;
}

/*!
** @brief Get the status register
*/
int sm4291GetStatus(sm4291_i2c_handle_t* i2c, uint16_t* status) {
    return sm4291ReadReg(i2c, ADDR_STATUS, status);
}

/*!
** @brief Get the status sync register
*/
int sm4291GetStatusSync(sm4291_i2c_handle_t* i2c, uint16_t* status) {
    return sm4291ReadReg(i2c, ADDR_STATUS_SYNC, status);
}

/*!
** @brief Reset the device
*/
int sm4291Reset(sm4291_i2c_handle_t* i2c) {
    return sm4291WriteReg(i2c, ADDR_CMD, CMD_RESET);
}

/*!
** @brief Put the device into sleep mode
**
** @note The datasheet doesn't specify how to get the device out of sleep mode
*/
int sm4291Sleep(sm4291_i2c_handle_t* i2c) {
    return sm4291WriteReg(i2c, ADDR_CMD, CMD_SLEEP);
}

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Reads a 16-bit register from the device
*/
static int sm4291ReadReg(sm4291_i2c_handle_t* i2c, uint8_t reg_addr, uint16_t* result) {
    assert_param(i2c);

    uint16_t dev_addr = i2c->crc ? TEMP_I2C_CRC_ADDR : TEMP_I2C_NO_CRC_ADDR;

    if(i2c->crc) {
        initCrc4(CRC4_INIT, CRC4_POLY);
        initCrc8(CRC8_INIT, CRC8_POLY);

        uint8_t crc4_data[2U] = {(uint8_t)(reg_addr >> 4U), ((reg_addr & 0xFU) << 4U) | 0x1U};
        uint8_t crc4 = crc4Calculate(crc4_data, 2U);

        uint8_t addr_buf[2U] = {reg_addr, (uint8_t)(0x10U | crc4)};
        if(HAL_OK != HAL_I2C_Master_Transmit(i2c->i2c, dev_addr << 1U, addr_buf, 2U, 1U)) {
            return -1;
        };

        uint8_t data_buf[3U] = {0};
        if(HAL_OK != HAL_I2C_Master_Receive(i2c->i2c, dev_addr << 1U, data_buf, 3U, 1U)) {
            return -2;
        };

        if(crc8Calculate(data_buf, 2U) != data_buf[2U]) {

            return -3;
        }

        *result = ((uint16_t) data_buf[1U] << 8U) | data_buf[0];
    }
    else {
        if(HAL_OK != HAL_I2C_Master_Transmit(i2c->i2c, dev_addr << 1U, &reg_addr, 1U, 1U)) {
            return -1;
        };

        if(HAL_OK != HAL_I2C_Master_Receive(i2c->i2c, dev_addr << 1U, (uint8_t*)result, 2U, 1U)) {
            return -2;
        };
    }

    return 0;
}

/*!
** @brief Writes a 16-bit register to the device
*/
static int sm4291WriteReg(sm4291_i2c_handle_t* i2c, uint8_t reg_addr, uint16_t value) {
    assert_param(i2c);

    uint16_t dev_addr = i2c->crc ? TEMP_I2C_CRC_ADDR : TEMP_I2C_NO_CRC_ADDR;

    if(i2c->crc) {
        initCrc4(CRC4_INIT, CRC4_POLY);
        initCrc8(CRC8_INIT, CRC8_POLY);

        uint8_t crc4_data[2U] = {(uint8_t)(reg_addr >> 4U), ((reg_addr & 0xF) << 4U) | 0x1U};
        uint8_t crc8_data[2U] = {(uint8_t)(value & 0xFFU), (uint8_t)((value >> 8U) & 0xFFU)};
        uint8_t crc4 = crc4Calculate(crc4_data, 2U);
        uint8_t crc8 = crc8Calculate(crc8_data, 2U);

        uint8_t buf[5U] = {reg_addr, (uint8_t)(0x10 | crc4), (uint8_t)(value & 0xFFU), (uint8_t)((value >> 8U) & 0xFFU), crc8};
        return (int)HAL_I2C_Master_Transmit(i2c->i2c, dev_addr << 1U, buf, 5U, 1U);
    }
    else {
        uint8_t buf[3U] = {reg_addr, (uint8_t)(value & 0xFF), (uint8_t)((value >> 8U) & 0xFFU)};
        return (int)HAL_I2C_Master_Transmit(i2c->i2c, dev_addr << 1U, buf, 3U, 1U);
    }
}
