/*!
 * @file    template_uptime.h
 * @brief   Header file of template_uptime.c
 * @date    DATE
 * @author  AUTHOR
 */

#ifndef INC_TEMPLATE_UPTIME_H_
#define INC_TEMPLATE_UPTIME_H_

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void initUptime(CRC_HandleTypeDef* _hcrc, const char* boot_msg);
void loopUptime();

#endif /* INC_TEMPLATE_UPTIME_H_ */
