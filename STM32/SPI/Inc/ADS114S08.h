/*!
 * @file    ADS114S08.h
 * @brief   This file contains the function prototypes and definitions for the ADS114S08 ADC driver
 * @ref     https://www.ti.com/lit/ds/symlink/ads114s08b.pdf
 * @date	26/02/2026
 * @author 	Valdemar H. Rasmussen
 */

#ifndef __ADS114S08_H
#define __ADS114S08_H

#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>
#include <stdbool.h>
#include "StmGpio.h"


/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct _ads114s08 {
    SPI_HandleTypeDef *hspi;    // SPI handler
    StmGpio *csPin;              // Chip select pin
    StmGpio *resetPin;           // Reset pin
    StmGpio *drdyPin;            // Data ready pin
    StmGpio *startSyncPin;       // Start/Sync pin (not used in current implementation, but defined for completeness)
} ads114s08_t;


/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void ADS114_Init(ads114s08_t *dev, SPI_HandleTypeDef *hspi, StmGpio *csPin, StmGpio *resetPin, StmGpio *drdyPin, StmGpio *startSyncPin);
uint16_t ADS114_ReadChannelSingleShot(ads114s08_t *dev, uint8_t positive_ain);

#endif // __ADS114S08_H