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
    (void)uptime_init(_hcrc, NUM_TEMPLATE_CHANNELS, template_channel_desc, boot_msg, GIT_VERSION);
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
