/*!
 * @file    TCI.h
 * @brief   Header file of TCI.c
 * @date    22/04/2026
 * @author  Timothé Dodin
 */

#ifndef INC_TCI_H_
#define INC_TCI_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef enum _tci_state {
    H2_MEAS = 0,  // Waiting for H2 measurement to be ready
    TEMP_MEAS,    // Waiting for temperature measurement to be ready
    NO_OF_TCI_STATES
} tci_state_t;

typedef struct _tci_data {
    float H2;           // [ppm]
    float temperature;  // [degC]
} tci_data_t;

typedef struct _tci {
    uint32_t id;                   // Sensor ID
    tci_state_t state;             // H2 sensor state
    I2C_HandleTypeDef *hi2c;       // I2C handler
    tci_data_t data;               // Measurement
    uint32_t lastMeasTime;         // [ms] - Timestamp of last measurement
    uint32_t lastStateChangeTime;  // [ms] - Timestamp for last state change
    bool error;                    // Communication error
} tci_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int tci_init(tci_t *dev, I2C_HandleTypeDef *hi2c);
int tci_loop(tci_t *dev, float relHumidity, float temperature, float pressure);

#endif /* INC_LPS28DFW_H_ */
