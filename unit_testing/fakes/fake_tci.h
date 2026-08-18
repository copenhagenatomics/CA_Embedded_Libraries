/*!
** @brief Fake I2C-level interface to the TCI H2 sensor for unit testing
**
** Responds to the read-ID, trigger-concentration, and trigger-temperature commands with a
** CRC-16/CCITT-FALSE-valid frame built from the raw counts last set via setH2()/setTemp(), or
** with an I2C failure if setCommsError() is active. Does not model conversion delay beyond
** replying immediately to whichever command was last transmitted.
**
** @author Timothé Dodin
** @date   07/07/2026
*/

#ifndef __FAKE_TCI_H_
#define __FAKE_TCI_H_

#include <cstdint>

#include "fake_stm32xxxx_hal.h"
#include "TCI.h"

class mockTci : public stm32I2cTestDevice {
   public:
    mockTci(uint32_t id, I2C_TypeDef* i2cBus);

    void setH2(float rawH2ppm);      // raw counts; TCI.c converts H2 = rawH2 * 100 ppm
    void setTemp(float rawTempDegC);   // raw counts; TCI.c converts 1:1 to degC
    void setCommsError(bool error);  // make transmit()/recv() fail, simulating a dead sensor

    /* Last command ID byte written by the UUT (e.g. 0xA8 trigger concentration, 0xA9 trigger
    ** temperature, 0xC2 read ID). Lets a test confirm which command was actually issued. */
    uint8_t lastCommand() const;

    HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size);
    HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size);

   private:
    uint32_t _id;
    uint8_t _lastCmd    = 0;
    int16_t _rawH2      = 0;
    int8_t _rawTemp     = 0;
    bool _commsError    = false;
};

#endif /* __FAKE_TCI_H_ */
