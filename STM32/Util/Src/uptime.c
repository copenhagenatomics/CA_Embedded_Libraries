/*!
 * @file    uptime.c
 * @brief   Generic handling of counters for uptime tracking
 * @date    22/07/2025
 * @author  Luke Walker
 */

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAProtocol.h"
#include "CAProtocolStm.h"
#include "FLASH_readwrite.h"
#include "USBprint.h"
#include "systemInfo.h"
#include "time32.h"
#include "uptime.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define UPDATE_INTERVAL_SESSION 60000     // Update states every minute (ms per minute)
#define UPDATE_INTERVAL_FLASH   86400000  // Store states in FLASH every day (ms per day)

/* Should be enough characters for hash-dirty (e.g. 2c4dff2-dirty) + \0 termination */
#define SW_VERSION_MAX_LENGTH 16

/***************************************************************************************************
** PRIVATE FUNCTION PROTOTYPES
***************************************************************************************************/

static int loadUptime();

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static CRC_HandleTypeDef* hcrc = NULL;
static int noOfChannels        = 0;  // Number of channels to track

/* Last software version.  */
static char* lastSwVersion      = {0};
static CounterChannel* channels = NULL;  // Array of channels

static const char* uptimeChannelDesc[NUM_DEFAULT_CHANNELS] = {
    "Total board uptime minutes",
    "Minutes since rework",
    "Minutes since last software update",
    "Software failures",
};

static const char** customChannelDesc = NULL;  // Custom channel descriptions, if any

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Load uptime counters from FLASH
**
** Note: lastSwVersion is stored contiguously with channels, so they can all be read together
*/
static int loadUptime() {
    // Read in stored uptime after power cycling
    uint32_t channelsSize = noOfChannels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    return readFromFlashCRC(hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)lastSwVersion,
                            channelsSize);
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
** @brief Save uptime counters to FLASH
**
** Note: lastSwVersion is stored contiguously with channels, so they can all be stored together
*/
void uptime_store() {
    uint32_t channelsSize = noOfChannels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    (void)writeToFlashCRC(hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)lastSwVersion, channelsSize);
}

/*!
** @brief Increment the count of a channel
*/
void uptime_incChannel(int ch) {
    if (channels && (ch >= 0) && (ch < noOfChannels)) {
        channels[ch].count++;
    }
}

/*!
** @brief Set the count of a channel to a particular value (used for transferring legacy data)
*/
void uptime_setChannel(int ch, uint32_t count) {
    if (channels && (ch >= 0) && (ch < noOfChannels)) {
        channels[ch].count = count;
    }
}

/*!
** @brief Increments the count of a specific channel every minute.
**
** @param ch The channel to update.
** @param lastUpdate The last time this channel was updated.
**
** @return The new timestamp if the channel was updated, otherwise lastUpdate
*/
uint32_t uptime_incChannelMinutes(int ch, uint32_t lastUpdate) {
    uint32_t now = 0;

    if (channels && (ch >= 0) && (ch < noOfChannels)) {
        now = HAL_GetTick();
        if (tdiff_u32(now, lastUpdate) >= UPDATE_INTERVAL_SESSION) {
            lastUpdate = now;
            channels[ch].count++;
        }
    }

    return lastUpdate;
}

/*!
** @brief Saves all the channel counters to FLASH on a daily basis.
*/
void uptime_update() {
    static uint32_t timestamp_save   = 0;
    static uint32_t timestamp_update = 0;

    if (channels) {
        /* Update the default minute timers */
        uint32_t now = HAL_GetTick();
        if (tdiff_u32(now, timestamp_update) >= UPDATE_INTERVAL_SESSION) {
            timestamp_update = now;
            channels[TOTAL_BOARD_MINS].count++;
            channels[MINS_SINCE_REWORK].count++;
            channels[MINS_SINCE_SW_UPDATE].count++;
        }

        if (tdiff_u32(now, timestamp_save) >= (UPDATE_INTERVAL_FLASH - 1000)) {
            bsSetField(BS_FLASH_ONGOING_Msk);
        }

        if (tdiff_u32(now, timestamp_save) >= UPDATE_INTERVAL_FLASH) {
            timestamp_save = now;
            uptime_store();
            bsClearField(BS_FLASH_ONGOING_Msk);
        }
    }
}

/*!
** @brief Reset a channel's count to 0 and increment the reset count
*/
void uptime_resetChannel(int ch) {
    /* channel 0 is the total operating minutes for a board and may not be reset */
    if ((ch > 0) && (ch < noOfChannels) && channels) {
        channels[ch].resetCount++;
        channels[ch].count = 0;
    }
}

/*!
** @brief Print the uptime information for all channels
*/
void uptime_print() {
    if (channels) {
        USBnprintf("Name, channel, reset, count\r\n");

        for (int i = 0; i < noOfChannels; i++) {
            /* Add a string descriptor of each channel, if one was provided at initialisation */
            const char* desc = NULL;
            if (i < NUM_DEFAULT_CHANNELS) {
                desc = uptimeChannelDesc[i];
            }
            else if (customChannelDesc) {
                desc = customChannelDesc[i - NUM_DEFAULT_CHANNELS];
            }
            else {
                desc = "Custom channel";
            }

            USBnprintf("%s, %" PRIu32 ", %" PRIu32 ", %" PRIu32 "\r\n", desc, channels[i].channel,
                       channels[i].resetCount, channels[i].count);
        }
    }
}

/*!
** @brief Default handler for uptime input commands
*/
void uptime_inputHandler(const char* input, void (*serialPrint)(void)) {
    if (strncmp(input, "uptime", 6) == 0) {
        char action = '\0';
        int ch      = -1;
        int args    = sscanf(input, "uptime %c %d", &action, &ch);

        if (args < 1) {
            USBnprintf("Start of uptime\r\n");
            if (serialPrint) {
                serialPrint();
            }
            uptime_print();
            USBnprintf("End of uptime\r\n");
        }
        else if (args == 2 && action == 'r') {
            if (ch > 0 && ch < noOfChannels) {
                uptime_resetChannel(ch);
                USBnprintf("Reset channel %d\r\n", ch);
            }
        }
        else if (args == 1 && action == 's') {
            uptime_store();
        }
        else if (args == 1 && action == 'l') {
            loadUptime();
        }
    }
    else {
        HALundefined(input);
    }
}

/*!
**  @brief Initialises the uptime counters and reads in the stored values from FLASH.
**
** @param _hcrc Pointer to the CRC handle.
** @param _noOfChannels The number of channels to track.
** @param channelDesc Array of channel descriptions, if any.
** @param bootMsg The boot message to check for software failures.
** @param swVersion The current software version (to check for updates)
**
** @return 0 on success, <0 on failure (e.g. -1 for too many channels, -2 for malloc failure).
*/
int uptime_init(CRC_HandleTypeDef* _hcrc, int _noOfChannels, const char** channelDesc,
                const char* bootMsg, const char* swVersion) {
    hcrc              = _hcrc;
    customChannelDesc = channelDesc;

    /* Validate no. of channels is allowed */
    noOfChannels = _noOfChannels + NUM_DEFAULT_CHANNELS;
    if (noOfChannels > MAX_COUNTER_CHANNELS) {
        return -1;
    }

    /* Note: SW_VERSION_MAX_LENGTH must be an integer multiple of uint32 (e.g. 4) */
    size_t channelsSize = noOfChannels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    lastSwVersion       = (char*)aligned_alloc(4U, channelsSize);
    if (lastSwVersion == NULL) {
        return -2;
    }

    /* First 16-bytes reserved for last SW version */
    channels = (CounterChannel*)(lastSwVersion + SW_VERSION_MAX_LENGTH);

    /* First time programming / CRC corruption */
    if (loadUptime() != 0) {
        for (int i = 0; i < noOfChannels; i++) {
            channels[i].channel    = i;
            channels[i].resetCount = 0;
            channels[i].count      = 0;
        }

        strncpy(lastSwVersion, swVersion, SW_VERSION_MAX_LENGTH - 1);
        uptime_store();
    }

    /* If the Watchdog has been triggered, this counts as a software failure */
    if (strstr(bootMsg, "Watch dog") != NULL) {
        uptime_incChannel(SW_FAILURES);
    }

    /* If the SW has been updated, reset the channel automatically */
    if (strcmp(swVersion, lastSwVersion) != 0) {
        uptime_resetChannel(MINS_SINCE_SW_UPDATE);

        strncpy(lastSwVersion, swVersion, SW_VERSION_MAX_LENGTH - 1);
        uptime_store();
    }

    return 0;
}
