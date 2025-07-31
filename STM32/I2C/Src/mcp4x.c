/*!
 * @file    mcp4xxx.c
 * @brief   Driver file for MCP4x family of digital potentiometers/rheostats
 * @date    28/07/2025
 * @authors Luke Walker
 * 
 * Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/22096b.pdf
 */

#include <stdint.h>

#include "stm32f4xx_hal.h"

#include "mcp4x.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MCP4X_WIPER_0_ADDR 0x00
#define MCP4X_WIPER_1_ADDR 0x01
#define MCP4X_STATUS_ADDR 0x05

#define MCP4X_WRITE_CMD 0x00
#define MCP4X_READ_CMD 0x0C
#define MCP4X_CMD_MSK 0x0C

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Initializes the MCP4x handle
**
** @param handle Empty structure for initialisation
** @param hi2c   Handle for I2C peripheral
** @param i2c_address I2C address of the MCP4x device
** @param num_bits Number of bits for the digital potentiometer (7/8)
** @param device_num Which device to initialise (valid for dual devices, always 0 for single devices)
** @return 0 on success, -1 on failure
*/
int mcp4x_init(mcp4x_handle_t* handle, I2C_HandleTypeDef* hi2c, uint8_t i2c_address, uint8_t num_bits, uint8_t device_num) {
    if (!handle || !hi2c || num_bits > 8 || num_bits < 7 || device_num > 1) {
        return -1;
    }

    handle->i2c_addr  = i2c_address;
    handle->hi2c      = hi2c;
    handle->num_bits  = num_bits;
    handle->max_value = (1 << num_bits) + 1; /* Acc. datasheet, it is 129 and 257 bits */
    handle->device_num = device_num;

    /* Test read to verify  */
    uint8_t command = MCP4X_STATUS_ADDR << 4 | MCP4X_READ_CMD;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(handle->hi2c, handle->i2c_addr << 1, &command, 1, 2);
    if (status != HAL_OK) {
        return -2; // I2C communication error
    }

    uint8_t data[2] = {0};
    status = HAL_I2C_Master_Receive(handle->hi2c, handle->i2c_addr << 1, data, 2, 2);

    if (status != HAL_OK) {
        return -3; // I2C communication error
    }

    /* Note: STATUS is fixed at 1F7h acc. datasheet for volatile devices. TODO: will this work for 
    ** non-volatile devices too? */
    return (data[0] == 0x01) && (data[1] == 0xF1) ? 0 : -4;
}

/*!
 * @brief Sets the wiper position of the MCP4x device
 *
 * @param handle Pointer to the initialized MCP4x handle
 * @param wiper_position Position to set the wiper (0 to max_value)
 * @return HAL status code
 */
int mcp4x_setWiperPos(mcp4x_handle_t* handle, uint16_t wiper_position) {
    if (!handle || wiper_position > handle->max_value) {
        return -1;
    }

    uint8_t command = (handle->device_num == 0 ? MCP4X_WIPER_0_ADDR : MCP4X_WIPER_1_ADDR) << 4;
    command |= MCP4X_WRITE_CMD;
    
    // MCP4651 expects 9-bit data: D8 in the LSB of the first byte
    uint8_t data[2];
    data[0] = command | ((wiper_position >> 8) & 0x01U);
    data[1] = wiper_position & 0xFFU;

    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(handle->hi2c, handle->i2c_addr << 1, data, 2, 2);
    
    return (status == HAL_OK) ? 0 : -2;
}

/*!
 * @brief Gets the wiper position of the MCP4x device
 *
 * @param handle Pointer to the initialized MCP4x handle
 * @param wiper_position Pointer to store the wiper position
 * @return HAL status code
 */
int mcp4x_getWiperPos(mcp4x_handle_t* handle, uint16_t* wiper_position) {
    if (!handle || !wiper_position) {
        return -1;
    }

    uint8_t command = (handle->device_num == 0 ? MCP4X_WIPER_0_ADDR : MCP4X_WIPER_1_ADDR) << 4;
    command |= MCP4X_READ_CMD;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(handle->hi2c, handle->i2c_addr << 1, &command, 1, 2);

    if(status != HAL_OK) {
        return -2;
    }

    /* MCP4651 expects 9-bit data: D8 in the LSB of the first byte */
    uint8_t data[2] = {0};
    status = HAL_I2C_Master_Receive(handle->hi2c, handle->i2c_addr << 1, data, 2, 2);

    if (status != HAL_OK) {
        return -3;
    }

    /* Combine the two bytes into a 9-bit value */
    *wiper_position = ((uint16_t)(data[0] & 0x01U) << 8) | ((uint16_t)data[1] & 0xFF);

    return 0;
}