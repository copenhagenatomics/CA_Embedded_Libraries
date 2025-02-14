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

int W5500Init(ethernetHandler_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, wiz_NetInfo netInfo, char *sendBuf) {
    return 0;
}

int W5500TCPServer(ethernetHandler_t *heth) {
    return 0;
}
