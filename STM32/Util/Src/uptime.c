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
static void storeUptime();

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static CRC_HandleTypeDef* hcrc = NULL;
static int no_of_channels      = 0;  // Number of channels to track

/* Last software version.  */
static char* last_sw_version = {0};
static CounterChannel* channels = NULL;  // Array of channels

static char* uptime_channel_desc[NUM_DEFAULT_CHANNELS] = {
    "Total board uptime minutes",
    "Minutes since rework",
    "Minutes since last software update",
    "Software failures",
};

static char** custom_channel_desc = NULL;  // Custom channel descriptions, if any

/***************************************************************************************************
** PRIVATE FUNCTIONS
***************************************************************************************************/

/*!
** @brief Load uptime counters from FLASH
**
** Note: last_sw_version is stored contiguously with channels, so they can all be read together
*/
static int loadUptime() {
    // Read in stored uptime after power cycling
    size_t channelsSize = no_of_channels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    return readFromFlashCRC(hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)last_sw_version, channelsSize);
}

/*!
** @brief Save uptime counters to FLASH
**
** Note: last_sw_version is stored contiguously with channels, so they can all be stored together
*/
static void storeUptime() {
    size_t channelsSize = no_of_channels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    (void)writeToFlashCRC(hcrc, (uint32_t)FLASH_ADDR_UPTIME, (uint8_t*)last_sw_version, channelsSize);
}

/***************************************************************************************************
** PUBLIC FUNCTIONS
***************************************************************************************************/

/*!
** @brief Increment the count of a channel
*/
void uptime_incChannel(int ch) {
    if (channels) {
        channels[ch].count++;
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

    if (channels) {
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
            storeUptime();
            bsClearField(BS_FLASH_ONGOING_Msk);
        }
    }
}

/*!
** @brief Reset a channel's count to 0 and increment the reset count
*/
void uptime_resetChannel(int ch) {
    /* channel 0 is the total operating hours for a board and may not be reset */
    if ((ch > 0) && (ch < no_of_channels) && channels) {
        channels[ch].reset_count++;
        channels[ch].count = 0;
    }
}

/*!
** @brief Print the uptime information for all channels
*/
void uptime_print() {
    if (channels) {
        USBnprintf("Name, channel, reset, count");

        for (int i = 0; i < no_of_channels; i++) {
            /* Add a string descriptor of each channel, if one was provided at initialisation */
            char* desc = NULL;
            if (i < NUM_DEFAULT_CHANNELS) {
                desc = uptime_channel_desc[i];
            }
            else if (custom_channel_desc) {
                desc = custom_channel_desc[i - NUM_DEFAULT_CHANNELS];
            }
            else {
                desc = "Custom channel";
            }

            USBnprintf("%s, %" PRIu32 ", %" PRIu32 ", %" PRIu32, desc, channels[i].channel,
                       channels[i].reset_count, channels[i].count);
        }
    }
}

/*!
** @brief Default handler for uptime input commands
*/
void uptime_inputHandler(const char* input) {
    if (strncmp(input, "uptime", 6) == 0) {
        char reset = '\0';
        int ch     = -1;
        int args   = sscanf(input, "uptime %c %d", &reset, &ch);

        if (args < 1) {
            USBnprintf("Start of uptime");
            uptime_print();
            USBnprintf("End of uptime");
        }
        else if (args == 2 && reset == 'r') {
            if (ch > 0 && ch < no_of_channels) {
                uptime_resetChannel(ch);
                USBnprintf("Reset channel %d", ch);
            }
        }
        else if (args == 1 && reset == 's') {
            storeUptime();
        }
        else if (args == 1 && reset == 'l') {
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
** @param _no_of_channels The number of channels to track.
** @param channel_desc Array of channel descriptions, if any.
** @param boot_msg The boot message to check for software failures.
** @param sw_version The current software version (to check for updates)
**
** @return 0 on success, <0 on failure (e.g. -1 for too many channels, -2 for malloc failure).
*/
int uptime_init(CRC_HandleTypeDef* _hcrc, int _no_of_channels, char** channel_desc,
                const char* boot_msg, const char* sw_version) {
    hcrc                = _hcrc;
    custom_channel_desc = channel_desc;

    /* Validate no. of channels is allowed */
    no_of_channels = _no_of_channels + NUM_DEFAULT_CHANNELS;
    if (no_of_channels > MAX_COUNTER_CHANNELS) {
        return -1;
    }

    /* Note: SW_VERSION_MAX_LENGTH must be an integer multiple of uint32 (e.g. 4) */
    size_t channelsSize = no_of_channels * sizeof(CounterChannel) + SW_VERSION_MAX_LENGTH;
    last_sw_version = aligned_alloc(4U, channelsSize);
    channels = (CounterChannel*)(last_sw_version + SW_VERSION_MAX_LENGTH);  // Point to the start of channels

    if (channels == NULL) {
        return -2;
    }

    /* First time programming */
    if (loadUptime() != 0) {
        for (int i = 0; i < no_of_channels; i++) {
            channels[i].channel     = i;
            channels[i].reset_count = 0;
            channels[i].count       = 0;
        }

        strncpy(last_sw_version, sw_version, SW_VERSION_MAX_LENGTH - 1);
        storeUptime();
    }

    /* If the Watchdog has been triggered, this counts as a software failure */
    if (strstr(boot_msg, "Watch dog") != NULL) {
        uptime_incChannel(SW_FAILURES);
    }

    /* If the SW has been updated, reset the channel automatically */
    if (strcmp(sw_version, last_sw_version) != 0) {
        uptime_resetChannel(MINS_SINCE_SW_UPDATE);

        strncpy(last_sw_version, sw_version, SW_VERSION_MAX_LENGTH - 1);
        storeUptime();
    }

    return 0;
}