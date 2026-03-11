/**
 ******************************************************************************
 * @file    faultHandlers.c
 * @brief   This file contains fault handlers and crash info
 * @date:   19 APR 2024
 * @author: Luke W
 ******************************************************************************
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "USBprint.h"
#include "faultHandlers.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define BUF_LEN 400

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
    if (faultType != NO_FAULT) {
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
void clearFaultInfo() { _localFaultInfo->fault = NO_FAULT; }

/*!
** @brief Returns the address of the fault info structure
*/
faultInfo_t* getFaultInfo() { return _localFaultInfo; }

/*!
** @brief This function formats and prints relevant faultInfo
*/
bool printFaultInfo() {
    static char buf[BUF_LEN] = {0};
    int len                  = 0;

    if (_localFaultInfo->fault != NO_FAULT) {
        CA_SNPRINTF(buf, len, "Start of fault info\r\n");
        CA_SNPRINTF(buf, len, "Last fault was: %d\r\n", _localFaultInfo->fault);
        CA_SNPRINTF(buf, len, "CFSR was: 0x%08" PRIx32 "\r\n", _localFaultInfo->CFSR);
        CA_SNPRINTF(buf, len, "HFSR was: 0x%08" PRIx32 "\r\n", _localFaultInfo->HFSR);
        CA_SNPRINTF(buf, len, "MMFA was: 0x%08" PRIx32 "\r\n", _localFaultInfo->MMFA);
        CA_SNPRINTF(buf, len, "BFA was:  0x%08" PRIx32 "\r\n", _localFaultInfo->BFA);
        CA_SNPRINTF(buf, len, "AFSR was: 0x%08" PRIx32 "\r\n", _localFaultInfo->ABFS);
        CA_SNPRINTF(buf, len, "Stack Frame was:\r\n");
        CA_SNPRINTF(buf, len,
                    "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n",
                    _localFaultInfo->sFrame.r0, _localFaultInfo->sFrame.r1,
                    _localFaultInfo->sFrame.r2, _localFaultInfo->sFrame.r3);
        CA_SNPRINTF(buf, len,
                    "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n",
                    _localFaultInfo->sFrame.r12, _localFaultInfo->sFrame.lr,
                    _localFaultInfo->sFrame.return_address, _localFaultInfo->sFrame.xpsr);
        CA_SNPRINTF(buf, len, "End of fault info\r\n");

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
void setLocalFaultInfo(faultInfo_t* localFaultInfo) { _localFaultInfo = localFaultInfo; }