/**
 ******************************************************************************
 * @file    CAProtocolACDC.h
 * @date:   31 Jan 2025
 * @author: Matias
 ******************************************************************************
 */

#ifndef INC_CAPROTOCOLACDC_H_
#define INC_CAPROTOCOLACDC_H_

#include "CAProtocolStm.h"

/***************************************************************************************************
** STRUCTURES
***************************************************************************************************/

typedef struct {
    // Turn on all ports
    void (*allOn)(bool isOn, int duration);
    // Control ports individually
    void (*portState)(int port, bool state, int percent, int duration);
} ACDCProtocolCtx;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void ACDCInputHandler(ACDCProtocolCtx* ctx, const char* input);

#endif /* INC_CAPROTOCOLACDC_H_ */