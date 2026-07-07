/*!
** @brief Fake I2C-level interface to the TCI H2 sensor for unit testing
**
** @author Timothé Dodin
** @date   07/07/2026
*/

#include "fake_tci.h"

#include "crc.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* TCI's I2C address, CRC-16/CCITT-FALSE parameters, and command IDs. None of these are exposed
** via TCI.h, so they're duplicated here - they describe the wire protocol, not TCI.c's internals. */
#define TCI_I2C_ADDR 0x36U

#define TCI_CRC_INIT 0xFFFFU
#define TCI_CRC_POLY 0x1021U

#define TCI_CMD_TRIGGER_CONC 0xA8U
#define TCI_CMD_TRIGGER_TEMP 0xA9U
#define TCI_CMD_READ_ID      0xC2U

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

static void appendCrc16(uint8_t* buf, uint16_t len) {
    initCrc16(TCI_CRC_INIT, TCI_CRC_POLY);
    uint16_t crc = crc16Calculate(buf, len);
    buf[len]     = (crc >> 8) & 0xFFU;
    buf[len + 1] = crc & 0xFFU;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

mockTci::mockTci(uint32_t id, I2C_TypeDef* i2cBus) {
    _id  = id;
    addr = TCI_I2C_ADDR << 1;
    bus  = i2cBus;
}

void mockTci::setH2(int16_t rawH2) {
    _rawH2 = rawH2;
}

void mockTci::setTemp(int8_t rawTemp) {
    _rawTemp = rawTemp;
}

void mockTci::setCommsError(bool error) {
    _commsError = error;
}

uint8_t mockTci::lastCommand() const {
    return _lastCmd;
}

HAL_StatusTypeDef mockTci::transmit(uint8_t* buf, uint8_t size) {
    if (_commsError) {
        return HAL_ERROR;
    }
    _lastCmd = buf[0];
    return HAL_OK;
}

HAL_StatusTypeDef mockTci::recv(uint8_t* buf, uint8_t size) {
    if (_commsError) {
        return HAL_ERROR;
    }

    if (_lastCmd == TCI_CMD_READ_ID && size >= 12) {
        buf[0] = 0;  // status OK
        buf[1] = (_id >> 24) & 0xFFU;
        buf[2] = (_id >> 16) & 0xFFU;
        buf[3] = (_id >> 8) & 0xFFU;
        buf[4] = (_id >> 0) & 0xFFU;
        buf[5] = buf[6] = buf[7] = buf[8] = buf[9] = 0;
        appendCrc16(buf, 10);
    }
    else if (_lastCmd == TCI_CMD_TRIGGER_CONC && size >= 5) {
        buf[0] = 0;
        buf[1] = (_rawH2 >> 8) & 0xFFU;
        buf[2] = _rawH2 & 0xFFU;
        appendCrc16(buf, 3);
    }
    else if (_lastCmd == TCI_CMD_TRIGGER_TEMP && size >= 4) {
        buf[0] = 0;
        buf[1] = (uint8_t)_rawTemp;
        appendCrc16(buf, 2);
    }

    return HAL_OK;
}
