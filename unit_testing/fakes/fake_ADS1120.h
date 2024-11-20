/*!
** @brief Fake Interface to ADS1120 for unit testing
**
** Extends normal ADS1120 interface with a few extra functions for testing
**
** @author Luke W
** @date   05/11/2024
*/

#ifndef __FAKE_ADS1120_H_
#define __FAKE_ADS1120_H_

#include <cstdint>

#include "fake_stm32xxxx_hal.h"
#include "fake_StmGpio.h" /* Included to prevent memory layout errors */

#include "ADS1120.h"

void setError(ADS1120Device *dev, int error);
void setTemp(ADS1120Device *dev, double chA, double chB, double internal);

#endif /* __FAKE_ADS1120_H_ */