/*!
 * @file    ca_hanning.c
 * @brief   This file contains a function to apply a hanning windows
 * @date    11/03/2024
 * @author  Timothé D
 */

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "hanning.h"

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Initialization of hanning windows
 * @param   pDst Pointer to list of coefficients
 * @param   length Windows length
 * @note    https://en.wikipedia.org/wiki/Hann_function
 */
void hanningInit(float *pDst, uint32_t length) {
    float k = 2.0f * M_PI / ((float)length);
    for (uint32_t index = 0; index < length; index++) {
        pDst[index] = 0.5f * (1.0f - cosf(index * k));
    }
}

/*!
 * @brief   Applies hanning windows on specified channel
 * @param   hanningCoef List of hanning coefficients
 * @param   pData Pointer to ADC buffer
 * @param   noOfChannels Number of ADC channels
 * @param   noOfSamples Number of samples for each channel
 * @param   channel ADC channel
 * @note    Useful before applying a fft, to decrease spectral leakage
 * @note    ca_hanning_init() should be used to define the coefficients beforehand
 */
void hanning(float *hanningCoef, int16_t *pData, uint32_t noOfChannels, uint32_t noOfSamples,
             uint16_t channel) {
    if (pData == NULL || hanningCoef == NULL) {
        return;
    }

    for (uint32_t sampleId = 0; sampleId < noOfSamples; sampleId++) {
        pData[sampleId * noOfChannels + channel] *= hanningCoef[sampleId];
    }
}

/*!
 * @brief   Applies hanning windows while calculating the coefficients on the go
 * @param   pData Pointer to ADC buffer
 * @param   noOfChannels Number of ADC channels
 * @param   noOfSamples Number of samples for each channel
 * @param   channel ADC channel
 * @note    Useful before applying a fft, to decrease spectral leakage
 */
void hanningFloatDirect(float *pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel) {
    if (pData == NULL) {
        return;
    }

    float k = 2.0f * M_PI / ((float)noOfSamples);

    for (uint32_t sampleId = 0; sampleId < noOfSamples; sampleId++) {
        pData[sampleId * noOfChannels + channel] *= 0.5f * (1.0f - cosf(sampleId * k));
    }
}
