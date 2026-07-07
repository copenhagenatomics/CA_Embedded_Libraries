/*!
** @brief Fake I2C-level interface to the Sensirion SHT45 for unit testing
**
** Responds to SHT4X_READ_SERIAL and to any measurement/heater command with a CRC-valid 6-byte
** frame built from the raw ADC codes last set via setTemp()/setHumidity(). Does not model
** conversion delay, heater timing, or I2C errors.
**
** @author Timothé Dodin
** @date   07/07/2026
*/

#ifndef __FAKE_SHT45_H_
#define __FAKE_SHT45_H_

#include <cstdint>

#include "fake_stm32xxxx_hal.h"
#include "sht45.h"

class mockSht45 : public stm32I2cTestDevice {
   public:
    mockSht45(uint32_t serial, I2C_TypeDef* i2cBus);

    void setTemp(uint16_t rawTemp);
    void setHumidity(uint16_t rawHumidity);

    /* Last command byte written by the UUT (e.g. SHT4X_MEASURE_HIGHREP, SHT4X_HEATER_110mW_1s,
    ** SHT4X_HEATER_200mW_100ms, SHT4X_READ_SERIAL). Lets a test confirm which command was
    ** actually issued over I2C. */
    uint8_t lastCommand() const;

    HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size);
    HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size);

   private:
    uint32_t _serial;
    uint8_t _mode   = 0;
    uint16_t _temp  = 0;
    uint16_t _humid = 0;
};

#endif /* __FAKE_SHT45_H_ */
