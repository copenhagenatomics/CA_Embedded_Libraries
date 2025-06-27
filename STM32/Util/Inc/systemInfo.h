/*!
 * @file    systemInfo.h
 * @brief   Header file of systemInfo.c
 * @date    25/03/2025
 * @author  Timothé D
 */

#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

/* General Board Status register definitions */

#define BS_ERROR_Pos 31U
#define BS_ERROR_Msk (1UL << BS_ERROR_Pos)

#define BS_OVER_TEMPERATURE_Pos 30U
#define BS_OVER_TEMPERATURE_Msk (1UL << BS_OVER_TEMPERATURE_Pos)

#define BS_UNDER_VOLTAGE_Pos 29U
#define BS_UNDER_VOLTAGE_Msk (1UL << BS_UNDER_VOLTAGE_Pos)

#define BS_OVER_VOLTAGE_Pos 28U
#define BS_OVER_VOLTAGE_Msk (1UL << BS_OVER_VOLTAGE_Pos)

#define BS_OVER_CURRENT_Pos 27U
#define BS_OVER_CURRENT_Msk (1UL << BS_OVER_CURRENT_Pos)

#define BS_VERSION_ERROR_Pos 26U
#define BS_VERSION_ERROR_Msk (1UL << BS_VERSION_ERROR_Pos)

#define BS_USB_ERROR_Pos 25U
#define BS_USB_ERROR_Msk (1UL << BS_USB_ERROR_Pos)

#define BS_FLASH_ONGOING_Pos 24U
#define BS_FLASH_ONGOING_Msk (1UL << BS_FLASH_ONGOING_Pos)

#define BS_100_HZ_OUTPUT_Pos 23U
#define BS_100_HZ_OUTPUT_Msk (1UL << BS_100_HZ_OUTPUT_Pos)

/* Used for defining which bits are errors, and which are statuses */
#define BS_SYSTEM_ERRORS_Msk                                                                      \
    (BS_OVER_TEMPERATURE_Msk | BS_UNDER_VOLTAGE_Msk | BS_OVER_VOLTAGE_Msk | BS_OVER_CURRENT_Msk | \
     BS_VERSION_ERROR_Msk | BS_USB_ERROR_Msk)

/***************************************************************************************************
** STRUCTURES
***************************************************************************************************/

// NOTE!! Do not change order or values since this list must match ALL OTP programmers.

typedef enum {
    AC_Board           = 1,
    DC_Board           = 2,
    Temperature        = 3,
    Current            = 4,
    GasFlow            = 5,
    HumidityChip       = 6,
    Pressure           = 7,
    SaltFlowBoard      = 8,
    SaltLeak           = 9,
    HotValveController = 10,
    ZrO2Oxygen         = 11,
    AMBcurrent         = 12,
    Geiger             = 13,
    AirCondition       = 14,
    LightController    = 15,
    LiquidLevel        = 16,
    ER                 = 17,
    ERHC               = 18,
    VFD                = 19,
    Tachometer         = 20,
    ACTenChannel       = 21,
    PhaseMonitor       = 22,
    SaltLeakCal        = 23,
    PressureCal        = 24,
    ERUHC              = 25,
    FanController      = 26
} BoardType;
typedef uint8_t SubBoardType;  // SubBoardType needed for some boards.

typedef struct {
    uint8_t major;
    uint8_t minor;
} pcbVersion;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

const char* systemInfo();
const char* statusInfo(bool printStart);
const char* statusDefInfo(bool printStart);
int getBoardInfo(BoardType* bdt, SubBoardType* sbdt);
int getPcbVersion(pcbVersion* ver);

void bsSetErrorRange(uint32_t field, uint32_t range);
void bsSetFieldRange(uint32_t field, uint32_t range);
void bsSetError(uint32_t field);
void bsClearError(uint32_t field);
void bsSetField(uint32_t field);
void bsClearField(uint32_t field);
void bsUpdateField(uint32_t field, bool set);
void bsUpdateError(uint32_t field, bool set, uint32_t error_bits);
uint32_t bsGetStatus();
uint32_t bsGetField(uint32_t field);

void setBoardTemp(float temp);
void setBoardVoltage(float voltage);
void setBoardCurrent(float current);
void setBoardUsbError(uint32_t err);
void setFirmwareBoardType(BoardType type);
void setFirmwareBoardVersion(pcbVersion version);

int boardSetup(BoardType type, pcbVersion breaking_version, uint32_t boardErrorsMsk);

#ifdef __cplusplus
}
#endif

#endif  // SYSTEM_INFO_H
