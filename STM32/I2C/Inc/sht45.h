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

// Valid commands can be found in section 4.5 of the datasheet
#define SHT4X_MEASURE_HIGHREP       0xFD
#define SHT4X_MEASURE_MEDREP        0xF6
#define SHT4X_MEASURE_LOWREP        0xE0
#define SHT4X_READ_SERIAL           0x89
#define SHT4X_SOFT_RESET            0x94
#define SHT4X_HEATER_200mW_1S       0x39
#define SHT4X_HEATER_200mW_100mS    0x32
#define SHT4X_HEATER_110mW_1S       0x2F
#define SHT4X_HEATER_110mW_100mS    0x24
#define SHT4X_HEATER_20mW_1S        0x1E
#define SHT4X_HEATER_20mW_100mS     0x15


typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint16_t device_address;
} sht4x_handle_t;

#endif /* INC_SHT45_H_ */