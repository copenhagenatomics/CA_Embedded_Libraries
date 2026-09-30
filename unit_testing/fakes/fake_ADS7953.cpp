/*!
 * @file    fake_ADS7953.cpp
 * @brief   Fake Interface to ADS7953 for unit testing
 * @date    06/02/2025
 * @author  Timothé D
*/

#include <math.h>

#include "fake_stm32xxxx_hal.h"
#include "ADS7953.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Mirrors the private definitions in ADS7953.c: duplicated here because they're not exposed via
// the header, but they're just the documented error codes/limits of the real functions below.
#define MAX_CHANNELS_NO         16
#define ADS7943_OK              0
#define ADS7943_ERR_NO_CHANNELS -3

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/* 
 * Everything below, other than the real hardware setup in ADS7953Init()/ADS7953Reset(),
 * is pure computation on the message/buffer/device struct with no dependency on real hardware, so
 * it uses the same implementation as ADS7953.c
 */

/*!
 * @brief   Extraction of channel address from SPI message
 * @param   message SPI message received
 * @return  ADC channel address
 */
uint16_t getChannelAddress(uint16_t message) {
    return ((message & 0xF000U) >> 12);
}

/*!
 * @brief   Extraction of ADC value from SPI message
 * @param   message SPI message received
 * @return  12-bits ADC value
 */
uint16_t getConversionResult(uint16_t message) {
    return (message & 0x0FFFU);
}

/*!
 * @brief   Verification of ADC buffer health
 * @note    Checks that no shift in measurement happened and removes the channel info (last 4 bits)
 * @note    Must be called before using the buffer
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @return  1 if the buffer is OK
 */
bool checkAndCleanBuffer(ADS7953Device_t *dev, int16_t *pData) {
    if (pData == NULL || dev == NULL) {
        return false;
    }

    for (uint16_t channel = 0; channel < dev->noOfChannels; channel++) {
        for (uint16_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++) {
            if (getChannelAddress(pData[sampleId * dev->noOfChannels + channel]) != channel) {
                return false;
            }
            pData[sampleId * dev->noOfChannels + channel] =
                getConversionResult(pData[sampleId * dev->noOfChannels + channel]);
        }
    }

    return true;
}

/*!
 * @brief   Calculation of maximum
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Maximum of buffer for given channel
 */
int16_t extADCMax(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    if (pData == NULL || dev == NULL || channel >= dev->noOfChannels) {
        return 0;
    }

    int16_t max = pData[channel];
    for (uint16_t sampleId = 1; sampleId < dev->noOfSamples; sampleId++) {
        int16_t sample = pData[sampleId * dev->noOfChannels + channel];
        if (sample > max) {
            max = sample;
        }
    }
    return max;
}

/*!
 * @brief   Calculation of minimum
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Minimum of buffer for given channel
 */
int16_t extADCMin(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    if (pData == NULL || dev == NULL || channel >= dev->noOfChannels) {
        return 0;
    }

    int16_t min = pData[channel];
    for (uint16_t sampleId = 1; sampleId < dev->noOfSamples; sampleId++) {
        int16_t sample = pData[sampleId * dev->noOfChannels + channel];
        if (sample < min) {
            min = sample;
        }
    }
    return min;
}

/*!
 * @brief   Calculation of average
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  Mean of buffer for given channel
 */
double extADCMean(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    if (pData == NULL || dev == NULL || channel >= dev->noOfChannels) {
        return 0;
    }

    int32_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++) {
        sum += pData[sampleId * dev->noOfChannels + channel];
    }
    return (((double)sum) / ((double)dev->noOfSamples));
}

/*!
 * @brief   Calculation of RMS
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @return  RMS of buffer for given channel
 */
double extADCRms(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    if (pData == NULL || dev == NULL || channel >= dev->noOfChannels) {
        return 0;
    }

    double sum = 0;
    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++) {
        double mult = pData[sampleId * dev->noOfChannels + channel];
        sum += (mult * mult);
    }
    return sqrt(sum / ((double)dev->noOfSamples));
}

/*!
 * @brief   Application of offset
 * @param   dev Pointer to the ADC structure
 * @param   pData Pointer to the ADC buffer
 * @param   channel ADC channel
 * @param   offset Offset
 */
void extADCSetOffset(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, int16_t offset) {
    if (pData == NULL || dev == NULL || channel >= dev->noOfChannels) {
        return;
    }

    for (uint32_t sampleId = 0; sampleId < dev->noOfSamples; sampleId++) {
        pData[sampleId * dev->noOfChannels + channel] += offset;
    }
}

/*!
 * @brief   Configuration of an ADS7953 device
 * @note    Populates the device struct the same way the real ADS7953Init() does
 * @param   dev Pointer to the ADC structure
 * @param   hspi Pointer to the SPI handler
 * @param   htim Pointer to the timer used for DMA
 * @param   DMAs List of DMA pointers
 * @param   buff Pointer to the ADC buffer
 * @param   length Buffer length
 * @param   noOfChannels Number of inputs used
 * @return  0 on success, else negative value
 */
int ADS7953Init(ADS7953Device_t *dev, SPI_HandleTypeDef *hspi, TIM_HandleTypeDef *htim,
                ADS7953DMAs_t DMAs, int16_t *buff, uint32_t length, uint8_t noOfChannels,
                extADCCallBack callback) {
    if ((noOfChannels > MAX_CHANNELS_NO) || (noOfChannels < 1)) {
        return ADS7943_ERR_NO_CHANNELS;
    }

    dev->cb = callback;

    dev->hspi = hspi;
    dev->htim = htim;
    dev->DMAs = DMAs;

    dev->buffer       = buff;
    dev->bufLength    = length;
    dev->noOfChannels = noOfChannels;
    dev->noOfSamples  = length / (2 * noOfChannels);
    // Second part as the (real) DMA would start by filling the first part
    dev->lastBuffer = SecondPart;
    // Same as last buffer so that the callback function isn't called now
    dev->activeBuffer = SecondPart;

    return ADS7943_OK;
}

/*!
 * @brief   Resets the ADC
 * @note    ADS7953Init must be called before this function
 * @param   dev Pointer to the ADC structure
 */
int ADS7953Reset(ADS7953Device_t *dev) {
    // Resets initial buffer state, same as the real driver
    // The timer/DMA/SPI register access that surrounds it on real hardware is skipped
    dev->lastBuffer   = SecondPart;
    dev->activeBuffer = SecondPart;

    return ADS7943_OK;
}

/*!
 * @brief   Loop to update the ADC buffer
 * @note    Called in Loop function
 * @note    ADS7953Init must be called before this function
 * @param   dev Pointer to the ADC structure
 * @param   callback Callback function to use the ADC values
 */
void ADS7953Loop(ADS7953Device_t *dev, extADCCallBack callback) {
    // If the buffer is half-full or full
    if (dev->activeBuffer != dev->lastBuffer) {
        dev->lastBuffer = dev->activeBuffer;
        int16_t *pData  = NULL;

        if (dev->activeBuffer == FirstPart) {
            pData = dev->buffer;
        }
        else {
            pData = &dev->buffer[dev->bufLength / 2];
        }
        callback(pData);
    }
}
