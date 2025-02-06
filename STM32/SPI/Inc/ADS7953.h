/*!
 * @file    ADS7953.h
 * @brief   Header file of ADS7953.c
 * @date    28/10/2024
 * @author  Timothé D
*/

#ifndef INC_ADS7953_H_
#define INC_ADS7953_H_

#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef enum {
    FirstPart,
    SecondPart
} ADS7953Buffer_t;                                  // Part of the buffer that is ready for further calculations

typedef struct
{
    SPI_HandleTypeDef *hspi;                        // Pointer to SPI handler
    DMA_HandleTypeDef *hdma_spi_rx;                 // Pointer to SPI RX DMA handler

    TIM_HandleTypeDef *htim;                        // Pointer to timer handler
    DMA_HandleTypeDef *hdma_tim_receiving;          // Pointer to timer DMA handler triggering the SPI receive
    DMA_HandleTypeDef *hdma_tim_disabling;          // Pointer to timer DMA handler responsible of pulling the NSS pin high
    DMA_HandleTypeDef *hdma_tim_enabling;           // Pointer to timer DMA handler responsible of pulling the NSS pin low

    uint8_t noOfChannels;                           // Number of ADC channels
    uint32_t noOfSamples;                           // Number of samples in each channel per interrupt, half buffer
    int16_t *buffer;                                // Pointer to the circular buffer
    uint32_t bufLength;                             // Total length of the circular buffer
    ADS7953Buffer_t lastBuffer;                     // Part of the circular buffer used for the last measurement
    ADS7953Buffer_t activeBuffer;                   // Part of the circular buffer used for the next measurement
} ADS7953Device_t;

typedef void (*extADCCallBack)(int16_t *pBuffer);   // Type of function called when the buffer is half-full or full

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

bool isBufferHealthy(ADS7953Device_t *dev, int16_t *pData);
int16_t extADCMax(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
int16_t extADCMin(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
double extADCMean(ADS7953Device_t *dev, int16_t *pData, uint16_t channel);
double extADCRms(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, uint32_t noOfPoints);
void extADCSetOffset(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, int16_t offset);

int ADS7953Init(ADS7953Device_t *dev, TIM_HandleTypeDef *htim, SPI_HandleTypeDef *hspi, int16_t *buff, uint32_t length, uint8_t noOfChannels, DMA_HandleTypeDef *hdma_spi_rx, DMA_HandleTypeDef *hdma_tim_receiving, DMA_HandleTypeDef *hdma_tim_disabling, DMA_HandleTypeDef *hdma_tim_enabling);
int ADS7953Reset(ADS7953Device_t *dev);
void ADS7953Loop(ADS7953Device_t *dev, extADCCallBack callback);

#endif /* INC_ADS7953_H_ */
