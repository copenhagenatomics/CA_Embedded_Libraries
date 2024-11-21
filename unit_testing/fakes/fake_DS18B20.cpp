/*!
** @brief Fake Interface to DS18B20 for unit testing
**
** @author Matias
** @date   21/11/2024
*/

#include "fake_DS18B20.h"

float getTemp()
{
    return 0;
}

void DS18B20Init(TIM_HandleTypeDef* htim, StmGpio* gpio, GPIO_TypeDef *blk, uint16_t pin)
{
    return;
}