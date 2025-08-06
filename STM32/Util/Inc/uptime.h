/*!
 * @file    uptime.h
 * @brief   See uptime.c
 * @date    22/07/2025
 * @author  Luke Walker
 */

#ifndef INC_UPTIME_H_
#define INC_UPTIME_H_

#include <stdint.h>

#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

extern uint32_t _FlashAddrUptime;  // Variable defined in ld linker script.
#define FLASH_ADDR_UPTIME ((uintptr_t)&_FlashAddrUptime)

typedef struct CounterChannel {
    uint32_t channel;      // Channel number
    uint32_t reset_count;  // Number of times the channel has been reset
    uint32_t count;        // Current count value
} CounterChannel;

#define MAX_COUNTER_CHANNELS 38 /* Max channels that can fit in a 0x200 byte heap, rounded down */

enum defaultChannels {
    TOTAL_BOARD_MINS = 0,
    MINS_SINCE_REWORK,
    MINS_SINCE_SW_UPDATE,
    SW_FAILURES,
    NUM_DEFAULT_CHANNELS
};

/***************************************************************************************************
** PUBLIC FUNCTION PROTOTYPES
***************************************************************************************************/

int uptime_init(CRC_HandleTypeDef* _hcrc, int _no_of_channels, char** channel_desc,
                const char* boot_msg, const char* sw_version);
void uptime_incChannel(int ch);
uint32_t uptime_incChannelMinutes(int ch, uint32_t lastUpdate);
void uptime_update();
void uptime_resetChannel(int ch);
void uptime_print();
void uptime_inputHandler(const char* input);

#endif /* INC_UPTIME_H_ */