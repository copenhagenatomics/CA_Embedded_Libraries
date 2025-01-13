/*!
 * @file    ADCmonitor.h
 * @brief   Header file of ADCmonitor.c
 * @date    25/08/2021
 * @author  Author: agp
*/

#ifndef _ADCMONITOR_H_
#define _ADCMONITOR_H_

#include <stdint.h>

#ifndef UNIT_TESTING
    #include "stm32f4xx_hal.h"
#else
    #include "fake_stm32xxxx_hal.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** STRUCTURES
***************************************************************************************************/

typedef enum {
    NotAvailable,
    First,
    Second
} activeBuffer_t;

typedef struct SineWave {
    uint32_t begin; // Index where the sinewave begins
    uint32_t end;   // Index where the sinewave ends
} SineWave;

/*
 * Callback function from ADCMonitorLoop.
 * The format of the buffer is [ CH0{s0}, CH1{s0},,,, CHN{s0},
 *                               CH0{s1}, CH1{s1},,,, CHN{s1},
 *                               ...........
 *                               CH0{sM}, CH2{sM},,,, CHN{SN} ], N,M =[0:inf]
 * Since this modules does not has a fixed format the buffer is a one dimensional array and
 * each sample is fetched using pData[SampleNo * noOfChannls + channelNumber]
*/
typedef void (*ADCCallBack)(int16_t *pBuffer, int noOfChannels, int noOfSamples);

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void ADCMonitorInit(ADC_HandleTypeDef* hadc, int16_t *pData, uint32_t length);
void ADCMonitorLoop(ADCCallBack callback);

int16_t cmaAvarage(int16_t *pData, uint16_t channel, int16_t cma, int k);
double ADCrms(const int16_t *pData, uint16_t channel);
double ADCTrueRms(const int16_t *pData, uint16_t channel, SineWave indexes);
double ADCMean(const int16_t *pData, uint16_t channel);
double ADCMeanLimited(const int16_t *pData, uint16_t channel, SineWave indexes);
float ADCMeanBitShift(const int16_t *pData, uint16_t channel, uint8_t shiftIdx);
double ADCAbsMean(const int16_t *pData, uint16_t channel);
int16_t ADCmax(const int16_t *pData, uint16_t channel);
int16_t ADCmin(const int16_t *pData, uint16_t channel);
void ADCSetOffset(int16_t* pData, int16_t offset, uint16_t channel);
SineWave sineWave(const int16_t* pData, uint32_t noOfChannels, uint32_t noOfSamples, uint16_t channel);

#ifdef __cplusplus
}
#endif

#endif // _ADCMONITOR_H_
