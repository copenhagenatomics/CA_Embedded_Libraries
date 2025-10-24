/*!
** @file   serialStatus_tests.cpp
** @author Luke W
** @date   16/08/2024
*/

#include <vector>
#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "fake_USBprint.h"
#include "fake_stm32xxxx_hal.h"
#include "pcbversion.h"
#include "serialStatus_tests.h"
#include "FLASH_readwrite.h"

/* Uptime won't compile properly if CRC not enabled */
#ifdef HAL_CRC_MODULE_ENABLED
    #include "uptime.h"
#endif

using namespace std;
using ::testing::Contains;
using ::testing::AllOf;

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Parses the final hex value from a comma-separated string to an int
*/
uint32_t parseFinalHex(const std::string& input) {
    // Find last comma
    std::size_t lastComma = input.find_last_of(',');
    std::string lastVar;

    if (lastComma == std::string::npos) {
        // No comma means whole string is the variable
        lastVar = input;
    } 
    else {
        lastVar = input.substr(lastComma + 1);
    }

    // Trim whitespace from both ends
    auto trim = [](std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    };
    trim(lastVar);

    // Convert hex string (expecting format "0x1234...")
    uint32_t value = 0;
    try {
        value = std::stoul(lastVar, nullptr, 16);
    } 
    catch (const std::exception& e) {
        std::cerr << "Error parsing hex string: " << e.what() << std::endl;
        throw;
    }

    return value;
}

/***************************************************************************************************
** TESTS
***************************************************************************************************/

/*!
** @brief Tests the golden path startup for a CA board
**
** @param[in] sst            Test data object
** @param[in] pass_string    The expected string printed for normal startup
** @param[in] firstPrintTick The tick at which the first successful print should be made. 100 ms by 
**                           default
**
** Initialises the board, runs until the requested tick and verifies the string is printed. External
** influences, e.g. ADC data should be filled before this test if the printout depends on it.
**
** Note: when filling ADC data, the board should be initialised first, the ADC data filled, then 
** this test called:
**
** 1. boardInit();
** 2. fillAdcData();
** 3. goldenPathTest(); //Performs re-initialisation
*/
void goldenPathTest(SerialStatusTest& sst, const char* pass_string, int firstPrintTick) {
    sst.boundInit();

    /* This should force a print on the USB bus */
    sst.testFixture->goToTick(firstPrintTick);

    /* Check the printout is correct */
    EXPECT_READ_USB(::testing::Contains(pass_string));
}

/*!
** @brief Tests for if the code is downloaded to the wrong board
**
** @param[in] sst            Test data object
** @param[in] firstPrintTick The tick at which the first successful print should be made. 100 ms by 
**                           default
**
** Programs the board to be a different board type and runs. Verifies that the status bit has 
** version error bit set
*/
void incorrectBoardTest(SerialStatusTest& sst, int firstPrintTick) {
    /* Update OTP with wrong board ID */
    if(sst.testFixture->bi.v2.boardType == 0) {
        sst.testFixture->bi.v2.boardType = 1;
    }
    else {
        sst.testFixture->bi.v2.boardType = 0;
    }

    HAL_otpWrite(&(sst.testFixture->bi));

    sst.boundInit();

    /* This should force a print on the USB bus */
    sst.testFixture->goToTick(firstPrintTick);

    /* Check the printout is correct */
    vector<string> ss = hostUSBread();

    /* Get the final element of the most recent message, strip any whitespace, parse as an integer
    ** and check the error bit */
    string final = ss.back();
    uint32_t status = parseFinalHex(final);
    EXPECT_TRUE(status & BS_VERSION_ERROR_Msk);
}

/*!
** @brief Tests for if the code is downloaded to the wrong board version
**
** @param[in] sst            Test data object
** @param[in] firstPrintTick The tick at which the first successful print should be made. 100 ms by 
**                           default
**
** Programs the board to be the latest non-compatible version and runs. Verifies that the status bit
** has version error and error bit set.
*/
void incorrectBoardVersionTest(SerialStatusTest& sst, int firstPrintTick) {
    /* Update OTP with incorrect board number */
    if(BREAKING_MINOR != 0) {
        sst.testFixture->bi.v2.pcbVersion.minor = BREAKING_MINOR - 1;
        sst.testFixture->bi.v2.pcbVersion.major = BREAKING_MAJOR;
    }
    else if(BREAKING_MAJOR != 0) {
        sst.testFixture->bi.v2.pcbVersion.minor = 0;
        sst.testFixture->bi.v2.pcbVersion.major = BREAKING_MAJOR - 1;
    }
    else {
        FAIL() << "Oldest PCB Version must be at least v0.1";
    }
    
    HAL_otpWrite(&(sst.testFixture->bi));

    sst.boundInit();

    /* This should force a print on the USB bus */
    sst.testFixture->goToTick(firstPrintTick);

    /* Check the printout is correct */
    vector<string> ss = hostUSBread();

    /* Get the final element of the most recent message, strip any whitespace, parse as an integer
    ** and check the error bit */
    string final = ss.back();
    uint32_t status = parseFinalHex(final);
    EXPECT_TRUE(status & BS_VERSION_ERROR_Msk);
}

/*!
** @brief Tests that the status printout matches the correct format
**
** @param[in] sst         Test data object
** @param[in] pass_string The board specific part of the status printout
**
** Initialises the board then sends the 'Status' command. Checks the response matches the protocol 
** template and the supplied board specific string.
*/
void statusPrintoutTest(SerialStatusTest& sst, vector<const char*> pass_string) {
    sst.boundInit();
    /* Note: usb RX buffer is flushed during the first loop, so a single loop must be done before
    ** printing anything */
    sst.testFixture->_loopFunction(sst.testFixture->bootMsg);
    (void) hostUSBread(true);

    sst.testFixture->writeBoardMessage("Status\n");

    vector<const char*> bs_pre = {"\r", 
        "Start of board status:\r"};
    vector<const char*> bs_post = {"\r", 
        "End of board status. \r"
    };

    bs_pre.insert(bs_pre.end(), pass_string.begin(), pass_string.end());
    bs_pre.insert(bs_pre.end(), bs_post.begin(), bs_post.end());

    EXPECT_FLUSH_USB(::testing::ElementsAreArray(bs_pre));
}

/*!
** @brief Tests that the status definition printout matches the correct format
**
** @param[in] sst         Test data object
** @param[in] pass_string The board specific part of the status definition printout
**
** Initialises the board then sends the 'StatusDef' command. Checks the response matches the protocol 
** template and the supplied board specific string.
*/
void statusDefPrintoutTest(SerialStatusTest& sst, const char* boardErrorsString, vector<const char*> boardStatusDefString) {
    sst.boundInit();
    /* Note: usb RX buffer is flushed during the first loop, so a single loop must be done before
    ** printing anything */
    sst.testFixture->_loopFunction(sst.testFixture->bootMsg);
    sst.testFixture->writeBoardMessage("StatusDef\n");

    vector<const char*> bsdPre1 = {"\r",
        "Boot Unit Test\r",
        "Start of board status definition:\r",
    };
    vector<const char*> bsdPre2 = {
            "0x80000000,Error\r",
            "0x40000000,Over temperature\r",
            "0x20000000,Under voltage\r",
            "0x10000000,Over voltage\r",
            "0x08000000,Over current\r",
            "0x04000000,Version error\r",
            "0x02000000,USB error\r",
            "0x01000000,Flash ongoing\r",
            "0x00800000,100Hz Output\r",
    };
    vector<const char*> bsdPost = {"\r",
        "End of board status definition.\r"
    };

    bsdPre1.push_back(boardErrorsString);
    bsdPre1.insert(bsdPre1.end(), bsdPre2.begin(), bsdPre2.end());
    bsdPre1.insert(bsdPre1.end(), boardStatusDefString.begin(), boardStatusDefString.end());
    bsdPre1.insert(bsdPre1.end(), bsdPost.begin(), bsdPost.end());

    EXPECT_FLUSH_USB(::testing::ElementsAreArray(bsdPre1));
}

/*!
** @brief Tests that the serial printout matches the correct format
**
** @param[in] sst         Test data object
** @param[in] boardName   The name of the board as a string
** @param[in] pass_string The board specific calibration string. Optional, default: Null.
**
** Initialises the board then sends the 'Serial' command. Checks the response matches the protocol 
** template and the supplied board specific calibration string.
*/
void serialPrintoutTest(SerialStatusTest& sst, const char* boardName, const char* cal_string) {
    sst.boundInit();

    /* Note: usb RX buffer is flushed during the first loop, so a single loop must be done before
    ** printing anything */
    sst.testFixture->_loopFunction(sst.testFixture->bootMsg);
    sst.testFixture->writeBoardMessage("Serial\n");

    /* The basic printout doesn't change on a board-to-board version */
    vector<const char *> pass_string = {
        "\r", 
        "Boot Unit Test\r", 
        "Serial Number: 000\r", 
        "PT",
        "Sub Product Type: 0\r", 
        "MCU Family: Unknown 0x  0 Rev 0\r", 
        "Software Version: 0\r", 
        "Compile Date: 0\r", 
        "Git SHA: 0\r",
        "PB",
        "CA"
    };

    string pt_string = "Product Type: " + string(boardName) + "\r";
    string pb_string = "PCB Version: " + to_string(LATEST_MAJOR) + "." + to_string(LATEST_MINOR) + "\r";
    pass_string[3] = pt_string.c_str();
    pass_string[9] = pb_string.c_str();

    if(cal_string) {
        pass_string[10] = cal_string;
    }
    else {
        pass_string.pop_back();
    }

    EXPECT_FLUSH_USB(::testing::ElementsAreArray(pass_string));
}

#ifdef HAL_CRC_MODULE_ENABLED
    /*!
    ** @brief Tests that the uptime counting functionality works correctly
    **
    ** @param[in] sst         Test data object
    ** @param[in] flashAddr   The address in flash memory where uptime information is stored
    **
    ** Initialises the board, and simulates one day of uptime. Checks that the output of the "uptime" 
    ** command is as expected. Checks that "uptime r" correctly resets a channel count, and finally 
    ** checks the data is stored in flash memory correctly.
    */
    void uptimeTest(SerialStatusTest& sst, uintptr_t flashAddr) {
        // Initialise board
        sst.boundInit();

        uint32_t tickUpdateVal   = 60000;  // 1 minute of tick (ms)
        uint32_t minutes_per_day = 1440;

        for (uint32_t i = 0; i <= minutes_per_day; i++) {
            // Increment ticker value 1 min at a time until it has reached one day of run time
            forceTick(tickUpdateVal * i);
            sst.testFixture->_loopFunction(sst.testFixture->bootMsg);
            sst.testFixture->writeBoardMessage("uptime\n");
            usbFlush();

            /* Test reset channel functionality halfway through */
            if(i == 720) {
                // Reset one channel to test reset count
                sst.testFixture->writeBoardMessage("uptime r 2\n");
            }

            int j = i;
            int r = 0;
            if(i > 720) {
                j = i - 720;
                r = 1;
            }

            // Check the board total time updates every minute, i = number of minutes simulated
            EXPECT_FLUSH_USB(AllOf(
                Contains("Total board uptime minutes, 0, 0, " + std::to_string(i) + "\r"),
                Contains("Minutes since last software update, 2, " + std::to_string(r) + ", " + std::to_string(j) + "\r")));
        }

        /* Read from FLASH and see that it has been stored. Don't use CRC as it may not be valid, 
        ** depending on how many custom channels are implemented for the board */
        struct {
            uint8_t reserved[16U]; /* Space for SW string */
            CounterChannel total_uptime_channel;
        } uptime_test = {0};
        (void)readFromFlash((uint32_t)flashAddr, (uint8_t*)&uptime_test, sizeof(uptime_test));
        EXPECT_EQ(uptime_test.total_uptime_channel.count, 1440);
    }
#endif
