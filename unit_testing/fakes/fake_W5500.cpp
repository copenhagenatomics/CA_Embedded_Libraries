/*!
 * @file    fake_ethernet.cpp
 * @brief   Fake Interface to W5500 for unit testing
 * @date    06/02/2025
 * @author  Timothé D
*/

#include "fake_stm32xxxx_hal.h"
#include "W5500.h"

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

bool isLinkOn(ethernet_t *heth) {
    return true;
}

bool isPhyEnabled(ethernet_t *heth) {
    return true;
}

int setPhyState(ethernet_t *heth, bool activate) {
    return 0;
}

void setMACRawMode(ethernet_t *heth) {
    return;
}

void setTCPMode(ethernet_t *heth) {
    return;
}

void sendGratuitousARP(ethernet_t *heth) {
    return;
}

int W5500Init(ethernet_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, netInfo_t netInfo, char *txBuf, char *rxBuf) {
    return 0;
}

int W5500TCPServer(ethernet_t *heth) {
    return 0;
}
