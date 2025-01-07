/*
 * sht35.c
 *
 *  Created on: Apr 15, 2021
 *      Author: alexander.mizrahi@copenhagenAtomics.com
 */

#include <assert.h>

#include "sht35.h"
#include "crc.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define CRC_INIT 0xFFU
#define CRC_POLY 0x31U

HAL_StatusTypeDef ret;


HAL_StatusTypeDef sht3x_send_command(sht3x_handle_t *handle, sht3x_command_t command){
	//divides the uint16_t command variable into two uint8_t variables
	uint8_t command_buffer[2] = {(command & 0xff00u) >> 8u, command & 0xffu};


	ret = HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address << 1u, command_buffer, sizeof(command_buffer), 50);
		if(ret != HAL_OK){
		return ret;
	}

	return HAL_OK;
}

static uint16_t uint8Merge(uint8_t msb, uint8_t lsb){
	return (uint16_t)((uint16_t)msb << 8u) | lsb;
}

HAL_StatusTypeDef sht3x_init(sht3x_handle_t *handle){

	assert(handle->i2c_handle->Init.NoStretchMode == I2C_NOSTRETCH_DISABLE);
	// TODO: Assert i2c frequency is not too high

	uint8_t status_reg_and_checksum[3];
	ret = HAL_I2C_Mem_Read(handle->i2c_handle, handle->device_address << 1u, SHT3X_COMMAND_READ_STATUS, 2, (uint8_t*)&status_reg_and_checksum,
					  sizeof(status_reg_and_checksum), 50);
	if (ret != HAL_OK) {
		return ret;
	}

	initCrc8(CRC_INIT, CRC_POLY);
	uint8_t calculated_crc = crc8Calculate(status_reg_and_checksum, 2);

	if (calculated_crc != status_reg_and_checksum[2]) {
		return HAL_ERROR;
	}

	return HAL_OK;
}

HAL_StatusTypeDef sht3x_read_temperature_and_humidity(sht3x_handle_t *handle, float *temperature, float *humidity){
	sht3x_send_command(handle, SHT3X_COMMAND_MEASURE_HIGHREP_STRETCH);

	HAL_Delay(1);

	uint8_t buffer[6];
	ret = HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address << 1u, buffer, sizeof(buffer), 50);
	if(ret != HAL_OK) {
		return ret;
	}

	//checks if data is correct using CRC
	initCrc8(CRC_INIT, CRC_POLY);
	uint8_t temperature_crc = crc8Calculate(buffer, 2);
	uint8_t humidity_crc = crc8Calculate(buffer + 3, 2);
	if (temperature_crc != buffer[2] || humidity_crc != buffer[5]) {
		return HAL_ERROR;
	}

	//merges data into a uint16
	uint16_t temperature_raw = uint8Merge(buffer[0], buffer[1]);
	uint16_t humidity_raw = uint8Merge(buffer[3], buffer[4]);

	//processes data
	*temperature = -45.0f + 175.0f * (float)temperature_raw / 65535.0f;
	*humidity = 100.0f * (float)humidity_raw / 65535.0f;

	return HAL_OK;
}
