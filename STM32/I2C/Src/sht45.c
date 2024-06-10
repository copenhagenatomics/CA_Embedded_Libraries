/*
 * sht45.c
 *
 *  Created on: June 10, 2024
 *      Author: matias
 * 
 *  Datasheet: https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf
 */

#include "sht45.h"

// CRC parameters can be found in section 4.4 of the datasheet 
static uint8_t calculate_crc(const uint8_t *data, size_t length){
    uint8_t crc = 0xff;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if ((crc & 0x80u) != 0) {
                crc = (uint8_t)((uint8_t)(crc << 1u) ^ 0x31u);
            } else {
                crc <<= 1u;
            }
        }
    }
    return crc;
}

static int checkCRC(uint8_t *buffer)
{
    // Ensure data is valid
    if (calculate_crc(buffer, 2) != buffer[2] || calculate_crc(buffer + 3, 2) != buffer[5]) {
        return 1;
    }
    return 0;
}

HAL_StatusTypeDef sht4x_set_mode(sht4x_handle_t *handle, sht4x_command_t command){

    if (HAL_I2C_Master_Transmit(handle->hi2c, handle->device_address << 1u, command, 1, 10) != HAL_OK) {
        return HAL_BUSY;
    }
    return HAL_OK;
}

HAL_StatusTypeDef sht4x_soft_reset(sht4x_handle_t *handle)
{
    // Soft reset the chip
    sht4x_set_mode(handle, SHT4X_COMMAND_SOFT_RESET);
}


HAL_StatusTypeDef sht4x_get_serial(sht4x_handle_t *handle, uint32_t *serial_number)
{
    // Send serial read command to output serial from SHT45 at next read 
    sht4x_set_mode(handle, SHT4X_COMMAND_READ_SERIAL);

    uint8_t buffer[6];
    ret = HAL_I2C_Master_Receive(handle->hi2c, handle->device_address << 1u, buffer, sizeof(buffer), 50);
    if(ret != HAL_OK) {
        return ret;
    }

    if (checkCRC(buffer) != 0)
    {
        return HAL_ERROR;
    }

    *serial_number = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[3] << 8) | buffer[4];
    return HAL_OK;
}

HAL_StatusTypeDef sht4x_get_measurement(sht4x_handle_t *handle, float *temperature, float *humidity){
    
    // Send measurement command to output temp and humidity from SHT45 at next read 
    sht4x_set_mode(handle, SHT4X_COMMAND_MEASURE_HIGHREP);

    uint8_t buffer[6];
    ret = HAL_I2C_Master_Receive(handle->hi2c, handle->device_address << 1u, buffer, sizeof(buffer), 50);
    if(ret != HAL_OK) {
        return ret;
    }

    if (checkCRC(buffer) != 0)
    {
        return HAL_ERROR;
    }

    //merges data into a uint16
    uint16_t temperature_adc = (buffer[0] << 8) | buffer[1];
    uint16_t humidity_adc = (buffer[3] << 8) | buffer[4];

    //processes data
    *temperature = -45.0f + 175.0f * temperature_adc / 65535.0f;
    *humidity = -6.0f + 125.0f * humidity_adc / 65535.0f;

    return HAL_OK;
}