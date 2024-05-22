#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* General Board Status register definitions */
#define BS_ERROR_Pos                        31U
#define BS_ERROR_Msk                        (1UL << BS_ERROR_Pos)

#define BS_OVER_TEMPERATURE_Pos             30U
#define BS_OVER_TEMPERATURE_Msk             (1UL << BS_OVER_TEMPERATURE_Pos)

#define BS_UNDER_VOLTAGE_Pos                29U
#define BS_UNDER_VOLTAGE_Msk                (1UL << BS_UNDER_VOLTAGE_Pos)

#define BS_OVER_VOLTAGE_Pos                 28U
#define BS_OVER_VOLTAGE_Msk                 (1UL << BS_OVER_VOLTAGE_Pos)

#define BS_OVER_CURRENT_Pos                 27U
#define BS_OVER_CURRENT_Msk                 (1UL << BS_OVER_CURRENT_Pos)

#define BS_VERSION_ERROR_Pos                26U
#define BS_VERSION_ERROR_Msk                (1UL << BS_VERSION_ERROR_Pos)

#define BS_USB_ERROR_Pos                    25U
#define BS_USB_ERROR_Msk                    (1UL << BS_USB_ERROR_Pos)

/* Used for defining which bits are errors, and which are statuses */
#define BS_SYSTEM_ERRORS_Msk                (BS_OVER_TEMPERATURE_Msk | BS_UNDER_VOLTAGE_Msk | \
                                             BS_OVER_VOLTAGE_Msk | BS_OVER_CURRENT_Msk | \
                                             BS_VERSION_ERROR_Msk | BS_USB_ERROR_Msk)

// NOTE!! Do not change order or values since this list must match ALL OTP programmers.
typedef enum {
    AC_Board          = 1,
    DC_Board          = 2,
    Temperature       = 3,
    Current           = 4,
    GasFlow           = 5,
    HumidityChip      = 6,
    Pressure          = 7,
    SaltFlowBoard     = 8,
    SaltLeak          = 9,
    HotValveController = 10,
	ZrO2Oxygen 		= 11,
	AMBcurrent 		= 12,
	Geiger 			= 13,
	AirCondition 	= 14,
	LightController = 15,
    LiquidLevel     = 16,
    ER              = 17,
    ERHC            = 18,
    VFD             = 19,
    Tachometer      = 20
} BoardType;
typedef uint8_t SubBoardType; // SubBoardType needed for some boards.

typedef struct {
    uint8_t major;
    uint8_t minor;
} pcbVersion;

/* Generic info about PCB and git build system/data.
 * @return info about system in null terminated string. */
const char* systemInfo();

/* Generic info about system status.
 * @return info about system in null terminated string. */
const char* statusInfo(bool printStart);

// Description: Get Boardinfo. If input values are NULL these are ignored.
// @param bdt:  Boardtype used in the SW running
// @param sbdt: SubBoardtype used in the SW running, if 0xFF value is ignore in compare
// Return 0 if data is valid, else negative value.
int getBoardInfo(BoardType* bdt, SubBoardType* sbdt);

// Description: get the PCB version of the board.
// @param ver: Pointer to struct pcbVersion
// Return 0 if data is valid, else negative value.
int getPcbVersion(pcbVersion * ver);

// Description: set a board status field.
// @param field: a 1 bit shifted to the field index to be set in addition to
// the error bit also being set.
// @param range: the bits in the range parameter is reset prior to setting the
// bits in the field parameter.
void bsSetErrorRange(uint32_t field, uint32_t range);

// Description: set a board status field.
// @param field: a 1 bit shifted to the field index to be set.
// @param range: the bits in the range parameter is reset prior to setting the
// bits in the field parameter.
void bsSetFieldRange(uint32_t field, uint32_t range);

// Description: Set a board status field, and the error bit
// @param field: a 1 bit shifted to the field index. Can be OR'd together
void bsSetError(uint32_t field);

// Description: Clear the error bit, if none of the bits in "field" are set
// @param field: a 1 bit shifted to the field index. Can be OR'd together
void bsClearError(uint32_t field);

// Description: Set a board status field.
// @param field: a 1 bit shifted to the field index to be set. Can be OR'd together
void bsSetField(uint32_t field);

// Description: set a board status field.
// @param field: a 1 bit shifted to the field index to be cleared.
void bsClearField(uint32_t field);

// Description: Get the board status registry.
// Return a uint32_t containing the board status.
uint32_t bsGetStatus();

// Description: get a board status field.
// @param field: a 1 bit shifted to the field index.
uint32_t bsGetField(uint32_t field);

void setBoardTemp(float temp);
void setBoardVoltage(float voltage);
void setBoardCurrent(float current);
void setBoardUsbError(uint32_t err);

/*!
** @brief Sets the board type, the firmware is expecting to be used with
*/
void setFirmwareBoardType(BoardType type);

/*!
** @brief Sets the board version, the firmware is expecting to be used with
*/
void setFirmwareBoardVersion(pcbVersion version);

/*!
** @brief Sets the board type and version the firmware was compiled for
*/
int boardSetup(BoardType type, pcbVersion breaking_version);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_INFO_H
