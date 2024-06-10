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

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t device_address;
} sht4x_handle_t;

// Valid commands can be found in section 4.5 of the datasheet
typedef enum
{
    SHT4X_COMMAND_MEASURE_HIGHREP = 0xFD,
    SHT4X_COMMAND_MEASURE_MEDREP = 0xF6,
    SHT4X_COMMAND_MEASURE_LOWREP = 0xE0,
    SHT4X_COMMAND_READ_SERIAL = 0x89,
    SHT4X_COMMAND_SOFT_RESET = 0x94,
    SHT4X_COMMAND_HEATER_200mW_1S = 0x39,
    SHT4X_COMMAND_HEATER_200mW_100mS = 0x32,
    SHT4X_COMMAND_HEATER_110mW_1S = 0x2F,
    SHT4X_COMMAND_HEATER_110mW_100mS = 0x24,
    SHT4X_COMMAND_HEATER_20mW_1S = 0x1E,
    SHT4X_COMMAND_HEATER_20mW_100mS = 0x15
} sht4x_command_t;

#endif /* INC_SHT45_H_ */