/*!
 * @file    MPU6000.h
 * @brief   See MPU6000.c for details
 * @date    02/10/2026
 * @authors Luke Walker
 */

#ifndef MPU6000_H_
#define MPU6000_H_

#include <stdint.h>

#include "stm32f4xx_hal.h" // Adjust this include to your MCU

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MPU6000_I2C_ADDR 0x68 /* 7-bit I2C address, assumes AD0 is tied low */

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

HAL_StatusTypeDef mpu6000_readWhoAmI(I2C_HandleTypeDef* hi2c, uint8_t* whoAmI);

#endif // MPU6000_H_
