/*!
 * @file    template_uptime.h
 * @brief   Header file of template_uptime.c
 * @date    DATE
 * @author  AUTHOR
 */

#ifndef INC_OXYGEN_INTEGRATED_UPTIME_H_
#define INC_OXYGEN_INTEGRATED_UPTIME_H_

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void initOxygenIntegratedUptime(CRC_HandleTypeDef* _hcrc, const char* boot_msg);
void loopOxygenIntegratedUptime(ZrO2Device_t* ZrO2s);

#endif /* INC_OXYGEN_INTEGRATED_UPTIME_H_ */
