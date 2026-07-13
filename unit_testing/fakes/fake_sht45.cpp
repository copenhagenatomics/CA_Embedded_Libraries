/*!
** @brief  Fake I2C-level interface to the Sensirion SHT45 for unit testing
** @author Timothé Dodin
** @date   07/07/2026
** @ref    https://sensirion.com/media/documents/33FD6951/67EB9032/HT_DS_Datasheet_SHT4x_5.pdf
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
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

static void prepareSht45Word(uint8_t* out, uint16_t word) {
    out[0] = (word >> 8) & 0xFF;
    out[1] = word & 0xFF;
    initCrc8(SHT45_CRC_INIT, SHT45_CRC_POLY);
    out[2] = crc8Calculate(out, 2);
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

mockSht45::mockSht45(uint32_t serial, I2C_TypeDef* i2cBus) {
    _serial = serial;
    addr    = SHT45_I2C_ADDR << 1;
    bus     = i2cBus;
}

void mockSht45::setTemp(float rawTempDegC) {
    _temp = (uint16_t)((rawTempDegC + 45.0f) * 65535.0f / 175.0f);
}

void mockSht45::setHumidity(float rawHumidityPct) {
    _humid = (uint16_t)((rawHumidityPct + 6.0f) * 65535.0f / 125.0f);
}

uint8_t mockSht45::lastCommand() const {
    return _mode;
}

// Sending data to mockSht45
HAL_StatusTypeDef mockSht45::transmit(uint8_t* buf, uint8_t size) {
    _mode = buf[0];
    return HAL_OK;
}

// Receiving data from mockSht45
HAL_StatusTypeDef mockSht45::recv(uint8_t* buf, uint8_t size) {
    if (size >= 6) {
        if (_mode == SHT4X_READ_SERIAL) {
            prepareSht45Word(&buf[0], (_serial >> 16) & 0xFFFFU);
            prepareSht45Word(&buf[3], _serial & 0xFFFFU);
        }
        else {
            prepareSht45Word(&buf[0], _temp);
            prepareSht45Word(&buf[3], _humid);
        }
    }

    return HAL_OK;
}

