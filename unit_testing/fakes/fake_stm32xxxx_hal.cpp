#include <chrono>

#include "fake_stm32xxxx_hal.h"

using namespace std::chrono;

/***************************************************************************************************
** PUBLIC OBJECTS
***************************************************************************************************/

RCC_TypeDef RCC_obj;
TIM_TypeDef TIM4_obj;
TIM_TypeDef TIM5_obj;

/***************************************************************************************************
** PRIVATE MEMBERS
***************************************************************************************************/

static bool force_tick = false;
static bool auto_tick = false;
static uint32_t next_tick = 0;

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
    /* Do nothing */
    hadc->dma_address = pData;
    hadc->dma_length  = Length;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg)
{
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)
{
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
{
    /* Do nothing */
    return HAL_OK;
}

void forceTick(uint32_t next_val)
{
    force_tick = true;
    next_tick = next_val;
}

void autoIncTick(uint32_t next_val, bool disable)
{
    if(!disable)
    {
        auto_tick = true;
        next_tick = next_val;
    }
    else
    {
        auto_tick = false;
    }
}

uint32_t HAL_GetTick(void) 
{
    if(!force_tick && !auto_tick)
    {
        return duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
    }
    else 
    {
        /* next_tick incremented after return for auto-tick */
        uint32_t ret_val = auto_tick ? next_tick++ : next_tick;
        return ret_val;
    }
}

void HAL_Delay(uint32_t Delay)
{

}