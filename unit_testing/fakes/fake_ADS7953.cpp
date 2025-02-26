/*!
 * @file    fake_ADS7953.cpp
 * @brief   Fake Interface to ADS7953 for unit testing
 * @date    06/02/2025
 * @author  Timothé D
*/

#include "fake_stm32xxxx_hal.h"
#include "ADS7953.h"

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

bool checkAndCleanBuffer(ADS7953Device_t *dev, int16_t *pData) {
    return true;
}

int16_t extADCMax(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    return 0;
}

int16_t extADCMin(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    return 0;
}

double extADCMean(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    return 0.0;
}

double extADCRms(ADS7953Device_t *dev, int16_t *pData, uint16_t channel) {
    return 0.0;
}

void extADCSetOffset(ADS7953Device_t *dev, int16_t *pData, uint16_t channel, int16_t offset) {
    return;
}

int ADS7953Init(ADS7953Device_t *dev, SPI_HandleTypeDef *hspi, TIM_HandleTypeDef *htim, ADS7953DMAs_t DMAs, int16_t *buff, uint32_t length, uint8_t noOfChannels) {
    return 0;
}

int ADS7953Reset(ADS7953Device_t *dev) {
    return 0;
}

void ADS7953Loop(ADS7953Device_t *dev, extADCCallBack callback) {
    return;
}
