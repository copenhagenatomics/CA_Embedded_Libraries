#include <string.h>
#include <stdio.h>
#include <inttypes.h>

#include "CAProtocolStm.h"
#include "USBprint.h"
#include "jumpToBootloader.h"
#include "systemInfo.h"

#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#elif defined(STM32H753xx)
#include "stm32h7xx_hal.h"
#endif
#include "HAL_otp.h"

#if defined(STM32H753xx)
#define RCC_FLAG_WWDGRST RCC_FLAG_WWDG1RST
#define RCC_FLAG_IWDGRST RCC_FLAG_IWDG1RST
#endif

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

void HALundefined(const char *input)
{
    if(strcmp(input, "\0"))
    {
        USBnprintf("MISREAD: %s", input);
    }
}

void HALJumpToBootloader()
{
    USBnprintf("Entering bootloader mode");
    __HAL_RCC_WWDG_CLK_DISABLE();
    HAL_Delay(200);
    JumpToBootloader(); // function never returns.
}

void CAPrintHeader()
{
    USBnprintf(systemInfo());
}

void CAPrintStatus(bool printStart)
{
    USBnprintf(statusInfo(printStart));
}

void CAPrintStatusDef(bool printStart)
{
    USBnprintf(statusDefInfo(printStart));
}

void CAotpRead()
{
    BoardInfo info;
    if (HAL_otpRead(&info))
    {
        USBnprintf("OTP: No production available");
    }
    else
    {
        switch(info.otpVersion)
        {
        case OTP_VERSION_1:
            USBnprintf("OTP %u %u %u.%u %u\r\n"
                     , info.otpVersion
                     , info.v1.boardType
                     , info.v1.pcbVersion.major
                     , info.v1.pcbVersion.minor
                     , info.v1.productionDate);
            break;
        case OTP_VERSION_2:
            USBnprintf("OTP %u %u %u %u.%u %u\r\n"
                     , info.otpVersion
                     , info.v2.boardType
                     , info.v2.subBoardType
                     , info.v2.pcbVersion.major
                     , info.v2.pcbVersion.minor
                     , info.v2.productionDate);
            break;
        default:
            USBnprintf("Not supported version %d of OTP data. Update firmware in board.", info.otpVersion);
            break;
        }
    }
}

bool CAhandleUserInputs(CAProtocolCtx* ctx, const char* startMsg)
{
    static bool isFirstWrite = true;
    bool firstWriteHappened = false;
    if (isUsbPortOpen())
    {
        // Upon first write print line and reset circular buffer to ensure no faulty misreads occurs.
        if (isFirstWrite)
        {
            USBnprintf(startMsg);
            flushCAProtocol(ctx);
            usbFlush();
            isFirstWrite = false;
            firstWriteHappened = true;
        }
    }
    else
    {
        isFirstWrite = true;
    }

    inputCAProtocol(ctx);

    return firstWriteHappened;
}

const char* CAonBoot()
{
    static char msg[100]; // Make static to prevent allocation on stack.

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
    {
        sprintf(msg, "reconnected Reset Reason: Hardware Watch dog");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
	{
		sprintf(msg, "reconnected Reset Reason: Internal Watch dog");
	}
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        sprintf(msg, "reconnected Reset Reason: Software Reset");
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        sprintf(msg, "reconnected Reset Reason: Power On");
    }
    else
    {
        // System has none of watchdog, SW reset or porrst bit set. At least
        // one reason should be set => this should never happen, inspect!!
        sprintf(msg, "reconnected Reset Reason: Unknown(%" PRIx32 ")", RCC->CSR);
    }

    // Reset the boot flags.
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return msg;
}
