/*!
** @file    fake_stm32xxxx_hal.h
** @author  Luke W
** @date    12/10/2023
**/

/* Prevent inclusion of real HALs, as well as re-inclusion of this one */
#ifndef __STM32xxxx_HAL_H
#define __STM32xxxx_HAL_H
#define __STM32F4xx_HAL_H
#define STM32H7xx_HAL_H

#include <stdint.h>

/* Using the fake "device file" means that the normal headers can be included, dramatically 
** reducing the amount of duplication this file requires */
#include "fake_stm32f401xc.h"
#define STM32F401xC
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_tim.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** DEFINES
***************************************************************************************************/



/***************************************************************************************************
** PUBLIC TYPEDEFS
***************************************************************************************************/

typedef struct ADC_HandleTypeDef
{
    struct {
        uint32_t NbrOfConversion;
    } Init;

    /* For imitating DMA samples */
    uint32_t* dma_address;
    uint32_t  dma_length;
} ADC_HandleTypeDef;

typedef uint32_t CRC_HandleTypeDef;
typedef uint32_t SPI_HandleTypeDef;
typedef uint32_t WWDG_HandleTypeDef;

/***************************************************************************************************
** PUBLIC OBJECTS
***************************************************************************************************/


/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/
/* ADC */
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length);

/* SPI */
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);

/* Watchdog */
HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg);

/* HAL */
void forceTick(uint32_t next_val);
void autoIncTick(uint32_t next_val, bool disable=false);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

#ifdef __cplusplus
}
#endif

#endif /* __STM32xxxx_HAL_H */