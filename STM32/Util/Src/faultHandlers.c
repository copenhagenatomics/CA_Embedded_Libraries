/** 
 ******************************************************************************
 * @file    faultHandlers.c
 * @brief   This file contains fault handlers and crash info
 * @date:   19 APR 2024
 * @author: Luke W
 ******************************************************************************
*/

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "faultHandlers.h"
#include "USBprint.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define BUF_LEN 400

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/



/***************************************************************************************************
** PRIVATE VARIABLES
***************************************************************************************************/

static faultInfo_t* _localFaultInfo = NULL;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief This function records the fault type in flash memory
**
** Depending on fault type, further information is collected and also stored in memory.
*/
void recordFaultType(contextStateFrame_t* frame, faultType_t faultType) {
    if(faultType != NO_FAULT) {
        _localFaultInfo->fault  = faultType;
        _localFaultInfo->CFSR   = SCB->CFSR;
        _localFaultInfo->HFSR   = SCB->HFSR;
        _localFaultInfo->MMFA   = SCB->MMFAR;
        _localFaultInfo->BFA    = SCB->BFAR;
        _localFaultInfo->ABFS   = SCB->AFSR;
        _localFaultInfo->sFrame = *frame;
    }
}

/*!
** @brief This function clears any information on faults from flash memory
*/
void clearFaultInfo() {
    _localFaultInfo->fault = NO_FAULT;
}

/*!
** @brief Returns the address of the fault info structure
*/
faultInfo_t* getFaultInfo() {
    return _localFaultInfo;
}

/*!
** @brief This function formats and prints relevant faultInfo
*/
bool printFaultInfo() {
    static char buf[BUF_LEN] = {0};
    int len = 0;

    if(_localFaultInfo->fault != NO_FAULT) {
        len += snprintf(&buf[len], BUF_LEN - len, "\nStart of fault info\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "Last fault was: %d\r\n", _localFaultInfo->fault);
        len += snprintf(&buf[len], BUF_LEN - len, "CFSR was: 0x%08" PRIx32 "\r\n",  _localFaultInfo->CFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "HFSR was: 0x%08" PRIx32 "\r\n",  _localFaultInfo->HFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "MMFA was: 0x%08" PRIx32 "\r\n",  _localFaultInfo->MMFA);
        len += snprintf(&buf[len], BUF_LEN - len, "BFA was:  0x%08" PRIx32 "\r\n",  _localFaultInfo->BFA);
        len += snprintf(&buf[len], BUF_LEN - len, "AFSR was: 0x%08" PRIx32 "\r\n",  _localFaultInfo->ABFS);
        len += snprintf(&buf[len], BUF_LEN - len, "Stack Frame was:\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n", 
            _localFaultInfo->sFrame.r0, _localFaultInfo->sFrame.r1, _localFaultInfo->sFrame.r2, _localFaultInfo->sFrame.r3);
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n",
            _localFaultInfo->sFrame.r12, _localFaultInfo->sFrame.lr, _localFaultInfo->sFrame.return_address, _localFaultInfo->sFrame.xpsr);
        len += snprintf(&buf[len], BUF_LEN - len, "End of fault info\r\n");

        writeUSB(buf, len);

        return true;
    }

    return false;
}

/*!
** @brief Sets the address of the variable to use to communicate fault information.
**
** It is the user's responsibility to ensure the fault information is written/loaded from flash as 
** required for the proper operation of these functions
*/
void setLocalFaultInfo(faultInfo_t* localFaultInfo) {
    _localFaultInfo = localFaultInfo;
}