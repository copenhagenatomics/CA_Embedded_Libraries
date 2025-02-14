/*!
 * @file    ethernet.h
 * @brief   Header file of ethernet.c
 * @date    18/09/2024
 * @author  Timothé D
*/

#ifndef INC_ETHERNET_H_
#define INC_ETHERNET_H_

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx_hal.h"
#include "StmGpio.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef enum
{
    NETINFO_STATIC = 1,    ///< Static IP configuration by manually.
    NETINFO_DHCP           ///< Dynamic IP configruation from a DHCP sever
}dhcp_mode;

typedef struct wiz_NetInfo_t
{
    uint8_t mac[6];  ///< Source Mac Address
    uint8_t ip[4];   ///< Source IP Address
    uint8_t sn[4];   ///< Subnet Mask 
    uint8_t gw[4];   ///< Gateway IP Address
    uint8_t dns[4];  ///< DNS server IP Address
    dhcp_mode dhcp;  ///< 1 - Static, 2 - DHCP
}wiz_NetInfo;

typedef struct
{
    uint8_t dns[4];
    dhcp_mode dhcp;
} static_wizchip_conf_t;

#define _WIZCHIP_SOCK_NUM_  8

typedef struct
{
    uint16_t sockAnyPort;
    uint16_t sockIoMode;
    uint16_t sockIsSending;
    uint16_t sockRemainedSize[_WIZCHIP_SOCK_NUM_];
} static_socket_t;

#define TCP_BUF_LEN    200                      // Maximum number of characters per TCP message

typedef struct {
    SPI_HandleTypeDef *hspi;                    // Pointer to SPI handler
    StmGpio select;                             // Chip select pin
    wiz_NetInfo netInfo;                        // Network parameters
    static_wizchip_conf_t static_wizchip_conf;  // Static variables in wizchip_conf.c
    static_socket_t static_socket;              // Static variables in socket.c
    char recvBuf[TCP_BUF_LEN];                  // Micro-controller rx buffer
    char *sendBuf;                              // Micro-controller tx buffer
    bool newADCReady;                           // Becomes true when a line is ready (10 Hz)
    bool newMessage;                            // Becomes true when new command is received
    uint32_t timeStamp;                         // Timestamp of last command
} ethernetHandler_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

int W5500Init(ethernetHandler_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, wiz_NetInfo netInfo, char *sendBuf);
int W5500TCPServer(ethernetHandler_t *heth);

#endif /* INC_ETHERNET_H_ */
