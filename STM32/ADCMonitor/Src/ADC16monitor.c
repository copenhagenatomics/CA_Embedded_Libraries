/*
 * ADCmonitor.c
 *
 *  Created on: 25 Aug 2021
 *      Author: agp
 *  Description:
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <math.h>
#include <ADC16Monitor.h>

typedef enum {
    NotAvailable,
    First,
    Second
} activeBuffer_t;

static struct {
    uint32_t   length;
    int32_t   *pData;        // DMA buffer
    uint32_t   noOfChannels; // No of channels for each sample
    uint32_t   noOfSamples;  // No of Samples in each channel per interrupt, half buffer.

    activeBuffer_t activeBuffer;
} ADCMonitorData;

void ADCMonitorInit(ADC_HandleTypeDef* hadc, int32_t *pData, uint32_t length)
{
    // Save internal data.
    ADCMonitorData.pData           = pData;
    ADCMonitorData.length          = length;
    ADCMonitorData.noOfChannels    = hadc->Init.NbrOfConversion;
    ADCMonitorData.noOfSamples     = length / (2 * hadc->Init.NbrOfConversion);

    // Write the registers
    HAL_ADC_Start_DMA(hadc, (uint32_t *) pData, length);
}

int ADCCalibrationInit(ADC_HandleTypeDef* hadc, uint32_t CalibrationMode, uint32_t SingleDiff)
{
    if (HAL_ADCEx_Calibration_Start(hadc, CalibrationMode, SingleDiff) != HAL_OK)
    {
        return 0;
    }
    return 1;
}

int32_t cmaAvarage(int32_t *pData, uint16_t channel, int32_t cma, int k)
{
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        // cumulative moving average
        int32_t* ptr = &pData[ADCMonitorData.noOfChannels * sampleId + channel];
        cma = cma + (*ptr - cma)/(k+1);
        *ptr = cma; // write in buffer
    }
    return cma;
}

void ADCMonitorLoop(ADCCallBack callback)
{
    static int lastBuffer = NotAvailable;

    if (ADCMonitorData.activeBuffer != lastBuffer)
    {
        lastBuffer = ADCMonitorData.activeBuffer;
        int32_t *pData = (ADCMonitorData.activeBuffer == First)
                ? ADCMonitorData.pData : &ADCMonitorData.pData[ADCMonitorData.length / 2];
        callback(pData, ADCMonitorData.noOfChannels, ADCMonitorData.noOfSamples);
    }
}

double ADCrms(const int32_t *pData, uint16_t channel)
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
        const int32_t mul = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        sum += (mul * mul); // add squared values to sum
    }

    return sqrt(((double) sum) / ((double)ADCMonitorData.noOfSamples));
}

double ADCMean(const int32_t *pData, uint16_t channel)
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

float ADCMeanBitShift(const int32_t *pData, uint16_t channel, uint8_t shiftIdx)
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

double ADCAbsMean(const int32_t *pData, uint16_t channel)
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

    return (sum / ADCMonitorData.noOfSamples);
}

uint32_t ADCmax(const int32_t *pData, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return 0;
    }

    uint32_t max = 0;
    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        int32_t sample = pData[sampleId*ADCMonitorData.noOfChannels + channel];
        if (max < sample)
            max = sample;
    }
    return max;
}

void ADCSetOffset(int32_t* pData, int16_t offset, uint16_t channel)
{
    if (ADCMonitorData.activeBuffer == NotAvailable ||
        pData == NULL ||
        channel >= ADCMonitorData.noOfChannels)
    {
        return;
    }

    for (uint32_t sampleId = 0; sampleId < ADCMonitorData.noOfSamples; sampleId++)
    {
        // No need to addjust for overflow since ADC is 12 bits.
        pData[sampleId*ADCMonitorData.noOfChannels + channel] += offset;
    }
}

static uint32_t sinePeakIdx(const int32_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel, bool reverse)
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

SineWave sineWave(const int32_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel)
{
    SineWave result = { sinePeakIdx(pData, noOfChannels, noOfSamples, channel, false),
                        sinePeakIdx(pData, noOfChannels, noOfSamples, channel, true ) };
    return result;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    ADCMonitorData.activeBuffer = First;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    ADCMonitorData.activeBuffer = Second;
}
