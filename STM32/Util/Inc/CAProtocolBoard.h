/** 
 ******************************************************************************
 * @file    CAProtocolBoard.h
 * @date:   31 Jan 2025
 * @author: Matias
 ******************************************************************************
*/

#ifndef INC_CAPROTOCOLBOARD_H_
#define INC_CAPROTOCOLBOARD_H_

#include "CAProtocol.h"
#include "CAProtocolStm.h"

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void ACDCInputHandler(CAProtocolCtx* ctx, const char* input);

#endif /* INC_CAPROTOCOLBOARD_H_ */