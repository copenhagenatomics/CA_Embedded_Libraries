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
#include "FLASH_readwrite.h"
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

/* Defined in the linker script */
extern uint32_t _FlashAddr;

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
        faultInfo_t faultInfo = {
            .fault  = faultType,
            .CFSR   = SCB->CFSR,
            .HFSR   = SCB->HFSR,
            .MMFA   = SCB->MMFAR,
            .BFA    = SCB->BFAR,
            .ABFS   = SCB->AFSR,
            .sFrame = *frame,
        };

        writeToFlash(0, sizeof(faultInfo_t), (uint8_t*)&faultInfo);
    }
}

/*!
** @brief This function clears any information on faults from flash memory
*/
void clearFaultInfo() {
    faultInfo_t faultInfo = {.fault = NO_FAULT};
    writeToFlash(0, sizeof(faultInfo_t), (uint8_t*)&faultInfo);
}

/*!
** @brief This function returns most recent fault Info as a struct
*/
void getFaultInfo(faultInfo_t* faultInfo) {
    readFromFlash(0, sizeof(faultInfo_t), (uint8_t*)faultInfo);
}

/*!
** @brief This function formats and prints relevant faultInfo
*/
bool printFaultInfo() {
    static char buf[BUF_LEN] = {0};
    int len = 0;
    faultInfo_t fi;
    getFaultInfo(&fi);

    if(fi.fault != NO_FAULT) {
        len += snprintf(&buf[len], BUF_LEN - len, "\nStart of fault info\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "Last fault was: %d\r\n", fi.fault);
        len += snprintf(&buf[len], BUF_LEN - len, "CFSR was: 0x%08" PRIx32 "\r\n",  fi.CFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "HFSR was: 0x%08" PRIx32 "\r\n",  fi.HFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "MMFA was: 0x%08" PRIx32 "\r\n",  fi.MMFA);
        len += snprintf(&buf[len], BUF_LEN - len, "BFA was:  0x%08" PRIx32 "\r\n",  fi.BFA);
        len += snprintf(&buf[len], BUF_LEN - len, "AFSR was: 0x%08" PRIx32 "\r\n",  fi.ABFS);
        len += snprintf(&buf[len], BUF_LEN - len, "Stack Frame was:\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n", 
            fi.sFrame.r0, fi.sFrame.r1, fi.sFrame.r2, fi.sFrame.r3);
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 ", 0x%08" PRIx32 "\r\n",
            fi.sFrame.r12, fi.sFrame.lr, fi.sFrame.return_address, fi.sFrame.xpsr);
        len += snprintf(&buf[len], BUF_LEN - len, "End of fault info\r\n");

        writeUSB(buf, len);

        return true;
    }

    return false;
}