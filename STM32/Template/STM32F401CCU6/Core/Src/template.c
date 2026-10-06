/*!
 * @file    template.c
 * @brief   This file contains the main program of template
 * @date    DATE
 * @author  AUTHOR
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ADCMonitor.h"
#include "CAProtocol.h"
#include "CAProtocolStm.h"
#include "USBprint.h"
#include "calibration.h"
#include "main.h"
#include "pcbversion.h"
#include "stm32f4xx_hal.h"
#include "systemInfo.h"
#include "template.h"
#include "template_uptime.h"
#include "uptime.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

// ADC (internal + 3 external)
#define ADC_CHANNELS         1     // Number of ADC channels used on the STM32
#define ADC_CHANNEL_BUF_SIZE 400   // 4 kHz sampling  rate -> 10 Hz
#define ANALOG_REF_VOLTAGE   3.3f  // V
#define ADC_RATIO            (ANALOG_REF_VOLTAGE / 4096)

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void printUptime(const char* input);
static void printHeader();
static void printStatus();
static void printStatusDef();
static void printOutputDef();
static void calibrate(int noOfCalibrations, const CACalibration* calibrations);
static void calibrateRW(bool write);
static void userInput(const char* input);

static void adcToFloat(int16_t* pData);
static void adcCallback(int16_t* pData, int noOfChannels, int noOfSamples);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

// Print buffer shared
static char buff[600];

// Circular buffer
static int16_t ADCBuffer[ADC_CHANNELS * ADC_CHANNEL_BUF_SIZE * 2];

// Calibration
static FlashCalibration_t cal;

// CA protocol handling
static CAProtocolCtx caProto = {.undefined        = userInput,
                                .printHeader      = printHeader,
                                .printStatus      = printStatus,
                                .printStatusDef   = printStatusDef,
                                .printOutputDef   = printOutputDef,
                                .jumpToBootLoader = HALJumpToBootloader,
                                .calibration      = calibrate,
                                .calibrationRW    = calibrateRW,
                                .logging          = NULL,
                                .otpRead          = CAotpRead,
                                .otpWrite         = NULL,
                                .uptime           = printUptime};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Interface with uptime feature
 */
static void printUptime(const char* input) {
    uptime_inputHandler(input, printHeader);
}

/*!
 * @brief Definition of what is printed when the 'Serial' command is received
 */
static void printHeader() {
    CAPrintHeader();
    calibrateRW(false);
}

/*!
 * @brief   Definition of status information when the 'Status' command is received
 */
static void printStatus() {
    int len = 0;

    // CA_SNPRINTF(buff, len, "State 1: %s\r\n", bsGetField(BS_STATE1_Msk) ? "YES" : "NO");
    // CA_SNPRINTF(buff, len, "State 2: %s\r\n", bsGetField(BS_STATE2_Msk) ? "YES" : "NO");

    writeUSB(buff, len);
}

/*!
 * @brief   Definition of status information when the 'StatusDef' command is received
 */
static void printStatusDef() {
    int len = 0;

    // CA_SNPRINTF(buff, len, "0x%08" PRIx32 ",State 1\r\n", (uint32_t)BS_STATE1_Msk);
    // CA_SNPRINTF(buff, len, "0x%08" PRIx32 ",State 2\r\n", (uint32_t)BS_STATE2_Msk);

    writeUSB(buff, len);
}

/*!
 * @brief Definition of output printed when the 'OutputDef' command is received
 */
static void printOutputDef() {
    int len = 0;

    CA_SNPRINTF(buff, len, "Channel 1,Unit1\r\n");
    CA_SNPRINTF(buff, len, "Channel 2,Unit2\r\n");

    writeUSB(buff, len);
}

/*!
 * @brief Applies the calibration sent by the user
 * @param noOfCalibrations Number of calibrations
 * @param calibrations Pointer to the calibration structure
 */
static void calibrate(int noOfCalibrations, const CACalibration* calibrations) {
    calibration(noOfCalibrations, calibrations, &cal, sizeof(cal));
}

/*!
 * @brief Printing of the calibration information when the 'Serial' command is received
 * @param write Write directly into the flash memory
 */
static void calibrateRW(bool write) {
    calibrationRW(write, &cal, sizeof(cal));
}

/*!
 * @brief   Implementation of the user input
 * @param   input Pointer to the message received
 */
static void userInput(const char* input) {
    float userValue;

    if (strncmp(input, "command1", 8) == 0) {
        // ACTION
    }
    else if (sscanf(input, "command2 %f", &userValue) == 1) {
        if ((userValue > 0.0f) && (userValue < 100.0f)) {
            // ACTION
        }
    }
    else {
        HALundefined(input);
    }
}

/*!
 * @brief Converts ADC into physical values
 * @param pData ADC buffer
 */
static void adcToFloat(int16_t* pData) {
    // ADC TO FLOAT
}

/*!
 * @brief Callback function called when buffer is half-full or full
 * @param pData ADC buffer
 * @param noOfChannels Number of ADC channels
 * @param noOfSamples Number of samples per channel
 */
static void adcCallback(int16_t* pData, int noOfChannels, int noOfSamples) {
    if (!isUsbPortOpen()) {
        return;
    }

    if (bsGetField(BS_VERSION_ERROR_Msk)) {
        USBnprintf("0x%08" PRIx32 "\r\n", bsGetStatus());
        return;
    }

    adcToFloat(pData);

    int len = 0;

    // CA_SNPRINTF(buff, len, "%0.2f, ", value1);
    // CA_SNPRINTF(buff, len, "%0.2f, ", value2);
    CA_SNPRINTF(buff, len, "0x%08" PRIx32 "\r\n", bsGetStatus());

    writeUSB(buff, len);
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Initialization function
 * @param adcTim Timer triggering the ADC DMA
 * @param hadc ADC handler
 * @param hcrc CRC handler
 */
void templateInit(TIM_HandleTypeDef* adcTim, ADC_HandleTypeDef* hadc, CRC_HandleTypeDef* hcrc,
                  const char* bootMsg) {
    // Enables communcation
    initCAProtocol(&caProto, usbRx);

    // Starts the internal ADC
    HAL_TIM_Base_Start(adcTim);
    ADCMonitorInit(hadc, ADCBuffer, sizeof(ADCBuffer) / sizeof(ADCBuffer[0]));

    // Board type and PCB version check
    if (boardSetup(Template, (pcbVersion){BREAKING_MAJOR, BREAKING_MINOR}, TEMPLATE_ERRORS_Msk) !=
        0) {
        return;
    }

    // Calibration
    calibrationInit(hcrc, &cal, sizeof(cal));

    // Uptime
    (void)initUptime(hcrc, bootMsg);

    // BOARD SPECIFIC INIT CODE
}

/*!
 * @brief Loop function called in while(1)
 * @param bootMsg Boot message
 */
void templateLoop(const char* bootMsg) {
    // Always allow DFU upload
    CAhandleUserInputs(&caProto, bootMsg);

    // Handles internal ADC
    ADCMonitorLoop(adcCallback);

    // Version check
    if (bsGetField(BS_VERSION_ERROR_Msk)) {
        return;
    }

    // Handles uptime
    (void)loopUptime();

    // BOARD SPECIFIC MAIN LOOP CODE
}
