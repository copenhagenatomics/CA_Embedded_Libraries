/*!
 * @file    calibration.c
 * @brief   This file contains the calibration functions
 * @date	DATE
 * @author 	AUTHOR
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FLASH_readwrite.h"
#include "USBprint.h"
#include "calibration.h"
#include "stm32f4xx_hal.h"

// Extern value defined in .ld linker script
extern uint32_t _FlashAddrCal;  // Starting address of calibration values in FLASH
#define FLASH_ADDR_CAL ((uintptr_t)&_FlashAddrCal)

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct _calDef {
    float min;         // Minimum value
    float max;         // Maximum value
    float defaultVal;  // Default value
} calDef_t;

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static bool setCal(const calDef_t* calDef, float* calPointer, float newValue);
static void setDefaultCal(FlashCalibration_t* cal);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

CRC_HandleTypeDef* hcrc_ = NULL;

static char buffer[300];  // Print buffer

// EXAMPLES
static const calDef_t CAL1_DEF = {1e-9, 1e9, 1};
static const calDef_t CAL2_DEF = {1e-9, 1e9, 1};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Applies calibration after checking it is in range
 * @param calDef Pointer to calibration definition
 * @param calPointer Pointer to the calibration value
 * @param newValue Updated value
 * @return 1 if calibration has been set
 */
static bool setCal(const calDef_t* calDef, float* calPointer, float newValue) {
    if ((newValue >= calDef->min) && (newValue <= calDef->max)) {
        *calPointer = newValue;
        return true;
    }
    return false;
}

/*!
 * @brief Applies default initialization
 * @param cal Pointer to the board calibration structure
 */
static void setDefaultCal(FlashCalibration_t* cal) {
    cal->cal1 = CAL1_DEF.defaultVal;
    cal->cal2 = CAL2_DEF.defaultVal;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Calibration function
 * @param noOfCalibrations Number of calibrations
 * @param calibrations Pointer to the CA calibration structure
 * @param cal Pointer to the board calibration structure
 */
void calibrate(int noOfCalibrations, const CACalibration* calibrations, FlashCalibration_t* cal) {
    /*
        Calibration command:
        -------------------------------------------------------------------------
        Sets manually cal1
        CAL 1,cal1

        Sets manually cal2
        CAL 2,cal2,0

        Triggers automatic measurement of cal2
        CAL 2,0,1
        -------------------------------------------------------------------------
    */

    for (int i = 0; i < noOfCalibrations; i++) {
        const CACalibration* c = &calibrations[i];
        bool calOk             = false;  // Is the calibration valid ?

        switch (c->port) {
            case 1:
                calOk = setCal(&CAL1_DEF, &cal->cal1, c->alpha);
                break;
            case 2:
                if (c->noOfValues >= 2) {
                    if (c->beta == 0.0) {
                        calOk = setCal(&CAL2_DEF, &cal->cal2, c->alpha);
                    }
                    else if (c->alpha == 0.0) {
                        // callbackFunction();
                        calOk = true;
                    }
                }
                break;

            default:
                break;
        }
        USBnprintf("CAL %d %s\r\n", c->port, calOk ? "success" : "failed");
    }
}

/*!
 * @brief Calibration initialization function
 * @param hcrc Pointer to the CRC handler
 * @param cal Pointer to the board calibration structure
 * @param size Size of the calibration
 */
void calibrationInit(CRC_HandleTypeDef* hcrc, FlashCalibration_t* cal, uint32_t size) {
    hcrc_ = hcrc;

    if (readFromFlashCRC(hcrc, (uint32_t)FLASH_ADDR_CAL, (uint8_t*)cal, size) != 0) {
        setDefaultCal(cal);
        // Write default calibration to flash if no valid calibration is found
        writeCalToFlash(cal, size);
    }
}

/*!
 * @brief Prints provided calibration
 * @param cal Pointer to the board calibration
 */
void printCal(FlashCalibration_t* cal) {
    int len = 0;

    CA_SNPRINTF(buffer, len, "Calibration: CAL ");
    CA_SNPRINTF(buffer, len, "1,%.2f ", cal->cal1);
    CA_SNPRINTF(buffer, len, "2,%.2f,0\r\n", cal->cal2);

    writeUSB(buffer, len);
}

/*!
 * @brief Reads calibration from flash
 */
void readCalFromFlash() {
    FlashCalibration_t cal;

    if (readFromFlashCRC(hcrc_, (uint32_t)FLASH_ADDR_CAL, (uint8_t*)&cal, sizeof(cal)) != 0) {
        USBnprintf("CAL m failed\r\n");
    }
    else {
        printCal(&cal);
    }
}

/*!
 * @brief Writes calibration into flash
 * @param cal Pointer to the board calibration
 * @param size Calibration size
 */
void writeCalToFlash(FlashCalibration_t* cal, uint32_t size) {
    int ret = writeToFlashCRC(hcrc_, (uint32_t)FLASH_ADDR_CAL, (uint8_t*)cal, size);
    USBnprintf("CAL w %s\r\n", (ret == 0) ? "success" : "failed");
}
