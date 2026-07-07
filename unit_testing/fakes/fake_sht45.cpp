/*!
** @brief Fake I2C-level interface to the Sensirion SHT45 for unit testing
**
** @author Timothé Dodin
** @date   07/07/2026
*/

#include "fake_sht45.h"

#include "crc.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* SHT45's I2C CRC-8 parameters (Sensirion datasheet section 4.4). Private #defines in sht45.c
** aren't exposed via sht45.h, so they're duplicated here - they describe the wire protocol, not
** sht45.c's internals. */
#define SHT45_CRC_INIT 0xFFU
#define SHT45_CRC_POLY 0x31U

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

mockSht45::mockSht45(uint32_t serial, I2C_TypeDef* i2cBus) {
    _serial = serial;
    addr    = SHT45_I2C_ADDR << 1;
    bus     = i2cBus;
}

void mockSht45::setTemp(uint16_t rawTemp) {
    _temp = rawTemp;
}

void mockSht45::setHumidity(uint16_t rawHumidity) {
    _humid = rawHumidity;
}

uint8_t mockSht45::lastCommand() const {
    return _mode;
}

HAL_StatusTypeDef mockSht45::transmit(uint8_t* buf, uint8_t size) {
    _mode = buf[0];
    return HAL_OK;
}

HAL_StatusTypeDef mockSht45::recv(uint8_t* buf, uint8_t size) {
    if (size >= 6) {
        if (_mode == SHT4X_READ_SERIAL) {
            buf[0] = (_serial >> 24) & 0xFFU;
            buf[1] = (_serial >> 16) & 0xFFU;
            buf[3] = (_serial >> 8) & 0xFFU;
            buf[4] = (_serial >> 0) & 0xFFU;
        }
        else {
            buf[0] = (_temp >> 8) & 0xFFU;
            buf[1] = (_temp >> 0) & 0xFFU;
            buf[3] = (_humid >> 8) & 0xFFU;
            buf[4] = (_humid >> 0) & 0xFFU;
        }

        initCrc8(SHT45_CRC_INIT, SHT45_CRC_POLY);
        buf[2] = crc8Calculate(buf + 0, 2);
        buf[5] = crc8Calculate(buf + 3, 2);
    }

    return HAL_OK;
}
