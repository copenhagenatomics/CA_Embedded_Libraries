#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

/* General Board Status register definitions */
#define BS_ERROR_Pos                        0U
#define BS_ERROR_Msk                        (1UL << BS_ERROR_Pos)

#define BS_OVER_TEMPERATURE_Pos             1U
#define BS_OVER_TEMPERATURE_Msk             (1UL << BS_OVER_TEMPERATURE_Pos)

#define BS_UNDER_VOLTAGE_Pos                2U
#define BS_UNDER_VOLTAGE_Msk                (1UL << BS_UNDER_VOLTAGE_Pos)

#define BS_OVER_VOLTAGE_Pos                 3U
#define BS_OVER_VOLTAGE_Msk                 (1UL << BS_OVER_VOLTAGE_Pos)

#define BS_OVER_CURRENT_Pos                 4U
#define BS_OVER_CURRENT_Msk                 (1UL << BS_OVER_CURRENT_Pos)


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
    ER              = 17
} BoardType;
typedef uint8_t SubBoardType; // SubBoardType needed for some boards.

typedef struct pcbVersion {
    uint8_t major;
    uint8_t minor;
} pcbVersion;

/* Generic info about PCB and git build system/data.
 * @return info about system in null terminated string. */
const char* systemInfo();

// Description: Get Boardinfo. If input values are NULL these are ignored.
// @param bdt:  Boardtype used in the SW running
// @param sbdt: SubBoardtype used in the SW running, if 0xFF value is ignore in compare
// Return 0 if data is valid, else negative value.
int getBoardInfo(BoardType* bdt, SubBoardType* sbdt);

// Description: get the PCB version of the board.
// @param ver: Pointer to struct pcbVersion
// Return 0 if data is valid, else negative value.
int getPcbVersion(pcbVersion* ver);

// Description: set a board status field.
// @param field: a 1 bit shifted to the field index to be set.
void bsSetField(uint32_t field);

// Description: set a board status field.
// @param field: a 1 bit shifted to the field index to be cleared.
void bsClearField(uint32_t field);

// Description: Get the board status registry.
// Return a uint32_t containing the board status.
uint32_t bsGetStatus();

void setBoardTemp(float temp);
void setBoardUnderVoltage(float voltage);
void setBoardOverVoltage(float voltage);
void setBoardCurrent(float current);

#endif // SYSTEM_INFO_H
