/*!
 * @file    MCP4531.h
 * @brief   See MCP4531.c for details
 * @date    28/07/2025
 * @authors Luke Walker
 */

#ifndef MCP4531_H_
#define MCP4531_H_

#include <stdint.h>

#include "stm32f4xx_hal.h" // Adjust this include to your MCU

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct {
    uint8_t i2c_addr; /* 7-bit I2C address */
    I2C_HandleTypeDef* hi2c; /* Pointer to STM I2C peripheral */
    uint8_t num_bits; /* Number of bits of the device (7/8) */
    uint16_t max_value; /* Maximum value of the device wiper (corresponds to number of bits) */
    uint8_t device_num; /* Sub-circuit on device - 0 or 1. Use 0 for single circuit devices */
} mcp4531_handle_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int mcp4531_init(mcp4531_handle_t* handle, I2C_HandleTypeDef* hi2c, uint8_t i2c_address, uint8_t num_bits, uint8_t device_num);
int mcp4531_setWiperPos(mcp4531_handle_t* handle, uint16_t wiper_position);
int mcp4531_getWiperPos(mcp4531_handle_t* handle, uint16_t* wiper_position);

#endif // MCP4531_H_