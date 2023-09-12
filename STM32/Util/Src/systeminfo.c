/*
 * Generic functions used the the STM system
 */

#include <stdio.h>

#if defined(STM32F401xC)
#include "stm32f4xx_hal.h"
#include "HAL_otp.h"
#elif defined(STM32H753xx)
#include "stm32h7xx_hal.h"
#include "HAL_H7_otp.h"
#endif
#include "systemInfo.h"
#include "githash.h"

static struct BS
{
    uint32_t boardStatus;
    float temp;
    float underVoltage;
    float overVoltage;
    float overCurrent;
} BS = {0, 0, 0, 0};

// F4xx UID
#define ID1 *((unsigned long *) (UID_BASE))
#define ID2 *((unsigned long *) (UID_BASE + 4U))
#define ID3 *((unsigned long *) (UID_BASE + 8U))

static const char* mcuType()
{
    static char mcu[50] = { 0 }; // static to prevent allocate on stack.
    int len = 0;

    const DBGMCU_TypeDef* mcuType = DBGMCU;
    const uint16_t idCode = 0x00000FFF & mcuType->IDCODE;
    const uint16_t revCode = 0xFFFF & (mcuType->IDCODE >> 16);

    switch(idCode)
    {
    case 0x423: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xB/C");        break;
    case 0x433: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xD/E");        break;
    default:    len += snprintf(&mcu[len], sizeof(mcu) -len, "Unknown 0x%3X", idCode); break;
    }

    len += snprintf(&mcu[len], sizeof(mcu) -len, " Rev %x", revCode);

    return mcu;
}

static char* productType(uint8_t id)
{
    switch(id)
    {
    case AC_Board         :  return "AC Board";          break;
    case DC_Board         :  return "DC Board";          break;
    case Temperature      :  return "Temperature";       break;
    case Current          :  return "Current";           break;
    case GasFlow          :  return "GasFlow";           break;
    case HumidityChip     :  return "HumidityChip";      break;
    case Pressure         :  return "Pressure";          break;
    case SaltFlowBoard    :  return "Salt Flow Board";   break;
    case SaltLeak         :  return "SaltLeak";          break;
    case HotValveController: return "HotValveController"; break;
    case ZrO2Oxygen		  :  return "ZrO2Oxygen"; 		 break;
    case AMBcurrent		  :  return "AMBcurrent"; 		 break;
    case Geiger           :  return "Geiger"; 			 break;
    case AirCondition     :  return "AirCondition"; 	 break;
    case LightController  :  return "LightController"; 	 break;
    case LiquidLevel      :  return "LiquidLevel";   	 break;
    case ER               :  return "ER";            	 break;
    }
    return "NA";
}

const char* systemInfo()
{
    static char buf[600] = { 0 };
    BoardInfo info = { 0 };

    if (HAL_otpRead(&info) != OTP_SUCCESS)
    {
        info.otpVersion = 0; // Invalid OTP version
    }

    int len = 0;
    len  = snprintf(&buf[len], sizeof(buf) - len, "Serial Number: %lX%lX%lX\r\n", ID1, ID2, ID3);
    switch(info.otpVersion)
    {
    case OTP_VERSION_1:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n", productType(info.v1.boardType));
        break;
    case OTP_VERSION_2:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n", productType(info.v2.boardType));
        len += snprintf(&buf[len], sizeof(buf) - len, "Sub Product Type: %u\r\n", info.v2.subBoardType);
        break;
    default:
        len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: NA\r\n");
        break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "MCU Family: %s\r\n", mcuType());
    len += snprintf(&buf[len], sizeof(buf) - len, "Software Version: %s\r\n", GIT_VERSION);
    len += snprintf(&buf[len], sizeof(buf) - len, "Compile Date: %s\r\n", GIT_DATE);
    len += snprintf(&buf[len], sizeof(buf) - len, "Git SHA: %s\r\n", GIT_SHA);
    switch(info.otpVersion)
    {
    case OTP_VERSION_1:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d", info.v1.pcbVersion.major, info.v1.pcbVersion.minor);
        break;
    case OTP_VERSION_2:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d", info.v2.pcbVersion.major, info.v2.pcbVersion.minor);
        break;
    default:
        len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: NA");
        break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "\r\n");

    return buf;
}

const char* statusInfo()
{
    static char buf[600] = { 0 };
    int len = 0;
    if (!(BS.boardStatus & BS_ERROR_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, "Board Status: The board is operating normally.\r\n");
        return buf;
    } 
    len += snprintf(&buf[len], sizeof(buf) - len, "Board Status: ");

    if ((BS.boardStatus & BS_OVER_TEMPERATURE_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over temperature. The board temperature is %.2fC.\r\n", BS.temp);
    }

    if ((BS.boardStatus & BS_UNDER_VOLTAGE_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Under voltage. The board operates at too low voltage of %.2fV. Check power supply.\r\n", BS.underVoltage);
    }

    if ((BS.boardStatus & BS_OVER_VOLTAGE_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over voltage. One of the ports has reached its voltage limit of %.2fV. \r\n", BS.overVoltage);
    }

    if ((BS.boardStatus & BS_OVER_CURRENT_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over current. One of the ports has reached its current limit of %.2fA.\r\n", BS.overCurrent);
    }
    return buf;
}

int getBoardInfo(BoardType *bdt, SubBoardType *sbdt)
{
    BoardInfo info = { 0 };
    if (HAL_otpRead(&info) != OTP_SUCCESS)
        return -1;

    if (info.otpVersion == OTP_VERSION_1)
    {
        if (bdt)  { *bdt  = info.v1.boardType; }
        if (sbdt) { *sbdt = 0; } // Sub board type is not included in version 1
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2)
    {
        if (bdt)  { *bdt  = info.v2.boardType; }
        if (sbdt) { *sbdt = info.v2.subBoardType; }
        return 0;
    }

    return -1; // New OTP version. i.e. this SW is to old.
}

int getPcbVersion(pcbVersion* ver)
{
    BoardInfo info = { 0 };
    if (ver == 0 || HAL_otpRead(&info) != OTP_SUCCESS)
        return -1;

    if (info.otpVersion == OTP_VERSION_1)
    {
        ver->major = info.v1.pcbVersion.major;
        ver->minor = info.v1.pcbVersion.minor;
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2)
    {
        ver->major = info.v2.pcbVersion.major;
        ver->minor = info.v2.pcbVersion.minor;
        return 0;
    }

    return -1; // New OTP version. i.e. this SW is to old.
}

// Functions updating board status
void bsSetField(uint32_t field){ BS.boardStatus |= field; }
void bsClearField(uint32_t field){ BS.boardStatus &= ~field; }
uint32_t bsGetStatus(){ return BS.boardStatus; }
void setBoardTemp(float temp){ BS.temp = temp; }
void setBoardUnderVoltage(float voltage){ BS.underVoltage = voltage; }
void setBoardOverVoltage(float voltage){ BS.overVoltage = voltage; }
void setBoardOverCurrent(float current){ BS.overCurrent = current; }