/** 
 ******************************************************************************
 * @file    faultHandlers.h
 * @brief   See faultHandlers.c
 * @date:   19 APR 2024
 * @author: Luke W
 ******************************************************************************
*/

#ifndef FAULT_HANDLERS_H_
#define FAULT_HANDLERS_H_

#include <stdint.h>
#include <stdbool.h>

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/*!
** The fault handler macro captures the address of the stack and type of fault, then calls  
** recordFaultType. It must be done as a macro, as calling a function will interfere with the stack
** pointer. Use [this article](https://interrupt.memfault.com/blog/cortex-m-hardfault-debug) as a 
** starting point.
**
** The [ARM Cortex-M7 Devices Generic User Guide](https://developer.arm.com/documentation/dui0646/c) 
** has more information.
*/
#define FAULT_HANDLER(_x)         \
    uint32_t tmp = _x;            /* Convert _x into local variable (allows accessing in asm) */ \
    __asm volatile(               /* Inline ASM */ \
        "tst lr, #4 \n"           /* Bit 2 of the execution return value (pushed to the link 
                                  ** register) informs which register the stack pointer is currently
                                  ** stored in */ \
        "ite eq \n"               /* If-else */ \
        "mrseq r0, msp \n"        /* If bit2 was low, copy stack pointer from MSP to R0 */ \
        "mrsne r0, psp \n"        /* Else, copy stack pointer from PSP to R0 */\
        "mov r1, %0 \n"           /* Move value from argument 0 (tmp) to R1 */ \
        "bl recordFaultType \n"   /* Branch-link to recordFaultType. R0 and R1 correspond with the 
                                  ** first and second argument. The link is required to be able to 
                                  ** return to this location in the code (e.g. to do some custom 
                                  ** error handling after the generic storage) */ \
        : : "r" (tmp))            /* Add tmp as a read argument for inline ASM */

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef enum {
    NO_FAULT = -1,
    NMI_FAULT = 0,
    HARD_FAULT,
    MEMMANAGE_FAULT,
    BUS_FAULT,
    USAGE_FAULT
} faultType_t;

typedef struct __attribute__((packed)) {
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t lr;
  uint32_t return_address;
  uint32_t xpsr;
} contextStateFrame_t;

typedef struct {
    faultType_t fault;
    uint32_t    CFSR;
    uint32_t    HFSR;
    uint32_t    MMFA;
    uint32_t    BFA;
    uint32_t    ABFS;
    contextStateFrame_t sFrame;
} faultInfo_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATION
***************************************************************************************************/

void recordFaultType(contextStateFrame_t* frame, faultType_t faultType);
void clearFaultInfo();
faultInfo_t* getFaultInfo();
bool printFaultInfo();
void setLocalFaultInfo(faultInfo_t* localFaultInfo);

#endif /* FAULT_HANDLERS_H_ */