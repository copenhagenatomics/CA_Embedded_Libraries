/*!
 * @file    ADS7953.h
 * @brief   Header file of ADS7953.c
 * @date    28/10/2024
 * @author  Timothé D
 */

#ifndef INC_ADS7953_H_
#define INC_ADS7953_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Part of the buffer that is ready for further calculations
typedef enum { FirstPart, SecondPart } ADS7953Buffer_t;

typedef struct {
    DMA_HandleTypeDef *hdma_spi_rx;         // SPI RX DMA handler
    DMA_HandleTypeDef *hdma_tim_receiving;  // Timer DMA handler triggering the SPI receive
    DMA_HandleTypeDef *hdma_tim_disabling;  // Timer DMA handler to pull the NSS pin high
    DMA_HandleTypeDef *hdma_tim_enabling;   // Timer DMA handler to pull the NSS pin low
} ADS7953DMAs_t;

typedef struct {
    ADS7953DMAs_t DMAs;            // List of DMA pointers
    SPI_HandleTypeDef *hspi;       // Pointer to SPI handler
    TIM_HandleTypeDef *htim;       // Pointer to timer handler
    uint8_t noOfChannels;          // Number of ADC channels
    uint32_t noOfSamples;          // Number of samples in each channel per interrupt, half buffer
    int16_t *buffer;               // Pointer to the circular buffer
    uint32_t bufLength;            // Total length of the circular buffer
    ADS7953Buffer_t lastBuffer;    // Part of the circular buffer used for the last measurement
    ADS7953Buffer_t activeBuffer;  // Part of the circular buffer used for the next measurement
} ADS7953Device_t;

// Type of function called when the buffer is half-full or full
typedef void (*extADCCallBack)(int16_t *pBuffer);

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

bool checkAndCleanBuffer(ADS7953Device_t *dev, int16_t *pData);
int16_t extADCMax(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
int16_t extADCMin(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
double extADCMean(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
double extADCRms(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
void extADCSetOffset(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, int16_t offset);

int ADS7953Init(ADS7953Device_t *dev, SPI_HandleTypeDef *hspi, TIM_HandleTypeDef *htim,
                ADS7953DMAs_t DMAs, int16_t *buff, uint32_t length, uint8_t noOfChannels);
int ADS7953Reset(ADS7953Device_t *dev);
void ADS7953Loop(ADS7953Device_t *dev, extADCCallBack callback);

#endif /* INC_ADS7953_H_ */
