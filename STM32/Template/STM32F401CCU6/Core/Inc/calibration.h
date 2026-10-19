/*!
 * @file    calibration.h
 * @brief   Header file of calibration.c
 * @date	DATE
 * @author 	AUTHOR
 */

#ifndef INC_CALIBRATION_H_
#define INC_CALIBRATION_H_

#include <stdbool.h>
#include <stdint.h>
#include "CAProtocol.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct FlashCalibration {
    float cal1;
    float cal2;
} FlashCalibration_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void calibration(int noOfCalibrations, const CACalibration *calibrations, FlashCalibration_t *cal,
                 uint32_t size);
void calibrationInit(CRC_HandleTypeDef *hcrc, FlashCalibration_t *cal, uint32_t size);
void calibrationRW(bool write, FlashCalibration_t *cal, uint32_t size);

#endif /* INC_CALIBRATION_H_ */
