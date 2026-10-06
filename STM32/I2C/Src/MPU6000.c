/*!
 * @file    MPU6000.c
 * @brief   Minimal driver for the Invensense MPU-6000 accelerometer/gyroscope
 * @date    02/10/2026
 * @authors Luke Walker
 *
 * Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 */

#include "stm32f4xx_hal.h"

#include "MPU6000.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MPU6000_REG_WHO_AM_I 0x75

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Reads the WHO_AM_I register of the MPU-6000
**
** @param hi2c    Handle for I2C peripheral
** @param whoAmI  Pointer to store the register value read
** @return HAL status of the I2C transaction
*/
HAL_StatusTypeDef mpu6000_readWhoAmI(I2C_HandleTypeDef* hi2c, uint8_t* whoAmI) {
    return HAL_I2C_Mem_Read(hi2c, MPU6000_I2C_ADDR << 1, MPU6000_REG_WHO_AM_I,
                            I2C_MEMADD_SIZE_8BIT, whoAmI, 1, 2);
}
