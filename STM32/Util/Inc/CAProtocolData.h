/** 
 ******************************************************************************
 * @file    CAProtocolData.h
 * @date:   03 Feb 2025
 * @author: Matias
 ******************************************************************************
*/

#ifndef INC_CAPROTOCOLDATA_H_
#define INC_CAPROTOCOLDATA_H_

#include <stddef.h>

#include "CAProtocol.h"

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef struct CAProtocolData {
    size_t len;         // Length of current data.
    uint8_t buf[512];   // Buffer for the string fetched from the circular buffer.
    ReaderFn rxReader;  // Reader for the buffer
} CAProtocolData;

#endif /* INC_CAPROTOCOLDATA_H_ */