/*
 *  Interface for external ADC LTC2512-24
 *  Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/251224fa.pdf
 * 
 *  author: matias
 *  created: 25/07/2023
 */

#ifndef INC_LTC2512_24_H_
#define INC_LTC2512_24_H_

#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#elif defined(STM32H753xx)
#include "stm32h7xx_hal.h"
#endif
#include <StmGpio.h>

// Configuration word response designating the downsampling factor
#define DF_4_WA 0x26
#define DF_8_WA 0x36
#define DF_16_WA 0x46
#define DF_32_WA 0x56

// Downsampling factor selector values
// Down-sampling factors of 4, 8, 16 and 32 are selected for [SEL1 SEL0]
// combinations of 00, 01, 10 and 11 respectively.
// Datasheet page 11.
#define DF_4_SELECT 0x00
#define DF_8_SELECT 0x01
#define DF_16_SELECT 0x02
#define DF_32_SELECT 0x03

#define DF_32_GROUP_DELAY 17

typedef struct LTC2512Device {
    SPI_HandleTypeDef* hspia; // Pointer to SPI interface on channel a. (Filtered output)
    SPI_HandleTypeDef* hspib; // Pointer to SPI interface on channel b. (Direct output) 

    StmGpio SEL0;            // GPIO output. Determines downsampling factor with SEL1. [SEL1, SEL0] 
    StmGpio SEL1;            // GPIO output. Determines downsampling factor with SEL0.

    StmGpio RDLA;            // GPIO output. When low SDOA pin is enabled. 
    StmGpio RDLB;            // GPIO output. When low SDOB pin is enabled.

    StmGpio MLCK;            // GPIO output. A rising edge initiates a new conversion.
    StmGpio SYNC;            // GPIO output. A pulse synchronises the phase of the digital filter.

    StmGpio DRL;             // GPIO input. A falling edge indicates new data ready on SDOA (part of hspi handle).
    StmGpio BUSY;            // GPIO input. High upon new conversion, low when conversion is done.
} LTC2512Device;

// Read the down-sampling factor stored in the LTC2512 Device.
uint8_t getDownsamplingFactor(LTC2512Device *dev);
int32_t transform2sComplement(int32_t value, const int MODULO, const int MAX_VALUE);

// Measure filtered output
int measureChannelA_IT(LTC2512Device *dev, uint8_t *buffer);
int measureChannelA(LTC2512Device *dev);
void measureChannelB(LTC2512Device *dev, int32_t* differential, uint32_t* common);
void syncConversion(LTC2512Device *dev);
void initiateConversion(LTC2512Device *dev);

void enableDisableChannels(LTC2512Device *dev, int channel, bool onOff);

// Initialise the external ADC LTC2512 Device.
int LTC2512Init(LTC2512Device *dev, uint8_t downsampleFactor);

#endif /* INC_LTC2512_24_H_ */