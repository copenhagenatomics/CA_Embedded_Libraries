/*!
** @file   serialStatus_tests.cpp
** @author Luke W
** @date   16/08/2024
*/

#include <vector>

#include "fake_USBprint.h"
#include "serialStatus_tests.h"
#include "pcbversion.h"

using namespace std;

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
** Programs the board to be the latest non-compatible version and runs. Verifies that the status bit
** has version error and error bit set. TODO: Currently the test will fail if other error bits are 
** also set. Fix this.
*/
void incorrectBoardTest(SerialStatusTest& sst, int firstPrintTick) {
    /* Update OTP with incorrect board number */
    if(BREAKING_MINOR != 0) {
        sst.testFixture->bi.v2.pcbVersion.minor = BREAKING_MINOR - 1;
    }
    else if(BREAKING_MAJOR != 0) {
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
    EXPECT_READ_USB(::testing::Contains(::testing::HasSubstr("0x84")));
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
    sst.testFixture->writeBoardMessage("Status\n");

    vector<const char*> bs_pre = {"\r", 
        "Boot Unit Test\r", 
        "Start of board status:\r", 
        "The board is operating normally.\r"};
    vector<const char*> bs_post = {"\r", 
        "End of board status. \r"
    };

    bs_pre.insert(bs_pre.end(), pass_string.begin(), pass_string.end());
    bs_pre.insert(bs_pre.end(), bs_post.begin(), bs_post.end());

    EXPECT_FLUSH_USB(::testing::ElementsAreArray(bs_pre));
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
