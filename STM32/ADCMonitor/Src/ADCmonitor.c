/*!
 * @file    ADCmonitor.c
 * @brief   This file contains the ADC libraries for the STM32F4
 * @date    25/08/2021
 * @author  Author: agp
*/

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#include "ADCMonitor.h"

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static uint32_t sinePeakIdx(const int16_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel, bool reverse);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static struct {
    uint32_t   length;
    int16_t   *pData;        // DMA buffer
    uint32_t   noOfChannels; // No of channels for each sample
    uint32_t   noOfSamples;  // No of Samples in each channel per interrupt, half buffer.

    activeBuffer_t activeBuffer;
} ADCMonitorData;

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Finds the index of the first peak of a sinewave
 * @param   pData Pointer to buffer
 * @param   noOfChannels Number of ADC channels
 * @param   noOfSamples Number of samples in each channel per interrupt
 * @param   channel ADC channel
 * @param   reverse To find the last peak instead of the first
 * @return  Index of the peak
*/
static uint32_t sinePeakIdx(const int16_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel, bool reverse)
{
    uint32_t begin = channel;
    uint32_t end = channel + (noOfSamples-1)* noOfChannels;

    // Sets initial search values
    uint32_t idx   = (!reverse) ? begin : end - noOfChannels;
    bool direction = (!reverse) ? pData[idx] < pData[noOfChannels + idx]
                                : pData[idx - noOfChannels] <  pData[idx];

    while (idx >= begin && idx <= end && idx != ((!reverse) ? end : begin))
    {
        int16_t next;
        int16_t current;

        if (!reverse) {
            next = pData[idx+noOfChannels];
            current = pData[idx];
        } else {
            next = pData[idx];
            current = pData[idx - noOfChannels];
        }

        bool gradient = current < next;
        if (direction != gradient)
            return (idx-channel)/noOfChannels;

        idx += (!reverse) ? noOfChannels : -noOfChannels;
    }

    // Failed, sine curve to small/not found
    uint32_t errIdx = (!reverse) ? begin : end;
    return (errIdx - channel)/noOfChannels;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   ADC Monitor initialisation function
 * @param   hadc Pointer to ADC handler
 * @param   pData Pointer to buffer
 * @param   length Buffer length
 * @note    Must be called before call to any other function
 * @note    Must only be called once
*/
void ADCMonitorInit(ADC_HandleTypeDef* hadc, int16_t *pData, uint32_t length)
{
    // Save internal data.
    ADCMonitorData.pData           = pData;
    ADCMonitorData.length          = length;
    ADCMonitorData.noOfChannels    = hadc->Init.NbrOfConversion;
    ADCMonitorData.noOfSamples     = length / (2 * hadc->Init.NbrOfConversion);

    // Write the registers
    HAL_ADC_Start_DMA(hadc, (uint32_t *) pData, length);
}

/*!
 * @brief   ADC Monitor loop function
 * @param   callback Callback function called when the buffer is full or half-full
 * @note    Must be called from from while(1) loop to get info about new buffer via ADCCallBack function
 * @note    If new buffer, callback is called, else nothing is done
*/
void ADCMonitorLoop(ADCCallBack callback)
{
    static int lastBuffer = NotAvailable;

    if (ADCMonitorData.activeBuffer != lastBuffer)
    {
        lastBuffer = ADCMonitorData.activeBuffer;
        int16_t *pData = (ADCMonitorData.activeBuffer == First)
                ? ADCMonitorData.pData : &ADCMonitorData.pData[ADCMonitorData.length / 2];
        callback(pData, ADCMonitorData.noOfChannels, ADCMonitorData.noOfSamples);
    }
}

/*!
 * @brief   Cumulative moving average on data in buffer
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
 * @param   cma Previous calculated cumulative moving average
 * @param   k Length of cumulative buffer
 * @note    Data is altered in buffer
*/
int16_t cmaAvarage(int16_t *pData, uint16_t channel, int16_t cma, int k)
{
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        // cumulative moving average
        int16_t* ptr = &pData[ADCMonitorData.noOfChannels * sampleId + channel];
        cma = cma + (*ptr - cma)/(k+1);
        *ptr = cma; // write in buffer
    }
    return cma;
}

/*!
 * @brief   RMS computation on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
*/
double ADCrms(const int16_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint64_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        const int16_t mul = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        sum += (mul * mul); // add squared values to sum
    }

    return sqrt(((double) sum) / ((double)ADCMonitorData.noOfSamples));
}

/*!
 * @brief   RMS computation on data for selected channel between specified indexes
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
 * @param   indexes Begin and end indexes
 * @note    Made to compute a true RMS of a sinusoidal signal
*/
double ADCTrueRms(const int16_t *pData, uint16_t channel, SineWave indexes)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint64_t sum = 0;
    for (uint32_t sampleId = indexes.begin; sampleId <= indexes.end; sampleId++)
    {
        const int16_t mul = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        sum += (mul * mul); // add squared values to sum
    }

    return sqrt(((double) sum) / ((double) (indexes.end - indexes.begin + 1)));
}

/*!
 * @brief   Mean computation on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
*/
double ADCMean(const int16_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint64_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        sum += pData[sampleId*ADCMonitorData.noOfChannels + channel];
    }

    return (((double) sum) / ((double) ADCMonitorData.noOfSamples));
}

/*!
 * @brief   Mean computation on data for selected channel between specified indexes
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
 * @param   indexes Begin and end indexes
 * @note    Made to compute the offset of a sinusoidal signal
*/
double ADCMeanLimited(const int16_t *pData, uint16_t channel, SineWave indexes)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels ||
        indexes.begin == indexes.end)
    {
        return 0;
    }

    uint64_t sum = 0;
    for (uint32_t sampleId = indexes.begin; sampleId <= indexes.end; sampleId++)
    {
        sum += pData[sampleId*ADCMonitorData.noOfChannels + channel];
    }

    return (((double) sum) / ((double) (indexes.end - indexes.begin + 1)));
}

/*!
 * @brief   Compute fast mean using bit shift for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
 * @param   shiftIdx Number of bit shifts. Should be N for length=2^N
 * @note    Can only be used if array length is multiple of 2
 * @note    Bit shifting is only possible on integral values meaning the returned value does not have a fractional part
*/
float ADCMeanBitShift(const int16_t *pData, uint16_t channel, uint8_t shiftIdx)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
            pData == NULL ||
            channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint32_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        sum += pData[sampleId*ADCMonitorData.noOfChannels + channel];
    }
    return (sum >> shiftIdx);
}

/*!
 * @brief   Absolute mean computation on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
*/
double ADCAbsMean(const int16_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint64_t sum = 0;
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        sum += abs(pData[sampleId*ADCMonitorData.noOfChannels + channel]);
    }

    return ( ((double) sum) / ((double) ADCMonitorData.noOfSamples) );
}

/*!
 * @brief   Maximum computation on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
*/
int16_t ADCmax(const int16_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    int16_t max = pData[channel];
    for (uint32_t sampleId = 1; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        int16_t sample = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        if (max < sample)
            max = sample;
    }
    return max;
}

/*!
 * @brief   Minimum computation on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   channel ADC channel
*/
int16_t ADCmin(const int16_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    int16_t min = pData[channel];
    for (uint32_t sampleId = 1; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        int16_t sample = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        if (min > sample)
            min = sample;
    }
    return min;
}

/*!
 * @brief   Set the specified offset on whole buffer for selected channel
 * @param   pData Pointer to buffer
 * @param   offset Offset to adjust channel
 * @param   channel ADC channel
 * @note    No check for overflow since ADC is 12 bits
 * @note    Data is altered in buffer
*/
void ADCSetOffset(int16_t* pData, int16_t offset, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return;
    }

    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        // No need to adjust for overflow since ADC is 12 bits.
        pData[sampleId*ADCMonitorData.noOfChannels + channel] += offset;
    }
}

/*!
 * @brief   Find sample start/begin of a sine curve
 * @param   pData Pointer to buffer
 * @param   noOfChannels Number of ADC channels
 * @param   noOfSamples Number of samples in each channel per interrupt
 * @param   channel ADC channel
 * @return  Start/end of sample Index (not pointer offset)
*/
SineWave sineWave(const int16_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel)
{
    SineWave result = { sinePeakIdx(pData, noOfChannels, noOfSamples, channel, false),
                        sinePeakIdx(pData, noOfChannels, noOfSamples, channel, true ) };
    return result;
}

/*!
 * @brief   Overwritten callback function for ADC buffer half-full
 * @param   hadc Pointer to ADC handler
*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    ADCMonitorData.activeBuffer = First;
}

/*!
 * @brief   Overwritten callback function for ADC buffer full
 * @param   hadc Pointer to ADC handler
*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    ADCMonitorData.activeBuffer = Second;
}
