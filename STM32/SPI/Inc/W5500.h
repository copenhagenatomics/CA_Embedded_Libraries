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

#include "StmGpio.h"
#include "stm32f4xx_hal.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct {
    uint8_t mac[6];  // Source Mac Address
    uint8_t ip[4];   // Source IP Address
    uint8_t sn[4];   // Subnet Mask
    uint8_t gw[4];   // Gateway IP Address
} netInfo_t;

#define TCP_BUF_LEN   200  // Maximum number of characters per TCP message
#define NO_OF_SOCKETS 2    // 2 sockets implemented

typedef struct {
    uint8_t remoteIP[4];  // IP address of the client
    uint16_t remotePort;  // Port of the client
    uint8_t status;       // Status (Sn_SR register)
} socket_t;

typedef struct {
    SPI_HandleTypeDef *hspi;          // Pointer to SPI handler
    StmGpio select;                   // Chip select pin
    netInfo_t netInfo;                // Network parameters
    char *rxBuf;                      // Micro-controller rx buffer
    bool rxReady;                     // Becomes true when new command is received
    uint32_t lastRxTime;              // Timestamp of last command
    char *txBuf;                      // Micro-controller tx buffer
    bool txReady;                     // Becomes true when a line is ready (10 Hz)
    bool stopADCPrint;                // To stop 10 Hz ADC print when Status/StatusDef/Serial
    socket_t sockets[NO_OF_SOCKETS];  // Sockets of the ethernet port
    uint8_t activeSocket;             // Active socket
    uint16_t sock_any_port;
    uint16_t sock_io_mode;
    uint16_t sock_is_sending;
} ethernet_t;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

bool isLinkOn(ethernet_t *heth);
bool isPhyEnabled(ethernet_t *heth);
int setPhyState(ethernet_t *heth, bool activate);
void setMACRawMode(ethernet_t *heth);
void setTCPMode(ethernet_t *heth);
void sendGratuitousARP(ethernet_t *heth);
int W5500Init(ethernet_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin,
              netInfo_t netInfo, char *txBuf, char *rxBuf);
int W5500TCPServer(ethernet_t *heth);

#endif /* INC_W5500_H_ */
