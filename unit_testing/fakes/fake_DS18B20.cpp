/*!
** @brief Fake Interface to DS18B20 for unit testing
**
** @author Matias
** @date   21/11/2024
*/

#include "fake_DS18B20.h"
#include "fake_stm32xxxx_hal.h"

float getTemp()
{

}

void DS18B20Init(TIM_HandleTypeDef* htim, StmGpio* gpio, GPIO_TypeDef *blk, uint16_t pin)
{

}