#include "LTC2512-24.h"
#include <stdbool.h>

static uint8_t RxBufferChA[4] = {0};
static uint8_t RxBufferChB[3] = {0};

// The LTC2512-24 ADC outputs 24 bit ADC values in 2's complement
// format. To format this correctly in a int32_t variable a transform
// is necessary
const int MODULO = (1 << 24);
const int MAX_VALUE = ((1 << 23) - 1);
static int32_t transform(int32_t value) 
{
    if (value > MAX_VALUE) {
        value -= MODULO;
    }
    return value;
}

// Set the down-sampling factor by setting [SEL1, SEL0]
void setDownsamplingFactor(LTC2512Device *dev, uint8_t df)
{
    stmSetGpio(dev->SEL0, df & 0x01);
    stmSetGpio(dev->SEL1, ((df & 0x02) >> 1));
}

void enableDisableChannels(LTC2512Device *dev, int channel, bool onOff)
{
    // The SDOA and SDOB pins are enabled when the RDLA and RDLB pins are pulled low,
    // and in high impedance state when the RDLA and RDLB pins are high.
    if (channel == 0)
    {
        stmSetGpio(dev->RDLA, !onOff);
    }   
    else if (channel == 1)
    {
        stmSetGpio(dev->RDLB, !onOff);
    }
}

int measureChannelA(LTC2512Device *dev)
{
    if (HAL_SPI_Receive(dev->hspia, RxBufferChA, 4, 1) != HAL_OK)
        return 0xFFFFFFFF;

    int32_t adc = (RxBufferChA[0] << 16) | (RxBufferChA[1] << 8) | RxBufferChA[2];

    for (int i = 0; i < sizeof(RxBufferChA)/sizeof(RxBufferChA[0]); i++)
        RxBufferChA[i] = 0;

    return transform(adc);
}

int measureChannelB(LTC2512Device *dev)
{   
    uint8_t rData[3];
    if (HAL_SPI_Receive(dev->hspib, rData, 3, 1) != HAL_OK)
        return 0xFFFFFFFF;

    int32_t adc = (((RxBufferChB[0] << 8) | RxBufferChB[1]) >> 2) & 0x3FFF;
    for (int i = 0; i < sizeof(RxBufferChB)/sizeof(RxBufferChB[0]); i++)
        RxBufferChB[i] = 0;
    return adc;
}

void syncConversion(LTC2512Device *dev)
{
    // Rising edge syncs conversion period for filtered output
    stmSetGpio(dev->SYNC, true);
    __NOP();
    __NOP();
    stmSetGpio(dev->SYNC, false);
}

/* Toggle MCLK to initiate new conversion
 * Ideally, the MLCK should be driven by a PWM pin,
 * but if that is not available it can be driven by a GPIO
 */
void initiateConversion(LTC2512Device *dev)
{
    // Rising edge initiates a conversion
    stmSetGpio(dev->MLCK, true);
    __NOP();
    __NOP();
    // Falling edge should occur within 40ns for best performance. 
    // Assuming __NOP() takes one clock cycle (~2.2ns) it should be able to react.
    stmSetGpio(dev->MLCK, false);
}

int LTC2512Init(LTC2512Device *dev, uint8_t downsampleFactor)
{
    // Set the desired downsample factor
    setDownsamplingFactor(dev, downsampleFactor);
    return 0;
}