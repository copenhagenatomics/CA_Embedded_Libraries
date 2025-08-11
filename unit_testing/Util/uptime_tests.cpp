/*!
** @file   uptime_tests.cpp
** @author Luke W
** @date   11/08/2025
*/

#include <string>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/* Fakes */
#include "fake_stm32xxxx_hal.h"
#include "fake_USBprint.h"

/* Real supporting units */
#include "CAProtocol.c"
#include "CAProtocolStm.c"

/* For memory writes */
extern "C" {
    uint32_t _FlashAddrUptime = 0;
}

/* UUT */
#include "uptime.c"

using namespace std;
using ::testing::Contains;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::HasSubstr;
using ::testing::IsSupersetOf;

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct {
    uint8_t sw[16];
    CounterChannel channels[4];
} uptime_data_t;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class TestUptime: public ::testing::Test
{
    protected: 
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        TestUptime() {
            forceTick(0);
        }

        /*******************************************************************************************
        ** MEMBERS
        *******************************************************************************************/

        CRC_HandleTypeDef hcrc;
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(TestUptime, testInitGolden) {
    /* Attempt to init with too many channels */
    EXPECT_EQ(-1, uptime_init(&hcrc, MAX_COUNTER_CHANNELS + 1, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* TODO: Force alloc to return NULL? */

    /* Successful init of 1 additional channels */
    EXPECT_EQ(0, uptime_init(&hcrc, 1, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* Blank memory means channels should be initialised to 0 */
    uptime_print();
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 0\r",
        "Minutes since rework, 1, 0, 0\r",
        "Minutes since last software update, 2, 0, 0\r",
        "Software failures, 3, 0, 0\r",
        "Custom channel, 4, 0, 0"
    ));


}

TEST_F(TestUptime, testInitExistingChannels) {
    /* Write flash memory with some existing data */
    uptime_data_t prevUptime = {
        .sw = "V1.0.0",
        .channels = {
            {0, 0, 100},
            {1, 0, 200},
            {2, 0, 300},
            {3, 0, 400}
        }
    };
    writeToFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&prevUptime, sizeof(prevUptime));

    /* Init with 0 channels */
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* Channels should be initialised */
    uptime_print();
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 100\r",
        "Minutes since rework, 1, 0, 200\r",
        "Minutes since last software update, 2, 0, 300\r",
        "Software failures, 3, 0, 400"
    ));
}

TEST_F(TestUptime, testInitSwError) {
    /* Init with 0 channels */
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Hardware Watch dog", "V1.0.0"));

    /* Channels should be initialised */
    uptime_print();
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 0\r",
        "Minutes since rework, 1, 0, 0\r",
        "Minutes since last software update, 2, 0, 0\r",
        "Software failures, 3, 0, 1"
    ));
}

TEST_F(TestUptime, testInitSwVersion) {
    /* Write flash memory with some existing data */
    uptime_data_t prevUptime = {
        .sw = "V1.0.0",
        .channels = {
            {0, 0, 100},
            {1, 0, 200},
            {2, 0, 300},
            {3, 0, 400}
        }
    };
    writeToFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&prevUptime, sizeof(prevUptime));

    /* Init with 1 channels */
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Power On", "V2.0.0"));

    /* Channels should be initialised */
    uptime_print();
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 100\r",
        "Minutes since rework, 1, 0, 200\r",
        "Minutes since last software update, 2, 1, 0\r",
        "Software failures, 3, 0, 400"
    ));

    /* Verify the new version has been stored in flash correctly */
    readFromFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&prevUptime, sizeof(prevUptime));
    EXPECT_STREQ("V2.0.0", (char*)prevUptime.sw);
}

TEST_F(TestUptime, testChannelOps) {
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* Check incrementing each channel */
    for(int i = 0; i < 4; i++) {
        uptime_incChannel(i);

        uptime_print();
        string test = to_string(i) + ", 0, 1";
        EXPECT_FLUSH_USB(Contains(HasSubstr(test)));
    }

    /* Check incrementing minutes */
    for(int i = 0; i < 4; i++) {
        forceTick(60000);  // Force a tick of 1 minute
        EXPECT_EQ(60000, uptime_incChannelMinutes(i, 0)); 

        uptime_print();
        string test = to_string(i) + ", 0, 2";
        EXPECT_FLUSH_USB(Contains(HasSubstr(test)));
    }

    /* Check resetting each channel */
    for(int i = 0; i < 4; i++) {
        uptime_resetChannel(i);
        uptime_print();

        /* Channel 0 is not allowed to be reset */
        if(i == 0) {
            EXPECT_FLUSH_USB(Contains("Total board uptime minutes, 0, 0, 2\r"));
        }
        else {
            string test = to_string(i) + ", 1, 0";
            EXPECT_FLUSH_USB(Contains(HasSubstr(test)));
        }
    }
}

TEST_F(TestUptime, testUpdate) {
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* Update should do nothing without a time shift */
    uptime_update();
    uptime_print();
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 0\r",
        "Minutes since rework, 1, 0, 0\r",
        "Minutes since last software update, 2, 0, 0\r",
        "Software failures, 3, 0, 0"
    ));

    /* After 1 minute, the time channels should be incremented */
    forceTick(60000);
    uptime_update();
    uptime_print();
    EXPECT_FLUSH_USB(IsSupersetOf({
        "Total board uptime minutes, 0, 0, 1\r",
        "Minutes since rework, 1, 0, 1\r",
        "Minutes since last software update, 2, 0, 1\r"
    }));

    /* Flash memory should not have been updated */
    uptime_data_t uptime_data = {
        .sw = "      ",
        .channels = {
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1},
            {1, 1, 1}
        }
    };
    readFromFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&uptime_data, sizeof(uptime_data));
    for(int i = 0; i < 4; i++) {
        EXPECT_EQ(i, uptime_data.channels[i].channel);
        EXPECT_EQ(0, uptime_data.channels[i].count);
    }

    /* After 1 day, the time channels should be incremented (only once due to nature of test) and 
    ** saved */
    forceTick(1000 * 60 * 60 * 24);
    uptime_update();
    uptime_print();
    EXPECT_FLUSH_USB(IsSupersetOf({
        "Total board uptime minutes, 0, 0, 2\r",
        "Minutes since rework, 1, 0, 2\r",
        "Minutes since last software update, 2, 0, 2\r"
    }));

    readFromFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&uptime_data, sizeof(uptime_data));
    for(int i = 0; i < 3; i++) {
        EXPECT_EQ(i, uptime_data.channels[i].channel);
        EXPECT_EQ(2, uptime_data.channels[i].count);
    }
}

TEST_F(TestUptime, testInputHandler) {
    EXPECT_EQ(0, uptime_init(&hcrc, 0, NULL, "reconnected Reset Reason: Power On", "V1.0.0"));

    /* Print */
    uptime_inputHandler("uptime");
    EXPECT_FLUSH_USB(ElementsAre("\r",
        "Start of uptime\r",
        "Name, channel, reset, count\r",
        "Total board uptime minutes, 0, 0, 0\r",
        "Minutes since rework, 1, 0, 0\r",
        "Minutes since last software update, 2, 0, 0\r",
        "Software failures, 3, 0, 0\r",
        "End of uptime"
    ));

    /* Reset Channel */
    uptime_incChannel(1);
    uptime_print();
    EXPECT_FLUSH_USB(Contains("Minutes since rework, 1, 0, 1\r"));
    uptime_inputHandler("uptime r 1");
    uptime_print();
    EXPECT_FLUSH_USB(Contains("Minutes since rework, 1, 1, 0\r"));

    /* Load from memory */
    uptime_inputHandler("uptime l");
    uptime_print();
    EXPECT_FLUSH_USB(Contains("Minutes since rework, 1, 0, 0\r"));

    /* Store to memory */
    uptime_incChannel(1);
    uptime_inputHandler("uptime s");

    uptime_data_t uptime_data = {0};
    readFromFlashCRC(&hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&uptime_data, sizeof(uptime_data));
    EXPECT_STREQ("V1.0.0", (char*)uptime_data.sw);
    EXPECT_EQ(1, uptime_data.channels[1].count);
}