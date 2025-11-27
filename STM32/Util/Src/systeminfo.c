/*!
 * @file    systemInfo.h
 * @brief   Generic functions used for the STM system
 * @date    25/03/2025
 * @author  Timothé D
 */

#include <inttypes.h>
#include <stdio.h>

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

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// F4xx UID
#ifndef UNIT_TESTING
#define ID1 *((unsigned long*)(UID_BASE))
#define ID2 *((unsigned long*)(UID_BASE + 4U))
#define ID3 *((unsigned long*)(UID_BASE + 8U))
#else
#define ID1 (unsigned long)0U
#define ID2 (unsigned long)0U
#define ID3 (unsigned long)0U

#define GIT_VERSION "0"
#define GIT_DATE    "0"
#define GIT_SHA     "0"
#endif

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static const char* mcuType();
static const char* productType(uint8_t id);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

/*
Struct containing the board status sw registry
as well as some of the general values.
A board should update the values in this struct using the
functions defined below for updating the board status.
*/
static struct BS {
    uint32_t boardErrorsMsk;
    uint32_t boardStatus;
    float temp;
    float voltage;
    float current;
    uint32_t usb;
    BoardType boardType;
    pcbVersion pcb_version;
} BS = {0};

// Print buffer for systemInfo, statusInfo and statusDefInfo
static char buf[600] = {0};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Gives MCU type
 * @return  MCU type as a string
 */
static const char* mcuType() {
    static char mcu[50] = {0};  // static to prevent allocate on stack.
    int len             = 0;

#ifndef UNIT_TESTING
    const DBGMCU_TypeDef* mcuType = DBGMCU;
    const uint16_t idCode         = 0x00000FFF & mcuType->IDCODE;
    const uint16_t revCode        = 0xFFFF & (mcuType->IDCODE >> 16);
#else
    const uint16_t idCode  = 0;
    const uint16_t revCode = 0;
#endif

    switch (idCode) {
        case 0x423:
            len += snprintf(&mcu[len], sizeof(mcu) - len, "STM32F401xB/C");
            break;
        case 0x433:
            len += snprintf(&mcu[len], sizeof(mcu) - len, "STM32F401xD/E");
            break;
        case 0x450:
            len += snprintf(&mcu[len], sizeof(mcu) - len, "STM32H753IIT6");
            break;
        default:
            len += snprintf(&mcu[len], sizeof(mcu) - len, "Unknown 0x%3X", idCode);
            break;
    }

    len += snprintf(&mcu[len], sizeof(mcu) - len, " Rev %x", revCode);

    return mcu;
}

/*!
 * @brief   Gives product type
 * @return  Board type as a string
 */
static const char* productType(uint8_t id) {
    switch (id) {
        case AC_Board:
            return "AC Board";
        case DC_Board:
            return "DC Board";
        case Temperature:
            return "Temperature";
        case Current:
            return "Current";
        case GasFlow:
            return "GasFlow";
        case HumidityChip:
            return "HumidityChip";
        case Pressure:
            return "Pressure";
        case SaltFlowBoard:
            return "Salt Flow Board";
        case SaltLeak:
            return "SaltLeak";
        case HotValveController:
            return "HotValveController";
        case ZrO2Oxygen:
            return "ZrO2Oxygen";
        case AMBcurrent:
            return "AMBcurrent";
        case Geiger:
            return "Geiger";
        case AirCondition:
            return "AirCondition";
        case LightController:
            return "LightController";
        case LiquidLevel:
            return "LiquidLevel";
        case ER:
            return "ER";
        case ERHC:
            return "ERHC";
        case VFD:
            return "VFD";
        case Tachometer:
            return "Tachometer";
        case ACTenChannel:
            return "ACTenChannel";
        case PhaseMonitor:
            return "PhaseMonitor";
        case SaltLeakCal:
            return "SaltLeakCal";
        case PressureCal:
            return "PressureCal";
        case ERUHC:
            return "ERUHC";
        case FanController:
            return "FanController";
        case AnalogInput:
            return "AnalogInput";
    }
    return "NA";
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Generic info about PCB and git build system/data
 * @return  Info about system in null terminated string
 */
const char* systemInfo() {
    BoardInfo info = {0};

    if (HAL_otpRead(&info) != OTP_SUCCESS) {
        info.otpVersion = 0;  // Invalid OTP version
    }

    int len = 0;
    len     = snprintf(&buf[len], sizeof(buf) - len, "Serial Number: %lX%lX%lX\r\n", ID1, ID2, ID3);
    switch (info.otpVersion) {
        case OTP_VERSION_1:
            len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n",
                            productType(info.v1.boardType));
            break;
        case OTP_VERSION_2:
            len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: %s\r\n",
                            productType(info.v2.boardType));
            len += snprintf(&buf[len], sizeof(buf) - len, "Sub Product Type: %u\r\n",
                            info.v2.subBoardType);
            break;
        default:
            len += snprintf(&buf[len], sizeof(buf) - len, "Product Type: NA\r\n");
            break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "MCU Family: %s\r\n", mcuType());
    len += snprintf(&buf[len], sizeof(buf) - len, "Software Version: %s\r\n", GIT_VERSION);
    len += snprintf(&buf[len], sizeof(buf) - len, "Compile Date: %s\r\n", GIT_DATE);
    len += snprintf(&buf[len], sizeof(buf) - len, "Git SHA: %s\r\n", GIT_SHA);
    switch (info.otpVersion) {
        case OTP_VERSION_1:
            len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d",
                            info.v1.pcbVersion.major, info.v1.pcbVersion.minor);
            break;
        case OTP_VERSION_2:
            len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: %d.%d",
                            info.v2.pcbVersion.major, info.v2.pcbVersion.minor);
            break;
        default:
            len += snprintf(&buf[len], sizeof(buf) - len, "PCB Version: NA");
            break;
    }
    len += snprintf(&buf[len], sizeof(buf) - len, "\r\n");

    return buf;
}

/*!
 * @brief   Generic info about system status
 * @param   printStart Select beginning or end of print
 * @return  Info about system in null terminated string
 */
const char* statusInfo(bool printStart) {
    int len = 0;

    // Print end of message and return
    if (!printStart) {
        len += snprintf(&buf[len], sizeof(buf) - len, "\r\nEnd of board status. \r\n");
        return buf;
    }

    len += snprintf(&buf[len], sizeof(buf) - len, "\r\nStart of board status:\r\n");
    if (!(BS.boardStatus & BS_ERROR_Msk)) {
        len += snprintf(&buf[len], sizeof(buf) - len, "The board is operating normally.\r\n");
        return buf;
    }

    if (BS.boardStatus & BS_OVER_TEMPERATURE_Msk) {
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "Over temperature. The board temperature is %.2fC.\r\n", BS.temp);
    }

    if (BS.boardStatus & BS_UNDER_VOLTAGE_Msk) {
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "Under voltage. The board operates at too low voltage of %.2fV. Check "
                        "power supply.\r\n",
                        BS.voltage);
    }

    if (BS.boardStatus & BS_OVER_VOLTAGE_Msk) {
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "Over voltage. The board operates at too high voltage of %.2fV. Check "
                        "power supply.\r\n",
                        BS.voltage);
    }

    if (BS.boardStatus & BS_OVER_CURRENT_Msk) {
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "Over current. One of the ports has reached a current out of its "
                        "measurement range at %.2fA.\r\n",
                        BS.current);
    }

    if (BS.boardStatus & BS_VERSION_ERROR_Msk) {
        BoardType bt  = (BoardType)0;
        pcbVersion pv = {0};
        (void)getBoardInfo(&bt, NULL);
        (void)getPcbVersion(&pv);
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "Error: Incorrect Version.\r\n"
                        "   Board is: %d.\r\n"
                        "   Board should be: %d.\r\n"
                        "   PCB Version is: %d.%d.\r\n"
                        "   PCB Version should be > %d.%d.\r\n",
                        (int)bt, (int)BS.boardType, pv.major, pv.minor, BS.pcb_version.major,
                        BS.pcb_version.minor);
    }

    if (BS.usb) {
        len += snprintf(&buf[len], sizeof(buf) - len,
                        "USB. USB communication error 0x%08" PRIx32 " occurred most recently.\r\n",
                        BS.usb);
        BS.usb = 0U;
    }

    return buf;
}

/*!
 * @brief   Generic info about system status definition
 * @param   printStart Select beginning or end of print
 * @return  Info about system status definition in null terminated string
 */
const char* statusDefInfo(bool printStart) {
    int len = 0;

    // Print end of message and return
    if (!printStart) {
        len += snprintf(&buf[len], sizeof(buf) - len, "\r\nEnd of board status definition.\r\n");
        return buf;
    }

    len += snprintf(&buf[len], sizeof(buf) - len, "\r\nStart of board status definition:\r\n");

    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",System errors\r\n",
                    (uint32_t)BS.boardErrorsMsk);
    len +=
        snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Error\r\n", (uint32_t)BS_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over temperature\r\n",
                    (uint32_t)BS_OVER_TEMPERATURE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Under voltage\r\n",
                    (uint32_t)BS_UNDER_VOLTAGE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over voltage\r\n",
                    (uint32_t)BS_OVER_VOLTAGE_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Over current\r\n",
                    (uint32_t)BS_OVER_CURRENT_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Version error\r\n",
                    (uint32_t)BS_VERSION_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",USB error\r\n",
                    (uint32_t)BS_USB_ERROR_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",Flash ongoing\r\n",
                    (uint32_t)BS_FLASH_ONGOING_Msk);
    len += snprintf(&buf[len], sizeof(buf) - len, "0x%08" PRIx32 ",100Hz Output\r\n",
                    (uint32_t)BS_100_HZ_OUTPUT_Msk);
    

    return buf;
}

/*!
 * @brief   Gets Boardinfo. If input values are NULL these are ignored
 * @param   bdt Boardtype used in the SW running
 * @param   sbdt SubBoardtype used in the SW running, if 0xFF value is ignore in compare
 * @return  0 if data is valid, else negative value
 */
int getBoardInfo(BoardType* bdt, SubBoardType* sbdt) {
    BoardInfo info = {0};
    if (HAL_otpRead(&info) != OTP_SUCCESS) {
        return -1;
    }

    if (info.otpVersion == OTP_VERSION_1) {
        if (bdt) {
            *bdt = (BoardType)info.v1.boardType;
        }
        if (sbdt) {
            *sbdt = 0;
        }  // Sub board type is not included in version 1
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2) {
        if (bdt) {
            *bdt = (BoardType)info.v2.boardType;
        }
        if (sbdt) {
            *sbdt = info.v2.subBoardType;
        }
        return 0;
    }

    return -1;  // New OTP version. i.e. this SW is to old.
}

/*!
 * @brief   Gets the PCB version of the board
 * @param   ver Pointer to struct pcbVersion
 * @return  0 if data is valid, else negative value
 */
int getPcbVersion(pcbVersion* ver) {
    BoardInfo info = {0};
    if (ver == 0 || HAL_otpRead(&info) != OTP_SUCCESS) {
        return -1;
    }

    if (info.otpVersion == OTP_VERSION_1) {
        ver->major = info.v1.pcbVersion.major;
        ver->minor = info.v1.pcbVersion.minor;
        return 0;
    }

    if (info.otpVersion == OTP_VERSION_2) {
        ver->major = info.v2.pcbVersion.major;
        ver->minor = info.v2.pcbVersion.minor;
        return 0;
    }

    return -1;  // New OTP version. i.e. this SW is to old.
}

/*!
 * @brief   Sets a board status error range
 * @param   field A 1 bit shifted to the field index to be set in addition to the error bit also
 * being set
 * @param   range The bits in the range parameter are reset prior to setting the bits in the field
 * parameter
 */
void bsSetErrorRange(uint32_t field, uint32_t range) {
    /*
    For ranges of bits where only a subset of them are being set,
    then setting a new subset will result in the union of the two subset.
    Since, for a range it is assumed that the bits in the range
    are related i.e. each combination meaning a unique state, then
    we need to reset previous state before setting the new state.
    */
    BS.boardStatus &= ~range;
    BS.boardStatus |= (BS_ERROR_Msk | field);
}

/*!
 * @brief   Sets a board status field
 * @param   field A 1 bit shifted to the field index to be set
 * @param   range The bits in the range parameter are reset prior to setting the bits in the field
 * parameter
 */
void bsSetFieldRange(uint32_t field, uint32_t range) {
    // Reset bits before setting the new value of the range for the same
    // reason as described in the bsSetErrorRange function.
    BS.boardStatus &= ~range;
    BS.boardStatus |= field;
}

/*!
 * @brief   Sets a board status field, and the error bit
 * @param   field A 1 bit shifted to the field index to be set. Can be OR'd together
 */
void bsSetError(uint32_t field) { BS.boardStatus |= (BS_ERROR_Msk | field); }

/*!
 * @brief   Clears the error bit, if none of the bits in "field" are set
 * @param   field A 1 bit shifted to the field index to be set. Can be OR'd together
 */
void bsClearError(uint32_t field) {
    if (!(BS.boardStatus & field)) {
        bsClearField(BS_ERROR_Msk);
    }
}

/*!
 * @brief   Sets a board status field
 * @param   field A 1 bit shifted to the field index to be set. Can be OR'd together
 */
void bsSetField(uint32_t field) { BS.boardStatus |= field; }

/*!
 * @brief   Clears a board status field
 * @param   field A 1 bit shifted to the field index to be cleared
 */
void bsClearField(uint32_t field) { BS.boardStatus &= ~field; }

/*!
 * @brief   Updates a field using a bool to determine whether set or clear
 * @param   field Field to set/clear
 * @param   set Whether to Set or Clear the field
 */
void bsUpdateField(uint32_t field, bool set) {
    if (set) {
        bsSetField(field);
    }
    else {
        bsClearField(field);
    }
}

/*!
 * @brief   Updates an error field using a bool to determine whether set or clear
 * @param   field Field to set/clear
 * @param   set Whether to Set or Clear the field
 * @param   error_bits A collection of all error bits. Used to determine if the master error bit
 * should be set or not
 */
void bsUpdateError(uint32_t field, bool set, uint32_t error_bits) {
    if (set) {
        bsSetError(field);
    }
    else {
        bsClearField(field);
        bsClearError(error_bits);
    }
}

/*!
 * @brief   Gets the board status registry
 * @return  uint32_t containing the board status
 */
uint32_t bsGetStatus() { return BS.boardStatus; }

/*!
 * @brief   Gets a board status field
 * @param   field A 1 bit shifted to the field index
 * @return  uint32_t containing the board status field
 */
uint32_t bsGetField(uint32_t field) { return BS.boardStatus & field; }

/*!
 * @brief   Sets temperature
 * @param   temp Temperature in degC
 */
void setBoardTemp(float temp) { BS.temp = temp; }

/*!
 * @brief   Sets voltage
 * @param   voltage Voltage in V
 */
void setBoardVoltage(float voltage) { BS.voltage = voltage; }

/*!
 * @brief   Sets current
 * @param   current Current in A
 */
void setBoardCurrent(float current) { BS.current = current; }

/*!
 * @brief   Sets USB error
 * @param   err USB error
 */
void setBoardUsbError(uint32_t err) { BS.usb = err; }

/*!
 * @brief   Sets the board type, the firmware is expecting to be used with
 * @param   type Board type
 */
void setFirmwareBoardType(BoardType type) { BS.boardType = type; }

/*!
 * @brief   Sets the board version, the firmware is expecting to be used with
 * @param   version Board version
 */
void setFirmwareBoardVersion(pcbVersion version) { BS.pcb_version = version; }

/*!
 * @brief   Sets the board type and version the firmware was compiled for
 * @note    If the board type/version does not match the value programmed in the OTP, this function
 * sets the board status version error flag
 * @param   type Type of board, e.g. AC, DC, Current ...
 * @param   breaking_version Oldest version of PCB this firmware can run on
 * @param   boardErrorsMsk Board error mask (with board specific bits)
 * @return  0 if OK, -1 if version error
 */
int boardSetup(BoardType type, pcbVersion breaking_version, uint32_t boardErrorsMsk) {
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
    else if (ver.major == breaking_version.major && ver.minor < breaking_version.minor) {
        bsSetError(BS_VERSION_ERROR_Msk);
    }

    BS.boardErrorsMsk = BS_SYSTEM_ERRORS_Msk | boardErrorsMsk;

    return (bsGetStatus() & BS_VERSION_ERROR_Msk) ? -1 : 0;
}
