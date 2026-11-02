/*!
 * @file    Template_tests.cpp
 * @brief   Unit tests for Template project
 * @date    DATE
 * @author  AUTHOR
 */

/* Linker script variables */
#include <inttypes.h>
extern "C" {
uint32_t _FlashAddrCal    = 0;
uint32_t _FlashAddrUptime = 0;
}

/* CA unit tests */
#include "caBoardUnitTests.h"
#include "serialStatus_tests.h"

/* Fakes */

/* Real supporting units */
#include "ADCmonitor.c"
#include "CAProtocol.c"
#include "CAProtocolStm.c"
#include "calibration.c"
#include "crc.c"
#include "template_uptime.c"
#include "time32.c"
#include "uptime.c"

/* UUT */
#include "template.c"

using namespace std;

/***************************************************************************************************
** OUTPUT STRINGS
***************************************************************************************************/

// Default flash calibration values, before any CAL command has been sent
static const string CAL_DEFAULT = "CAL 1,1.00 2,1.00,0\r";

// Calibration string containing only valid calibrations
static const string CAL_VALID = "CAL 1,2.00 2,2.00,0\r";

// Calibration string containing only invalid calibrations
static const string CAL_INVALID = "CAL 1,-1.00 2,1.00,0\r";

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class TemplateTests : public CaBoardUnitTest {
   protected:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/

    TemplateTests() : CaBoardUnitTest(&templateLoop, Template, {LATEST_MAJOR, LATEST_MINOR}) {
        hadc1.Init.NbrOfConversion = ADC_CHANNELS;
    }

    void simTick() {
        // ADC buffer should be half full every 100 ms
        if (tickCounter != 0) {
            if (tickCounter % 200 == 0) {
                HAL_ADC_ConvCpltCallback(&hadc1);
            }
            else if (tickCounter % 200 == 100) {
                HAL_ADC_ConvHalfCpltCallback(&hadc1);
            }
        }
        templateLoop(bootMsg);
    }

    /*******************************************************************************************
    ** MEMBERS
    *******************************************************************************************/

    TIM_HandleTypeDef htim2 = {.Instance = TIM2};
    ADC_HandleTypeDef hadc1 = {0};
    CRC_HandleTypeDef hcrc  = {.Instance = CRC};

    SerialStatusTest sst = {.boundInit   = bind(templateInit, &htim2, &hadc1, &hcrc, bootMsg),
                            .testFixture = this};
};

/***************************************************************************************************
** GENERIC CA TESTS
***************************************************************************************************/

TEST_F(TemplateTests, incorrectBoard) {
    incorrectBoardTest(sst);
}

TEST_F(TemplateTests, incorrectBoardVersion) {
    incorrectBoardVersionTest(sst);
}

// clang-format off
TEST_F(TemplateTests, goldenPath) {
    goldenPathTest(sst,
        "0x00000000\r",
        100);
}

TEST_F(TemplateTests, printStatus) {
    statusPrintoutTest(sst, {
        "The board is operating normally.\r",
        // "State 1: OFF\r",
        // "State 2: OFF\r"
    });
}

TEST_F(TemplateTests, printStatusDef) {
    statusDefPrintoutTest(sst, "0x7e000000,System errors\r", {
        // "0x00000001,State 1\r",
        // "0x00000002,State 2\r"
    });
}

TEST_F(TemplateTests, printOutputDef) {
    outputDefPrintoutTest(sst, {
        "Channel 1,Unit1\r",
        "Channel 2,Unit2\r"
    });
}
// clang-format on

TEST_F(TemplateTests, printSerial) {
    string calStr = "Calibration: " + CAL_DEFAULT;
    serialPrintoutTest(sst, "Template", calStr.c_str());
}

TEST_F(TemplateTests, incorrectCommand) {
    incorrectCommandTest(sst);
}

TEST_F(TemplateTests, upTime) {
    uptimeTest(sst, (uintptr_t)&_FlashAddrUptime);
}

TEST_F(TemplateTests, calibration) {
    calibrationTest(sst, CAL_DEFAULT.c_str(), CAL_VALID.c_str(), CAL_INVALID.c_str());
}
