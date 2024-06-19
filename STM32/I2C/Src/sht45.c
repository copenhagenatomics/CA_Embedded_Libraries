/*
 * sht45.c
 *
 *  Created on: June 10, 2024
 *      Author: matias
 * 
 *  Datasheet: https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf
 */

#include "sht45.h"
#include <math.h>

/***************************************************************************************************
** PRIVATE FUNCTION PROTOTYPES
***************************************************************************************************/

static uint8_t calculate_crc(const uint8_t *data, size_t length);
static int checkCRC(uint8_t *buffer);
static void relativeToAbsolute(sht4x_handle_t *dev);
static HAL_StatusTypeDef sht4x_set_mode(sht4x_handle_t *dev, uint8_t command);

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Computes CRC of incoming data
** @note CRC parameters can be found in section 4.4 of the datasheet 
*/
static uint8_t calculate_crc(const uint8_t *data, size_t length)
{
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

/*!
** @brief Compares computed and received CRCs
** @note For every transfer there are two CRCs.
*/
static int checkCRC(uint8_t *buffer)
{
    // Ensure data is valid
    if (calculate_crc(buffer, 2) != buffer[2] || calculate_crc(buffer + 3, 2) != buffer[5]) {
        return 1;
    }
    return 0;
}

/*!
** @brief Conversion from relative [% to dew point] to absolute humidity [grams/m^3]
*/
static void relativeToAbsolute(sht4x_handle_t *dev)
{
    dev->data.absolute_humidity = 6.112 * exp((17.67*dev->data.temperature)/(dev->data.temperature+243.5))*dev->data.relative_humidity*2.1674/(273.15+dev->data.temperature);
}

/*!
** @brief Function to send any valid command to SHT4x chip (except for abort call)
*/
static HAL_StatusTypeDef sht4x_set_mode(sht4x_handle_t *dev, uint8_t command)
{
    if (HAL_I2C_Master_Transmit(dev->hi2c, dev->device_address << 1u, &command, 1, 1) != HAL_OK) {
        return HAL_BUSY;
    }
    return HAL_OK;
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
** @brief Soft reset of SHT4x chip
** @note Soft reset takes approximately 1ms
*/
HAL_StatusTypeDef sht4x_soft_reset(sht4x_handle_t *dev)
{
    return sht4x_set_mode(dev, SHT4X_SOFT_RESET);
}

/*!
** @brief Abort any ongoing command or heating process
*/
HAL_StatusTypeDef sht4x_abort_command(sht4x_handle_t *dev)
{
    static uint8_t abort_call = 0x06;
    if (HAL_I2C_Master_Transmit(dev->hi2c, 0x00, &abort_call, 1, 1) != HAL_OK) {
        return HAL_BUSY;
    }
    return HAL_OK;
}

/*!
** @brief Get serial number of SHT4x chip
** @param serial_number: pointer to uint32_t
*/
HAL_StatusTypeDef sht4x_get_serial(sht4x_handle_t *dev)
{
    HAL_StatusTypeDef ret = HAL_ERROR;
    // Send serial read command to output serial from SHT45 at next read 
    ret = sht4x_set_mode(dev, SHT4X_READ_SERIAL);
    HAL_Delay(1);

    if(ret != HAL_OK) {
        return ret;
    }

    uint8_t buffer[6];
    ret = HAL_I2C_Master_Receive(dev->hi2c, dev->device_address << 1u, buffer, sizeof(buffer), 2);
    if(ret != HAL_OK) {
        return ret;
    }

    if (checkCRC(buffer) != 0)
    {
        return HAL_ERROR;
    }

    dev->serial_number = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[3] << 8) | buffer[4];
    if (dev->serial_number == 0)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/*!
** @brief Get temperature and RH% measurement
*/
HAL_StatusTypeDef sht4x_get_measurement(sht4x_handle_t *dev)
{   
    uint8_t buffer[6];
    HAL_StatusTypeDef ret = HAL_I2C_Master_Receive(dev->hi2c, dev->device_address << 1u, buffer, sizeof(buffer), 2);
    
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

    // Convert data to physical quantities
    dev->data.temperature = -45.0f + 175.0f * temperature_adc / 65535.0f;
    dev->data.relative_humidity = -6.0f + 125.0f * humidity_adc / 65535.0f;

    // Cut relative humidity measure if outside of physical bounds
    if (dev->data.relative_humidity > 100)
    {
        dev->data.relative_humidity = 100;
    } 
    else if (dev->data.relative_humidity < 0)
    {
        dev->data.relative_humidity = 0;
    }

    // Compute absolute humidity
    relativeToAbsolute(dev);

    return ret;
}

/*!
** @brief Choose measurement mode
** @param heating_program: Valid inputs can be found in sht45.h in defines of the form
**                         SHT4X_MEASURE_*.
** @note The user is responsible for waiting the time it takes for a specific conversion
**       before calling the sht4x_get_measurement(sht4x_handle_t *dev)
**          SHT4X_MEASURE_HIGHREP       Max conversion time 8.3 ms
**          SHT4X_MEASURE_MEDREP        Max conversion time 4.5 ms
**          SHT4X_MEASURE_LOWREP        Max conversion time 1.6 ms
*/
HAL_StatusTypeDef sht4x_initiate_measurement(sht4x_handle_t *dev, uint8_t command)
{
    return sht4x_set_mode(dev, command);
}

/*!
** @brief Turn on heating cycle
** @param heating_program: Valid inputs can be found in sht45.h in defines of the form
**                         SHT4X_HEATER_*.
**                         Available heater powers: 20, 110, 200mW
**                         Available heater durations: 100ms, 1s
** @note The maximum duty cycle of the heater is 10%. Should only be used in ambient temperatures
**       of <65C.  
**       At the end of the heater cycle a new measurement is available. The user is responsible for
**       waiting for heater duration + 10% before requesting the next measurement.
*/
HAL_StatusTypeDef sht4x_turn_on_heater(sht4x_handle_t *dev, uint8_t heating_program)
{
    return sht4x_set_mode(dev, heating_program);
}