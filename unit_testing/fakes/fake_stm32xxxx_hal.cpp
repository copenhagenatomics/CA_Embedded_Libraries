#include <chrono>
#include <vector>
#include "fake_stm32xxxx_hal.h"

using namespace std::chrono;

/***************************************************************************************************
** PUBLIC OBJECTS
***************************************************************************************************/

TIM_TypeDef TIM2_obj;
TIM_TypeDef TIM3_obj;
TIM_TypeDef TIM4_obj;
TIM_TypeDef TIM5_obj;
RTC_TypeDef RTC_obj;
WWDG_TypeDef WWDG_obj;
IWDG_TypeDef IWDG_obj;
SPI_TypeDef I2S2ext_obj;
SPI_TypeDef SPI2_obj;
SPI_TypeDef SPI3_obj;
SPI_TypeDef I2S3ext_obj;
USART_TypeDef USART2_obj;
I2C_TypeDef I2C1_obj;
I2C_TypeDef I2C2_obj;
I2C_TypeDef I2C3_obj;
PWR_TypeDef PWR_obj;
TIM_TypeDef TIM1_obj;
USART_TypeDef USART1_obj;
USART_TypeDef USART6_obj;
ADC_TypeDef ADC1_obj;
ADC_Common_TypeDef ADC1_COMMON_obj;
//SDIO_TypeDef SDIO_obj;
SPI_TypeDef SPI1_obj;
SPI_TypeDef SPI4_obj;
SYSCFG_TypeDef SYSCFG_obj;
EXTI_TypeDef EXTI_obj;
TIM_TypeDef TIM9_obj;
TIM_TypeDef TIM10_obj;
TIM_TypeDef TIM11_obj;
GPIO_TypeDef GPIOA_obj;
GPIO_TypeDef GPIOB_obj;
GPIO_TypeDef GPIOC_obj;
GPIO_TypeDef GPIOD_obj;
GPIO_TypeDef GPIOE_obj;
GPIO_TypeDef GPIOH_obj;
CRC_TypeDef CRC_obj;
RCC_TypeDef RCC_obj;
FLASH_TypeDef FLASH_obj;
DMA_TypeDef DMA1_obj;
DMA_Stream_TypeDef DMA1_Stream0_obj;
DMA_Stream_TypeDef DMA1_Stream1_obj;
DMA_Stream_TypeDef DMA1_Stream2_obj;
DMA_Stream_TypeDef DMA1_Stream3_obj;
DMA_Stream_TypeDef DMA1_Stream4_obj;
DMA_Stream_TypeDef DMA1_Stream5_obj;
DMA_Stream_TypeDef DMA1_Stream6_obj;
DMA_Stream_TypeDef DMA1_Stream7_obj;
DMA_TypeDef DMA2_obj;
DMA_Stream_TypeDef DMA2_Stream0_obj;
DMA_Stream_TypeDef DMA2_Stream1_obj;
DMA_Stream_TypeDef DMA2_Stream2_obj;
DMA_Stream_TypeDef DMA2_Stream3_obj;
DMA_Stream_TypeDef DMA2_Stream4_obj;
DMA_Stream_TypeDef DMA2_Stream5_obj;
DMA_Stream_TypeDef DMA2_Stream6_obj;
DMA_Stream_TypeDef DMA2_Stream7_obj;
DBGMCU_TypeDef DBGMCU_obj;
USB_OTG_GlobalTypeDef USB_OTG_FS_obj;
SCB_Type SCB_obj;

/***************************************************************************************************
** PRIVATE MEMBERS
***************************************************************************************************/

static bool force_tick = false;
static bool auto_tick = false;
static uint32_t next_tick = 0;

/* For simulating I2C devices */
std::vector<stm32I2cTestDevice*>* devices = nullptr;

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

#ifdef HAL_ADC_MODULE_ENABLED

HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
    /* Do nothing */
    hadc->dma_address = pData;
    hadc->dma_length  = Length;
    return HAL_OK;
}

__weak void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_ADC_ConvHalfCpltCallback could be implemented in the user file
   */
}

__weak void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_ADC_ConvHalfCpltCallback could be implemented in the user file
   */
}

#endif

#ifdef HAL_WWDG_MODULE_ENABLED

HAL_StatusTypeDef HAL_WWDG_Refresh(WWDG_HandleTypeDef *hwwdg)
{
    /* Do nothing */
    return HAL_OK;
}

#endif

#ifdef HAL_TIM_MODULE_ENABLED

HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)
{
    htim->State = HAL_TIM_STATE_BUSY;

    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}

HAL_TIM_StateTypeDef HAL_TIM_Base_GetState(TIM_HandleTypeDef *htim) {
    return htim->State;
}

HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    /* Do nothing */
    return HAL_OK;
}

#endif

#ifdef HAL_SPI_MODULE_ENABLED

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

HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, const uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    /* Do nothing */
    return HAL_OK;
}

#endif

#ifdef HAL_I2C_MODULE_ENABLED

void fakeHAL_I2C_addDevice(stm32I2cTestDevice* new_device) {
    if(!devices) {
        devices = new std::vector<stm32I2cTestDevice*>();
    }

    devices->push_back(new_device);
}

HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    if(devices) {
        for(const auto& i : *devices) {
            if((i->bus == hi2c->Instance) && (i->addr == DevAddress)) {
                return i->transmit(pData, Size);
            }
        }
    }

    return HAL_ERROR;
}

HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *hi2c, uint16_t DevAddress, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    if(devices) {
        for(const auto& i : *devices) {
            if((i->bus == hi2c->Instance) && (i->addr == DevAddress)) {
                return i->recv(pData, Size);
            }
        }
    }

    return HAL_ERROR;
}

#endif

#ifdef HAL_FLASH_MODULE_ENABLED
HAL_StatusTypeDef HAL_FLASH_Lock(void)
{
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Unlock(void)
{
    /* Do nothing */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data)
{
    /* Do nothing */
    return HAL_OK;
}

void FLASH_Erase_Sector(uint32_t Sector, uint8_t VoltageRange)
{
    /* Do nothing */
    return;
}
#endif

#ifdef HAL_CRC_MODULE_ENABLED

uint32_t HAL_CRC_Calculate(CRC_HandleTypeDef *hcrc, uint32_t pBuffer[], uint32_t BufferLength)
{
    /* Do nothing */
    return 0;
}

#endif
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

void HAL_NVIC_SystemReset(void)
{

}