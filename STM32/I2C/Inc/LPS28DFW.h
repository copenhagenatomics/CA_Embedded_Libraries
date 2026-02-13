/*!
 * @file    LPS28DFW.h
 * @brief   Header file of LPS28DFW.c
 * @date    07/01/2026
 * @author  Timothé Dodin
 */

#ifndef INC_LPS28DFW_H_
#define INC_LPS28DFW_H_

#include <stdbool.h>
#include <stdint.h>

#include "StmGpio.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// I2C addresses for LPS28DFW devices
#define LPS28DFW_I2C_ADDR_0 0x5C
#define LPS28DFW_I2C_ADDR_1 0x5D

#define INCORRECT_VALUE 10000.0  // In case of communication error

typedef struct _lps28dfw_data {
    float pressure;
    float temperature;
} lps28dfw_data_t;

typedef struct _lps28dfw {
    I2C_HandleTypeDef *hi2c;  // I2C handler
    StmGpio *intDrdy;         // Data ready pin
    uint8_t address;          // I2C address
    bool fullScale;           // 4060 hPa / 1260 hPa
    lps28dfw_data_t data;     // Measurement
    uint32_t lastAdcTime;     // [ms] - Timestamp of last measurement
    bool error;               // Communication error
} lps28dfw_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int lps28dfw_init(lps28dfw_t *dev, I2C_HandleTypeDef *hi2c, uint8_t address, StmGpio *intDrdy);
int lps28dfw_loop(lps28dfw_t *dev);

#endif /* INC_LPS28DFW_H_ */
