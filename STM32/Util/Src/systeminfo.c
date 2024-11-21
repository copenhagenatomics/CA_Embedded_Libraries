/*
 * Generic functions used the the STM system
 */

#include <stdio.h>
#include <inttypes.h>

#include "HAL_otp.h"
#include "systemInfo.h"

#ifndef UNIT_TESTING
    #if defined(STM32F401xC)
    #include "stm32f4xx_hal.h"
    #elif defined(STM32H753xx)
    #include "stm32h7xx_hal.h"
    #endif
    #include "githash.h"
#else

#endif


// Struct containing the board status sw registry
// as well as some of the general values.
// A board should update the values in this struct using the
// functions defined below for updating the board status.
static struct BS
{
    uint32_t boardStatus;
    float temp;
    float voltage;
    float current;
    uint32_t usb;
    BoardType boardType;
    pcbVersion pcb_version;
} BS = {0};

// Print buffer for systemInfo & statusInfo
static char buf[600] = { 0 };

// F4xx UID
#ifndef UNIT_TESTING
    #define ID1 *((unsigned long *) (UID_BASE))
    #define ID2 *((unsigned long *) (UID_BASE + 4U))
    #define ID3 *((unsigned long *) (UID_BASE + 8U))
#else
    #define ID1 (unsigned long) 0U
    #define ID2 (unsigned long) 0U
    #define ID3 (unsigned long) 0U

    #define GIT_VERSION "0"
    #define GIT_DATE    "0"
    #define GIT_SHA     "0"
#endif

static const char* mcuType()
{
    static char mcu[50] = { 0 }; // static to prevent allocate on stack.
    int len = 0;

#ifndef UNIT_TESTING
    const DBGMCU_TypeDef* mcuType = DBGMCU;
    const uint16_t idCode = 0x00000FFF & mcuType->IDCODE;
    const uint16_t revCode = 0xFFFF & (mcuType->IDCODE >> 16);
#else
    const uint16_t idCode = 0;
    const uint16_t revCode = 0;
#endif

    switch(idCode)
    {
    case 0x423: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xB/C");        break;
    case 0x433: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32F401xD/E");        break;
    case 0x450: len += snprintf(&mcu[len], sizeof(mcu) -len, "STM32H753IIT6");        break;
    default:    len += snprintf(&mcu[len], sizeof(mcu) -len, "Unknown 0x%3X", idCode); break;
    }

    len += snprintf(&mcu[len], sizeof(mcu) -len, " Rev %x", revCode);

    return mcu;
}

static const char* productType(uint8_t id)
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
    case ERHC             :  return "ERHC";            	 break;
    case VFD              :  return "VFD";            	 break;
    case Tachometer       :  return "Tachometer";     	 break;
    case ACTenChannel     :  return "ACTenChannel";    	 break;
    }
    return "NA";
}

const char* systemInfo()
{
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

const char* statusInfo(bool printStart)
{
    int len = 0;

    // Print end of message and return
    if (!printStart)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, "End of board status. \r\n");
        return buf;
    }

    len += snprintf(&buf[len], sizeof(buf) - len, "Start of board status:\r\n");
    if (!(BS.boardStatus & BS_ERROR_Msk))
    {
        len += snprintf(&buf[len], sizeof(buf) - len, "The board is operating normally.\r\n");
        return buf;
    } 
    
    if (BS.boardStatus & BS_OVER_TEMPERATURE_Msk)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over temperature. The board temperature is %.2fC.\r\n", BS.temp);
    }

    if (BS.boardStatus & BS_UNDER_VOLTAGE_Msk)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Under voltage. The board operates at too low voltage of %.2fV. Check power supply.\r\n", BS.voltage);
    }

    if (BS.boardStatus & BS_OVER_VOLTAGE_Msk)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over voltage. One of the ports has reached a voltage out of its measurement range at %.2fV. \r\n", BS.voltage);
    }

    if (BS.boardStatus & BS_OVER_CURRENT_Msk)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "Over current. One of the ports has reached a current out of its measurement range at %.2fA.\r\n", BS.current);
    }

    if (BS.boardStatus & BS_VERSION_ERROR_Msk)
    {
        BoardType bt = (BoardType)0;
        pcbVersion pv = {0};
        (void) getBoardInfo(&bt, NULL);
        (void) getPcbVersion(&pv);
        len += snprintf(&buf[len], sizeof(buf) - len, 
            "Error: Incorrect Version.\r\n"
            "   Board is: %d.\r\n"
            "   Board should be: %d.\r\n"
            "   PCB Version is: %d.%d.\r\n"
            "   PCB Version should be > %d.%d.\r\n", 
            (int)bt, (int)BS.boardType, pv.major, pv.minor, BS.pcb_version.major, BS.pcb_version.minor);
    }

    if (BS.usb)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, 
                        "USB. USB communication error 0x%08" PRIx32 " occurred most recently.\r\n", BS.usb);
        BS.usb = 0U;
    }

    return buf;
}

const char* statusDefInfo(bool printStart)
{
    int len = 0;

    // Print end of message and return
    if (!printStart)
    {
        len += snprintf(&buf[len], sizeof(buf) - len, "End of board status definition. \r\n");
        return buf;
    }

    len += snprintf(&buf[len], sizeof(buf) - len, "Start of board status definition:\r\n");

    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",System errors\r\n", BS_SYSTEM_ERRORS_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Error\r\n", BS_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over temperature\r\n", BS_OVER_TEMPERATURE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Under voltage\r\n", BS_UNDER_VOLTAGE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over voltage\r\n", BS_OVER_VOLTAGE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over current\r\n", BS_OVER_CURRENT_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Version error\r\n", BS_VERSION_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",USB error\r\n", BS_USB_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Flash ongoing\r\n", BS_FLASH_ONGOING_Msk);

    return buf;
}

int getBoardInfo(BoardType *bdt, SubBoardType *sbdt)
{
    BoardInfo info = { 0 };
    if (HAL_otpRead(&info) != OTP_SUCCESS)
        return -1;

    if (info.otpVersion == OTP_VERSION_1)
    {
        if (bdt)  { *bdt  = (BoardType)info.v1.boardType; }
        if (sbdt) { *sbdt = 0; } // Sub board type is not included in version 1
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2)
    {
        if (bdt)  { *bdt  = (BoardType)info.v2.boardType; }
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
void bsSetErrorRange(uint32_t field, uint32_t range)
{
    // For ranges of bits where only a subset of them are being set,
    // then setting a new subset will result in the union of the two subset.
    // Since, for a range it is assumed that the bits in the range
    // are related i.e. each combination meaning a unique state, then
    // we need to reset previous state before setting the new state.
    BS.boardStatus &= ~range;
    BS.boardStatus |= (BS_ERROR_Msk | field);
}

void bsSetFieldRange(uint32_t field, uint32_t range)
{
    // Reset bits before setting the new value of the range for the same
    // reason as described in the bsSetErrorRange function.
    BS.boardStatus &= ~range;
    BS.boardStatus |= field;
}

/*!
** @brief Clears the error bit, if none of the bits in the field are set
*/
void bsClearError(uint32_t field) {     
    if (!(BS.boardStatus & field))
    {
        bsClearField(BS_ERROR_Msk);
    } 
}

void bsSetError(uint32_t field) { BS.boardStatus |= (BS_ERROR_Msk | field); }
void bsSetField(uint32_t field){ BS.boardStatus |= field; }
void bsClearField(uint32_t field){ BS.boardStatus &= ~field; }
uint32_t bsGetStatus(){ return BS.boardStatus; }
uint32_t bsGetField(uint32_t field){ return BS.boardStatus & field; }
void setBoardTemp(float temp){ BS.temp = temp; }
void setBoardVoltage(float voltage){ BS.voltage = voltage; }
void setBoardCurrent(float current){ BS.current = current; }
void setBoardUsbError(uint32_t err) {BS.usb = err; }
void setFirmwareBoardType(BoardType type){ BS.boardType = type; }
void setFirmwareBoardVersion(pcbVersion version){ BS.pcb_version = version; }

/*!
** @brief Sets the board type and version the firmware was compiled for
**
** If the board type/version does not match the value programmed in the OTP, this function sets the 
** board status version error flag, and returns -1
**
** @param[in] type              Type of board, e.g. AC, DC, Current ...
** @param[in] breaking_version  Oldest version of PCB this firmware can run on
*/
int boardSetup(BoardType type, pcbVersion breaking_version) {
    setFirmwareBoardType(type);
    setFirmwareBoardVersion(breaking_version);

    BoardType board;
    if (getBoardInfo(&board, NULL) || board != type) {
        bsSetError(BS_VERSION_ERROR_Msk);
    }

    pcbVersion ver;
    if (getPcbVersion(&ver) || ver.major < breaking_version.major) {
        bsSetError(BS_VERSION_ERROR_Msk);
    }
    else if(ver.major == breaking_version.major && ver.minor < breaking_version.minor) {
        bsSetError(BS_VERSION_ERROR_Msk);
    }

    return (bsGetStatus() & BS_VERSION_ERROR_Msk) ? -1 : 0;
}
