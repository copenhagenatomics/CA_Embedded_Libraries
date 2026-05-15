/*!
 * @file    TCI.c
 * @brief   Driver file for TCI H2 sensor
 * @date    22/04/2026
 * @author  Timothé Dodin
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "TCI.h"
#include "crc.h"
#include "stm32f4xx_hal.h"
#include "time32.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define I2C_ADDRESS            0x36
#define WRONG_TEMP             10000.0f  // [degC]
#define WRONG_H2               -1.0f     // [ppm]
#define I2C_TIMEOUT            1         // [ms]
#define MAX_I2C_MESSAGE_LENGTH 12

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int sendMessage(tci_t *dev, uint8_t *message, uint16_t len);
static int receiveMessage(tci_t *dev, uint8_t *message, uint16_t len);
static int triggerConcentrationMeas(tci_t *dev, float relHumidity, float temperature,
                                    float pressure);
static int receiveConcentrationMeas(tci_t *dev);
static int triggerTemperatureMeas(tci_t *dev);
static int receiveTemperatureMeas(tci_t *dev);
static int triggerReadID(tci_t *dev);
static int receiveID(tci_t *dev);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Sends message with CRC
 * @param  dev H2 device
 * @param  message Message address
 * @param  len Number of bytes
 * @return 0 if OK, else < 0
 */
static int sendMessage(tci_t *dev, uint8_t *message, uint16_t len) {
    if (len + 2 > MAX_I2C_MESSAGE_LENGTH) {
        return -1;
    }

    uint16_t crc = crc16Calculate(message, len);
    uint8_t messageWithCrc[MAX_I2C_MESSAGE_LENGTH];
    memcpy(messageWithCrc, message, len);

    messageWithCrc[len]     = (crc >> 8) & 0xFF;
    messageWithCrc[len + 1] = crc & 0xFF;

    if (HAL_I2C_Master_Transmit(dev->hi2c, I2C_ADDRESS << 1, messageWithCrc, len + 2,
                                I2C_TIMEOUT) != HAL_OK) {
        return -2;
    }
    return 0;
}

/*!
 * @brief  Reads a message and verify CRC
 * @param  dev H2 device
 * @param  message Message address
 * @param  len Number of bytes
 * @return 0 if OK, else < 0
 */
static int receiveMessage(tci_t *dev, uint8_t *message, uint16_t len) {
    if (len + 2 > MAX_I2C_MESSAGE_LENGTH) {
        return -1;
    }

    uint8_t received[MAX_I2C_MESSAGE_LENGTH];
    if (HAL_I2C_Master_Receive(dev->hi2c, I2C_ADDRESS << 1, received, len + 2, I2C_TIMEOUT) !=
        HAL_OK) {
        return -2;
    }
    if (crc16Calculate(received, len) !=
        (((uint16_t)received[len] << 8) | ((uint16_t)received[len + 1]))) {
        return -3;
    }

    memcpy(message, received, len);
    return 0;
}

/*!
 * @brief  Sends trigger concentration measurement command
 * @param  dev H2 device
 * @param  relHumidity Relative humidity in %
 * @param  temperature Temperature of humidity sensor in degC
 * @param  pressure Pressure in bar
 * @return 0 if OK, else < 0
 */
static int triggerConcentrationMeas(tci_t *dev, float relHumidity, float temperature,
                                    float pressure) {
    static const uint8_t TRIG_CONC_MEAS_COMMAND_ID = 0xA8;
    /*
    0.25% relative humidity provided
    Field contamination check enabled
    Fully compensated concentration is provided
    */
    static const uint8_t TRIG_CONC_MEAS_COMMAND_CONFIG = 0b01100000;
    static const float HUMIDITY_RES                    = 0.25;  // [%]
    static const float BAR_TO_KPA                      = 100.0;
    static const uint8_t MIN_REL_HUM_BYTE              = 0;
    static const uint8_t MAX_REL_HUM_BYTE              = 255;
    static const int8_t MIN_TEMP_BYTE                  = -40;
    static const int8_t MAX_TEMP_BYTE                  = 105;
    static const uint8_t MIN_PRES_BYTE                 = 50;
    static const uint8_t MAX_PRES_BYTE                 = 130;

    uint8_t command[5];
    command[0] = TRIG_CONC_MEAS_COMMAND_ID;
    command[1] = TRIG_CONC_MEAS_COMMAND_CONFIG;

    // Float to integer
    int32_t relHumidityInt = (int32_t)roundf(relHumidity / HUMIDITY_RES);
    int32_t temperatureInt = (int32_t)roundf(temperature);
    int32_t pressureInt    = (int32_t)roundf(pressure * BAR_TO_KPA);

    // Clamping
    command[2] =
        (uint8_t)((relHumidityInt < MIN_REL_HUM_BYTE)
                      ? MIN_REL_HUM_BYTE
                      : ((relHumidityInt > MAX_REL_HUM_BYTE) ? MAX_REL_HUM_BYTE : relHumidityInt));

    command[3] =
        (int8_t)((temperatureInt < MIN_TEMP_BYTE)
                     ? MIN_TEMP_BYTE
                     : ((temperatureInt > MAX_TEMP_BYTE) ? MAX_TEMP_BYTE : temperatureInt));

    command[4] = (uint8_t)((pressureInt < MIN_PRES_BYTE)
                               ? MIN_PRES_BYTE
                               : ((pressureInt > MAX_PRES_BYTE) ? MAX_PRES_BYTE : pressureInt));

    return sendMessage(dev, command, 5);
}

/*!
 * @brief  Reads H2 concentration
 * @note   triggerConcentrationMeas() must have been called at least 30 ms before
 * @param  dev H2 device
 * @return 0 if OK, else < 0
 */
static int receiveConcentrationMeas(tci_t *dev) {
    static const float H2_RES = 0.01;  // [%]

    uint8_t message[3] = {0};
    if (receiveMessage(dev, message, 3) != 0) {
        return -1;
    }

    // Status
    if (message[0] != 0) {
        return -2;
    }

    dev->data.H2      = (int16_t)(((uint16_t)message[1] << 8) | ((uint16_t)message[2])) * H2_RES;
    dev->lastMeasTime = HAL_GetTick();

    return 0;
}

/*!
 * @brief  Sends trigger temperature measurement command
 * @param  dev H2 device
 * @return 0 if OK, else < 0
 */
static int triggerTemperatureMeas(tci_t *dev) {
    static const uint8_t TRIG_TEMP_MEAS_COMMAND_ID = 0xA9;
    return sendMessage(dev, (uint8_t *)&TRIG_TEMP_MEAS_COMMAND_ID, 1);
}

/*!
 * @brief  Reads temperature
 * @note   triggerTemperatureMeas() must have been called at least 1 ms before
 * @param  dev H2 device
 * @return 0 if OK, else < 0
 */
static int receiveTemperatureMeas(tci_t *dev) {
    uint8_t message[2] = {0};
    if (receiveMessage(dev, message, 2) != 0) {
        return -1;
    }

    // Status
    if (message[0] != 0) {
        return -2;
    }

    dev->data.temperature = (float)((int8_t)message[1]);

    return 0;
}

/*!
 * @brief  Sends trigger read ID command
 * @param  dev H2 device
 * @return 0 if OK, else < 0
 */
static int triggerReadID(tci_t *dev) {
    static const uint8_t READ_ID_COMMAND_ID = 0xC2;
    return sendMessage(dev, (uint8_t *)&READ_ID_COMMAND_ID, 1);
}

/*!
 * @brief  Reads sensor ID
 * @note   triggerReadID() must have been called at least 0.1 ms before
 * @param  dev H2 device
 * @return 0 if OK, else < 0
 */
static int receiveID(tci_t *dev) {
    uint8_t message[10] = {0};
    if (receiveMessage(dev, message, 10) != 0) {
        return -1;
    }

    // Status
    if (message[0] != 0) {
        return -2;
    }

    dev->id = ((uint32_t)message[1] << 24) | ((uint32_t)message[2] << 16) |
              ((uint32_t)message[3] << 8) | (uint32_t)message[4];

    return 0;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief  Initializes H2 sensor
 * @param  dev TCI device
 * @param  hi2c I2C handler
 * @return 0 if OK, else < 0
 */
int tci_init(tci_t *dev, I2C_HandleTypeDef *hi2c) {
    dev->hi2c             = hi2c;
    dev->data.temperature = WRONG_TEMP;
    dev->data.H2          = WRONG_H2;
    dev->id               = 0;
    dev->lastMeasTime     = HAL_GetTick();
    dev->error            = true;
    dev->state            = WAIT_BEFORE_H2;

    initCrc16(0xFFFF, 0x1021);  // CRC-16/CCITT-FALSE

    if (triggerReadID(dev) != 0) {
        return -1;
    }
    HAL_Delay(1);
    if (receiveID(dev) != 0) {
        return -2;
    }

    dev->error               = false;
    uint32_t now             = HAL_GetTick();
    dev->lastStateChangeTime = now;
    dev->lastMeasTime        = now;
    return 0;
}

/*!
 * @brief  H2 sensor loop (state machine)
 * @param  relHumidity Relative humidity in %
 * @param  temperature Temperature of humidity sensor in degC
 * @param  pressure Pressure in bar
 * @return 0 if OK, else < 0
 */
int tci_loop(tci_t *dev, float relHumidity, float temperature, float pressure) {
    static const uint32_t TIMEOUT = 550;  // [ms] - Time out between measurements
    static const uint32_t STATE_TIMES[NO_OF_TCI_STATES] = {60, 60, 60, 60};  // [ms] - 10 Hz cycle

    uint32_t now = HAL_GetTick();

    if (tdiff_u32(now, dev->lastStateChangeTime) >= STATE_TIMES[dev->state]) {
        // State machine
        switch (dev->state) {
            case WAIT_BEFORE_H2:  // Sensor needs a bit a time between each command
                triggerConcentrationMeas(dev, relHumidity, temperature, pressure);
                dev->lastStateChangeTime = now;
                dev->state               = H2_MEAS;
                break;

            case H2_MEAS:  // Reads H2 measurement after acquisition period
                if (receiveConcentrationMeas(dev) == 0) {
                    dev->lastMeasTime = now;
                    dev->error        = false;
                }
                dev->lastStateChangeTime = now;
                dev->state               = WAIT_BEFORE_TEMP;
                break;

            case WAIT_BEFORE_TEMP:  // Sensor needs a bit a time between each command
                triggerTemperatureMeas(dev);
                dev->lastStateChangeTime = now;
                dev->state               = TEMP_MEAS;
                break;

            case TEMP_MEAS:  // Reads temperature measurement after acquisition period
                receiveTemperatureMeas(dev);
                dev->lastStateChangeTime = now;
                dev->state               = WAIT_BEFORE_H2;
                break;

            default:
                // Should never happen
                dev->error = true;
                break;
        }
    }

    // Time out
    if (tdiff_u32(now, dev->lastMeasTime) > TIMEOUT) {
        dev->data.temperature = WRONG_TEMP;
        dev->data.H2          = WRONG_H2;
        dev->error            = true;
    }

    return 0;
}
