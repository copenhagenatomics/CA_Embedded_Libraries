/*!
 * @file    W5500.h
 * @brief   Header file of W5500.c
 * @date    18/09/2024
 * @author  Timothé D
*/

#ifndef INC_W5500_H_
#define INC_W5500_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct
{
    uint8_t mac[6];  ///< Source Mac Address
    uint8_t ip[4];   ///< Source IP Address
    uint8_t sn[4];   ///< Subnet Mask 
    uint8_t gw[4];   ///< Gateway IP Address
} netInfo_t;

#define _WIZCHIP_SOCK_NUM_  8

#define TCP_BUF_LEN    200                      // Maximum number of characters per TCP message

typedef struct {
    SPI_HandleTypeDef *hspi;                    // Pointer to SPI handler
    StmGpio select;                             // Chip select pin
    netInfo_t netInfo;                        // Network parameters

    char recvBuf[TCP_BUF_LEN];                  // Micro-controller rx buffer
    char *sendBuf;                              // Micro-controller tx buffer
    bool newADCReady;                           // Becomes true when a line is ready (10 Hz)
    bool newMessage;                            // Becomes true when new command is received
    uint32_t timeStamp;                         // Timestamp of last command

    uint16_t sock_any_port;
    uint16_t sock_io_mode;
    uint16_t sock_is_sending;
    uint16_t sock_remained_size[_WIZCHIP_SOCK_NUM_];
} ethernetHandler_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int W5500Init(ethernetHandler_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, netInfo_t netInfo, char *sendBuf);
int W5500TCPServer(ethernetHandler_t *heth);

#endif /* INC_W5500_H_ */
