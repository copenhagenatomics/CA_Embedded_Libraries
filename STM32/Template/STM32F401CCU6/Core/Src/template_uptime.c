/*!
 * @file    template_uptime.c
 * @brief   Handling of counters for uptime tracking for Template
 * @date    DATE
 * @author  AUTHOR
 */

#include <stdint.h>
#include <string.h>

#include "FLASH_readwrite.h"
#include "calibration.h"
#include "githash.h"
#include "stm32f4xx_hal.h"
#include "uptime.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

enum templateUptimeChannels {
    CHANNEL_1_MINS = NUM_DEFAULT_CHANNELS,
    CHANNEL_2_MINS,
    CHANNEL_END_ENUM
};

#define NUM_TEMPLATE_CHANNELS (CHANNEL_END_ENUM - NUM_DEFAULT_CHANNELS)

typedef struct {
    uint32_t board_uptime;
    uint32_t channel_1_uptime;
    uint32_t channel_2_uptime;
} legacy_uptime_t;

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static const char* template_channel_desc[NUM_TEMPLATE_CHANNELS] = {"Channel 1 uptime minutes",
                                                                   "Channel 2 uptime minutes"};

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Initializes uptime
 * @param _hcrc CRC handler
 * @param boot_msg Boot message
 */
void initUptime(CRC_HandleTypeDef* _hcrc, const char* boot_msg) {
    /* Verify if legacy uptime counter data is present. Legacy data used just 20 bytes of
    ** flash (+4 CRC bytes). Check byte 25 to see if it is FF or not */
    legacy_uptime_t legacy_uptime = {0};
    uint8_t crc_check             = 0;

    if (readFromFlash((uint32_t)FLASH_ADDR_UPTIME + 25, &crc_check, 1) == 0 && crc_check == 0xFF) {
        /* Legacy data found - store to convert later. uptime_init() will wipe old data due to CRC
        ** fail */
        readFromFlashCRC(_hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)&legacy_uptime,
                         sizeof(legacy_uptime));
    }

    (void)uptime_init(_hcrc, NUM_TEMPLATE_CHANNELS, template_channel_desc, boot_msg, GIT_VERSION);

    /* Should only be true if legacy uptime information is present */
    if (crc_check == 0xFF) {
        /* The flash should now have been upgraded to the new uptime format */
        uptime_setChannel(TOTAL_BOARD_MINS, legacy_uptime.board_uptime);
        uptime_setChannel(MINS_SINCE_REWORK, legacy_uptime.board_uptime);
        uptime_setChannel(CHANNEL_1_MINS, legacy_uptime.channel_1_uptime);
        uptime_setChannel(CHANNEL_1_MINS, legacy_uptime.channel_2_uptime);
        uptime_store();
    }
}

/*!
 * @brief Uptime loop (called in while(1))
 */
void loopUptime() {
    static uint32_t timestamp_s0 = 0;
    static uint32_t timestamp_s1 = 0;

    // CONDITIONS TO BE CHANGED
    if (1) {
        timestamp_s0 = uptime_incChannelMinutes(CHANNEL_1_MINS, timestamp_s0);
    }
    if (1) {
        timestamp_s0 = uptime_incChannelMinutes(CHANNEL_2_MINS, timestamp_s1);
    }

    uptime_update();
}
