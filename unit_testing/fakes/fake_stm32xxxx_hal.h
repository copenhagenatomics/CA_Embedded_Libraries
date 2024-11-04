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
#define __STM32F4xx_ADC_H
#include "stm32f4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/** @defgroup ADC_Resolution ADC Resolution
  * @{
  */ 
#define ADC_RESOLUTION_12B  0x00000000U
#define ADC_RESOLUTION_10B  ((uint32_t)ADC_CR1_RES_0)
#define ADC_RESOLUTION_8B   ((uint32_t)ADC_CR1_RES_1)
#define ADC_RESOLUTION_6B   ((uint32_t)ADC_CR1_RES)

#define IS_ADC_RANGE(RESOLUTION, ADC_VALUE)                                     \
   ((((RESOLUTION) == ADC_RESOLUTION_12B) && ((ADC_VALUE) <= 0x0FFFU)) || \
    (((RESOLUTION) == ADC_RESOLUTION_10B) && ((ADC_VALUE) <= 0x03FFU)) || \
    (((RESOLUTION) == ADC_RESOLUTION_8B)  && ((ADC_VALUE) <= 0x00FFU)) || \
    (((RESOLUTION) == ADC_RESOLUTION_6B)  && ((ADC_VALUE) <= 0x003FU)))

#ifdef __LIBRARY_TEST
    #define __NOP() {}
#endif

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

/* For simulating I2C devices */
class stm32I2cTestDevice {
    public:
        I2C_TypeDef*    bus;
        uint16_t        addr;
        virtual HAL_StatusTypeDef transmit(uint8_t* buf, uint8_t size) = 0;
        virtual HAL_StatusTypeDef recv(uint8_t* buf, uint8_t size) = 0;
};

/***************************************************************************************************
** PUBLIC OBJECTS
***************************************************************************************************/


/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/
/* ADC */
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);

/* HAL */
void forceTick(uint32_t next_val);
void autoIncTick(uint32_t next_val, bool disable=false);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);

void fakeHAL_I2C_addDevice(stm32I2cTestDevice* new_device);

#ifdef __cplusplus
}
#endif

#endif /* __STM32xxxx_HAL_H */