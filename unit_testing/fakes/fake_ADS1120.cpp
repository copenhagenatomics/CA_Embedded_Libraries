/*!
** @brief Fake Interface to ADS1120 for unit testing
**
** @author Luke W
** @date   05/11/2024
*/

#include <map>

#include "fake_stm32xxxx_hal.h"
#include "fake_ADS1120.h"

/***************************************************************************************************
** PRIVATE DEFINITIONS
***************************************************************************************************/

typedef struct {
    int error;
    double chA;
    double chB;
    double internal;
} ADS1120_t;

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

std::map<ADS1120Device*, ADS1120_t*> *spi_map = nullptr;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

ADS1120_t* getAdsStruct(ADS1120Device *dev) {
    if(!spi_map) {
        spi_map = new std::map<ADS1120Device*, ADS1120_t*>();
    }

    if(!spi_map->count(dev)) {
        spi_map->insert(std::pair<ADS1120Device*, ADS1120_t*>(dev, new ADS1120_t()));
    }

    return spi_map->at(dev);
}

/*!
** @brief Sets if there is an error or not
*/
void setError(ADS1120Device *dev, int error) {
    ADS1120_t* ads = getAdsStruct(dev);
    ads->error = error;
}

void setTemp(ADS1120Device *dev, double chA, double chB, double internal) {
    ADS1120_t* ads = getAdsStruct(dev);
    ads->chA = chA;
    ads->chB = chB;
    ads->internal = internal;
}

int ADS1120Init(ADS1120Device *dev) {
    return getAdsStruct(dev)->error;
}



int ADS1120Loop(ADS1120Device *dev, float *type_calibration) {
    ADS1120_t* ads = getAdsStruct(dev);

    if(ads->error) {
        return ads->error;
    }

    dev->data.chA = ads->chA;
    dev->data.chB = ads->chB;
    dev->data.internalTemp = ads->internal;
    return 0;
}
