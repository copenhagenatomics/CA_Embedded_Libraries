/*
 * sht45.h
 *
 *  Created on: June 10, 2024
 *      Author: Matias
 * 
 *  Datasheet: https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf
 */

#ifndef INC_SHT45_H_
#define INC_SHT45_H_

#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#elif defined(STM32H753xx)
#include "stm32h7xx_hal.h"
#endif

#include <stdint.h>

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// I2C addresses for SHT45 devices
#define SHT45_I2C_ADDR              0x44

// List of valid commands (Section 4.5 of the datasheet)
#define SHT4X_MEASURE_HIGHREP       0xFD
#define SHT4X_MEASURE_MEDREP        0xF6
#define SHT4X_MEASURE_LOWREP        0xE0
#define SHT4X_READ_SERIAL           0x89
#define SHT4X_SOFT_RESET            0x94
#define SHT4X_HEATER_200mW_1s       0x39
#define SHT4X_HEATER_200mW_100ms    0x32
#define SHT4X_HEATER_110mW_1s       0x2F
#define SHT4X_HEATER_110mW_100ms    0x24
#define SHT4X_HEATER_20mW_1s        0x1E
#define SHT4X_HEATER_20mW_100ms     0x15
#define SHT4X_ABORT_CALL            0x06

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t device_address;
} sht4x_handle_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

HAL_StatusTypeDef sht4x_soft_reset(sht4x_handle_t *handle);
HAL_StatusTypeDef sht4x_abort_command(sht4x_handle_t *handle);
HAL_StatusTypeDef sht4x_get_serial(sht4x_handle_t *handle, uint32_t *serial_number);
HAL_StatusTypeDef sht4x_get_measurement(sht4x_handle_t *handle, float *temperature, float *humidity);
HAL_StatusTypeDef sht4x_turn_on_heater(sht4x_handle_t *handle, uint8_t heating_program);

#endif /* INC_SHT45_H_ */