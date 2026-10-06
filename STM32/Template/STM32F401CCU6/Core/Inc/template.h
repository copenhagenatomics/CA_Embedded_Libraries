/*!
 * @file    template.h
 * @brief   Header file of template.c
 * @date    DATE
 * @author  AUTHOR
 */

#ifndef INC_TEMPLATE_H_
#define INC_TEMPLATE_H_

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// Used for defining which bits are errors, and which are statuses
#define TEMPLATE_ERRORS_Msk (BS_SYSTEM_ERRORS_Msk)

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void templateInit(TIM_HandleTypeDef* adcTim, ADC_HandleTypeDef* hadc, CRC_HandleTypeDef* hcrc,
                  const char* bootMsg);
void templateLoop(const char* bootMsg);

#endif /* INC_TEMPLATE_H_ */
