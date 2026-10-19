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

// EXAMPLES
static const calDef_t CAL1_DEF = {1e-9, 1.0e9, 1};
static const calDef_t CAL2_DEF = {1e-9, 1.0e9, 1};

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
    if ((newValue >= calDef->min) && (newValue < calDef->max)) {
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
 * @param size Size of the calibration
 */
void calibration(int noOfCalibrations, const CACalibration* calibrations, FlashCalibration_t* cal,
                 uint32_t size) {
    /*
        Calibration command:
        -------------------------------------------------------------------------
        CAL 1,cal1,cal2
        -------------------------------------------------------------------------
    */

    for (int i = 0; i < noOfCalibrations; i++) {
        int port     = calibrations[i].port;
        double alpha = calibrations[i].alpha;
        double beta  = calibrations[i].beta;
        bool calOk   = false;  // Is the calibration valid ?

        switch (port) {
            case 1:
                calOk = setCal(&CAL1_DEF, &cal->cal1, alpha);
                calOk &= setCal(&CAL2_DEF, &cal->cal2, beta);
                break;

            default:
                break;
        }
        if (!calOk) {
            USBnprintf("CAL error with port %d\r\n", port);
        }
    }
    calibrationRW(true, cal, size);
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
    }
}

/*!
 * @brief Read and write calibration function
 * @param write Write into the flash memory
 * @param cal Pointer to the board calibration structure
 * @param size Size of the calibration
 */
void calibrationRW(bool write, FlashCalibration_t* cal, uint32_t size) {
    if (write) {
        if (writeToFlashCRC(hcrc_, (uint32_t)FLASH_ADDR_CAL, (uint8_t*)cal, size) != 0) {
            USBnprintf("Calibration was not stored in FLASH\r\n");
        }
    }
    else {
        char buffer[300];
        int len = 0;

        CA_SNPRINTF(buffer, len, "Calibration: CAL");
        CA_SNPRINTF(buffer, len, " 1,%.2f,%.2f\r\n", cal->cal1, cal->cal2);

        writeUSB(buffer, len);
    }
}
