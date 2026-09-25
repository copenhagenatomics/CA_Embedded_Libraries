/*!
 * @file    MMA8451Q.h
 * @brief   Header file of MMA8451Q.c
 * @date    03/09/2026
 * @author  Luke Walker
 */

#ifndef INC_MMA8451Q_H_
#define INC_MMA8451Q_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// I2C addresses for MMA8451Q devices (depends on the state of the SA0 pin)
#define MMA8451Q_I2C_ADDR_0 0x1C
#define MMA8451Q_I2C_ADDR_1 0x1D

typedef struct _mma8451q_data {
    float x;  // [g]
    float y;  // [g]
    float z;  // [g]
} mma8451q_data_t;

typedef struct _mma8451q {
    I2C_HandleTypeDef* hi2c;  // I2C handler
    uint8_t address;          // I2C address
    mma8451q_data_t data;     // Measurement
    bool error;               // Communication error
} mma8451q_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int mma8451q_init(mma8451q_t* dev, I2C_HandleTypeDef* hi2c, uint8_t address);
int mma8451q_loop(mma8451q_t* dev);

#endif /* INC_MMA8451Q_H_ */
