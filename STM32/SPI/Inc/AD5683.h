#ifndef __AD5683_H
/*!
 * @file    AD5683.h
 * @brief   This file contains the function prototypes and definitions for the AD5683 DAC driver
 * @ref     https://www.analog.com/media/en/technical-documentation/data-sheets/AD5683R_5682R_5681R_5683.pdf
 * @date	26/02/2026
 * @author 	Valdemar H. Rasmussen
 */

#define __AD5683_H


#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#endif

#include <stdint.h>
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* 
// SYNC = CS (active low)
#define AD5683R_SYNC_PORT      GPIOA
#define AD5683R_SYNC_PIN       GPIO_PIN_4
#define AD5683R_RESET_PORT     GPIOA
#define AD5683R_RESET_PIN      GPIO_PIN_3
// LADC tied to GND
*/

/* Power-down mode options from Table 14 of datasheet */
typedef enum {
    AD5683R_POWER_NORMAL = 0x0,     /* Normal Operation */
    AD5683R_POWER_1K_GND = 0x1,     /* 1 kΩ to GND */
    AD5683R_POWER_100K_GND = 0x2,   /* 100 kΩ to GND */
    AD5683R_POWER_TRISTATE = 0x3     /* Three-state */
} AD5683R_PowerMode_t;

typedef struct _ad5683r {
    SPI_HandleTypeDef *hspi;    // SPI handler
    StmGpio *csPin;             // CS pin
    StmGpio *resetPin;          // RESET pin
} ad5683r_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void AD5683R_Init(ad5683r_t *dev, SPI_HandleTypeDef *hspi, StmGpio *syncPin, StmGpio *resetPin);
void AD5683R_Reset(ad5683r_t *dev);
void AD5683R_SetValue(ad5683r_t *dev, uint16_t data);
void AD5683R_PowerMode(ad5683r_t *dev, AD5683R_PowerMode_t mode);
void AD5683R_InternalRef(ad5683r_t *dev, uint8_t enable);


#endif /* __AD5683_H */