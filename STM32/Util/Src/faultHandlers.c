/** 
 ******************************************************************************
 * @file    faultHandlers.c
 * @brief   This file contains fault handlers and crash info
 * @date:   19 APR 2024
 * @author: Luke W
 ******************************************************************************
*/

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "stm32h7xx_hal.h"

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
    faultInfo_t faultInfo = {0};
    
    if(faultType != NO_FAULT) {
        faultInfo.fault  = faultType;
        faultInfo.CFSR   = SCB->CFSR;
        faultInfo.HFSR   = SCB->HFSR;
        faultInfo.MMFA   = SCB->MMFAR;
        faultInfo.BFA    = SCB->BFAR;
        faultInfo.ABFS   = SCB->AFSR;
        faultInfo.sFrame = *frame;

        writeToFlash((uint32_t)(&_FlashAddr), sizeof(faultInfo_t), (void*)&faultInfo);
    }
}

/*!
** @brief This function clears any information on faults from flash memory
*/
void clearFaultInfo() {
    faultInfo_t faultInfo = {.fault = -1};
    writeToFlash((uint32_t)(&_FlashAddr), sizeof(faultInfo_t), (void*)&faultInfo);
}

/*!
** @brief This function returns most recent fault Info as a struct
*/
void getFaultInfo(faultInfo_t* faultInfo) {
    readFromFlash((uint32_t)(&_FlashAddr), sizeof(faultInfo_t), (void*)faultInfo);
}

/*!
** @brief This function formats and prints relevant faultInfo
*/
bool printFaultInfo() {
    static char buf[BUF_LEN] = {0};
    int len = 0;
    faultInfo_t fi = {0};
    getFaultInfo(&fi);

    if(fi.fault != NO_FAULT) {
        len += snprintf(&buf[len], BUF_LEN - len, "\r\nStart of fault info\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "Last fault was: %d\r\n", fi.fault);
        len += snprintf(&buf[len], BUF_LEN - len, "CFSR was: 0x%08lX\r\n",  fi.CFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "HFSR was: 0x%08lX\r\n",  fi.HFSR);
        len += snprintf(&buf[len], BUF_LEN - len, "MMFA was: 0x%08lX\r\n",  fi.MMFA);
        len += snprintf(&buf[len], BUF_LEN - len, "BFA was:  0x%08lX\r\n",  fi.BFA);
        len += snprintf(&buf[len], BUF_LEN - len, "AFSR was: 0x%08lX\r\n",  fi.ABFS);
        len += snprintf(&buf[len], BUF_LEN - len, "Stack Frame was:\r\n");
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08lX, 0x%08lX, 0x%08lX, 0x%08lX\r\n", 
            fi.sFrame.r0, fi.sFrame.r1, fi.sFrame.r2, fi.sFrame.r3);
        len += snprintf(&buf[len], BUF_LEN - len, "0x%08lX, 0x%08lX, 0x%08lX, 0x%08lX\r\n",
            fi.sFrame.r12, fi.sFrame.lr, fi.sFrame.return_address, fi.sFrame.xpsr);
        len += snprintf(&buf[len], BUF_LEN - len, "End of fault info");

        writeUSB(buf, len);

        return true;
    }

    return false;
}