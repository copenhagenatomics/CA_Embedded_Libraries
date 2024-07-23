/*!
** @brief Fake Interface to honeywell zephyr for unit testing
**
** @author Luke W
** @date   12/07/2024
*/

#include <map>

#include "fake_stm32xxxx_hal.h"
#include "fake_honeywellZephyrI2C.h"

/***************************************************************************************************
** PRIVATE DEFINITIONS
***************************************************************************************************/

typedef struct {
    HAL_StatusTypeDef   error;
    uint16_t            serial;
    uint16_t            flow;
} honeywellZephyr_t;

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

std::map<I2C_HandleTypeDef*, honeywellZephyr_t*> *i2c_map = nullptr;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

honeywellZephyr_t* getZephyrStruct(I2C_HandleTypeDef *hi2c) {
    if(!i2c_map) {
        i2c_map = new std::map<I2C_HandleTypeDef*, honeywellZephyr_t*>();
    }

    if(!i2c_map->count(hi2c)) {
        i2c_map->insert(std::pair<I2C_HandleTypeDef*, honeywellZephyr_t*>(hi2c, new honeywellZephyr_t()));
    }

    return i2c_map->at(hi2c);
}

/*!
** @brief Sets if there is an I2C error or not
*/
void setI2cError(I2C_HandleTypeDef *hi2c, HAL_StatusTypeDef error) {
    honeywellZephyr_t* hz = getZephyrStruct(hi2c);
    hz->error = error;
}

void setSerialNumber(I2C_HandleTypeDef *hi2c, uint16_t serial) {
    honeywellZephyr_t* hz = getZephyrStruct(hi2c);
    hz->serial = serial;
}

void setFlow(I2C_HandleTypeDef *hi2c, float flow) {
    honeywellZephyr_t* hz = getZephyrStruct(hi2c);
    hz->flow = flow;
}

HAL_StatusTypeDef honeywellZephyrRead(I2C_HandleTypeDef *hi2c, float *flowData) {
    honeywellZephyr_t* hz = getZephyrStruct(hi2c);

    if(hz->flow) {
        *flowData = hz->flow;
    }

    return hz->error;
}

HAL_StatusTypeDef honeywellZephyrSerial(I2C_HandleTypeDef *hi2c, uint32_t *serialNB) {
    honeywellZephyr_t* hz = getZephyrStruct(hi2c);

    if(hz->serial) {
        *serialNB = hz->serial;
    }

    return hz->error;
}
