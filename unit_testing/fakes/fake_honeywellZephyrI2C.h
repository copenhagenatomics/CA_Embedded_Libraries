/*!
** @brief Fake Interface to honeywell zephyr for unit testing
**
** Extends normal honeywell zephyr interface with a few extra functions for testing
**
** @author Luke W
** @date   12/07/2024
*/

#ifndef __FAKE_HONEYWELLZEPHYRI2C_H_
#define __FAKE_HONEYWELLZEPHYRI2C_H_

#include <cstdint>

#include "fake_stm32xxxx_hal.h"

#include "honeywellZephyrI2C.h"

void setI2cError(I2C_HandleTypeDef *hi2c, HAL_StatusTypeDef error);
void setSerialNumber(I2C_HandleTypeDef *hi2c, uint16_t serial);
void setFlow(I2C_HandleTypeDef *hi2c, float flow);

#endif /* __FAKE_HONEYWELLZEPHYRI2C_H_ */