/*!
 * @file    W5500.c
 * @brief   This file contains the functions implementing the ethernet protocol for the W5500
 * @note    It is based on the drivers files provided by the W5500 manufacturer
 * @file    https://docs.wiznet.io/img/products/w5500/W5500_ds_v110e.pdf
 * @date    18/09/2024
 * @author  Timothé D
 */

//*****************************************************************************
//
//! Copyright (c)  2013, WIZnet Co., LTD.
//! All rights reserved.
//!
//! Redistribution and use in source and binary forms, with or without
//! modification, are permitted provided that the following conditions
//! are met:
//!
//!     * Redistributions of source code must retain the above copyright
//! notice, this list of conditions and the following disclaimer.
//!     * Redistributions in binary form must reproduce the above copyright
//! notice, this list of conditions and the following disclaimer in the
//! documentation and/or other materials provided with the distribution.
//!     * Neither the name of the <ORGANIZATION> nor the names of its
//! contributors may be used to endorse or promote products derived
//! from this software without specific prior written permission.
//!
//! THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//! AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//! IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//! ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
//! LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//! CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//! SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//! INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//! CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//! ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
//! THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "StmGpio.h"
#include "USBprint.h"
#include "W5500.h"
#include "stm32f4xx_hal.h"
#include "time32.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define PORT           5000   // Communication port
#define TIME_OUT_MS    50     // Time out
#define INVALID_SOCKET 0xffu  // Invalid socket

// From W5500 library file

#define _W5500_SPI_VDM_OP_ 0x00

#define _W5500_SPI_READ_  (0x00 << 2)
#define _W5500_SPI_WRITE_ (0x01 << 2)

#define WIZCHIP_CREG_BLOCK     0x00
#define WIZCHIP_SREG_BLOCK(N)  (1 + 4 * N)
#define WIZCHIP_TXBUF_BLOCK(N) (2 + 4 * N)
#define WIZCHIP_RXBUF_BLOCK(N) (3 + 4 * N)

#define WIZCHIP_OFFSET_INC(ADDR, N) (ADDR + (N << 8))

/* Common registers adresses */
#define MR      (0x0000 << 8) + (WIZCHIP_CREG_BLOCK << 3)
#define GAR     (0x0001 << 8) + (WIZCHIP_CREG_BLOCK << 3)
#define SUBR    (0x0005 << 8) + (WIZCHIP_CREG_BLOCK << 3)
#define SHAR    (0x0009 << 8) + (WIZCHIP_CREG_BLOCK << 3)
#define SIPR    (0x000F << 8) + (WIZCHIP_CREG_BLOCK << 3)
#define PHYCFGR (0x002E << 8) + (WIZCHIP_CREG_BLOCK << 3)

/* Socket registers adresses */
#define Sn_MR(N)         (0x0000 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_CR(N)         (0x0001 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_IR(N)         (0x0002 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_SR(N)         (0x0003 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_PORT(N)       (0x0004 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_DIPR(N)       (0x000C << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_DPORT(N)      (0x0010 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_RXBUF_SIZE(N) (0x001E << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_TXBUF_SIZE(N) (0x001F << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_TX_FSR(N)     (0x0020 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_RX_RSR(N)     (0x0026 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_RX_RD(N)      (0x0028 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)
#define Sn_TX_WR(N)      (0x0024 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3)

/* MODE register values */
#define MR_RST   0x80
#define MR_WOL   0x20
#define MR_PB    0x10
#define MR_PPPOE 0x08
#define MR_FARP  0x02

/* PHYCFGR register value */
#define PHYCFGR_RST         (1 << 7)
#define PHYCFGR_OPMD        (1 << 6)
#define PHYCFGR_OPMDC_ALLA  (7 << 3)
#define PHYCFGR_OPMDC_PDOWN (6 << 3)
#define PHYCFGR_OPMDC_NA    (5 << 3)
#define PHYCFGR_OPMDC_100FA (4 << 3)
#define PHYCFGR_OPMDC_100F  (3 << 3)
#define PHYCFGR_OPMDC_100H  (2 << 3)
#define PHYCFGR_OPMDC_10F   (1 << 3)
#define PHYCFGR_OPMDC_10H   (0 << 3)
#define PHYCFGR_DPX_FULL    (1 << 2)
#define PHYCFGR_DPX_HALF    (0 << 2)
#define PHYCFGR_SPD_100     (1 << 1)
#define PHYCFGR_SPD_10      (0 << 1)
#define PHYCFGR_LNK_ON      (1 << 0)
#define PHYCFGR_LNK_OFF     (0 << 0)

/* Sn_MR Default values */
#define Sn_MR_MULTI  0x80
#define Sn_MR_BCASTB 0x40
#define Sn_MR_ND     0x20
#define Sn_MR_UCASTB 0x10
#define Sn_MR_MACRAW 0x04
#define Sn_MR_IPRAW  0x03
#define Sn_MR_UDP    0x02
#define Sn_MR_TCP    0x01
#define Sn_MR_CLOSE  0x00
#define Sn_MR_MFEN   Sn_MR_MULTI
#define Sn_MR_MMB    Sn_MR_ND
#define Sn_MR_MIP6B  Sn_MR_UCASTB
#define Sn_MR_MC     Sn_MR_ND
#define SOCK_STREAM  Sn_MR_TCP
#define SOCK_DGRAM   Sn_MR_UDP

/* Sn_CR values */
#define Sn_CR_OPEN      0x01
#define Sn_CR_LISTEN    0x02
#define Sn_CR_CONNECT   0x04
#define Sn_CR_DISCON    0x08
#define Sn_CR_CLOSE     0x10
#define Sn_CR_SEND      0x20
#define Sn_CR_SEND_MAC  0x21
#define Sn_CR_SEND_KEEP 0x22
#define Sn_CR_RECV      0x40

/* Sn_IR values */
#define Sn_IR_SENDOK  0x10
#define Sn_IR_TIMEOUT 0x08
#define Sn_IR_RECV    0x04
#define Sn_IR_DISCON  0x02
#define Sn_IR_CON     0x01

/* Sn_SR values */
#define SOCK_CLOSED      0x00
#define SOCK_INIT        0x13
#define SOCK_LISTEN      0x14
#define SOCK_SYNSENT     0x15
#define SOCK_SYNRECV     0x16
#define SOCK_ESTABLISHED 0x17
#define SOCK_FIN_WAIT    0x18
#define SOCK_CLOSING     0x1A
#define SOCK_TIME_WAIT   0x1B
#define SOCK_CLOSE_WAIT  0x1C
#define SOCK_LAST_ACK    0x1D
#define SOCK_UDP         0x22
#define SOCK_IPRAW       0x32
#define SOCK_MACRAW      0x42

/* Get common registers values */
#define getMR(heth)         WIZCHIP_READ(heth, MR)
#define getGAR(heth, gar)   WIZCHIP_READ_BUF(heth, GAR, gar, 4)
#define getSUBR(heth, subr) WIZCHIP_READ_BUF(heth, SUBR, subr, 4)
#define getSHAR(heth, shar) WIZCHIP_READ_BUF(heth, SHAR, shar, 6)
#define getSIPR(heth, sipr) WIZCHIP_READ_BUF(heth, SIPR, sipr, 4)
#define getPHYCFGR(heth)    WIZCHIP_READ(heth, PHYCFGR)

/* Set common registers values */
#define setMR(heth, mr)           WIZCHIP_WRITE(heth, MR, mr)
#define setGAR(heth, gar)         WIZCHIP_WRITE_BUF(heth, GAR, gar, 4)
#define setSUBR(heth, subr)       WIZCHIP_WRITE_BUF(heth, SUBR, subr, 4)
#define setSHAR(heth, shar)       WIZCHIP_WRITE_BUF(heth, SHAR, shar, 6)
#define setSIPR(heth, sipr)       WIZCHIP_WRITE_BUF(heth, SIPR, sipr, 4)
#define setPHYCFGR(heth, phycfgr) WIZCHIP_WRITE(heth, PHYCFGR, phycfgr)

/* Get socket registers values */
#define getSn_MR(heth, sn)         WIZCHIP_READ(heth, Sn_MR(sn))
#define getSn_CR(heth, sn)         WIZCHIP_READ(heth, Sn_CR(sn))
#define getSn_SR(heth, sn)         WIZCHIP_READ(heth, Sn_SR(sn))
#define getSn_IR(heth, sn)         (WIZCHIP_READ(heth, Sn_IR(sn)) & 0x1F)
#define getSn_TXBUF_SIZE(heth, sn) WIZCHIP_READ(heth, Sn_TXBUF_SIZE(sn))
#define getSn_RXBUF_SIZE(heth, sn) WIZCHIP_READ(heth, Sn_RXBUF_SIZE(sn))
#define getSn_TX_WR(heth, sn)                            \
    (((uint16_t)WIZCHIP_READ(heth, Sn_TX_WR(sn)) << 8) + \
     WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_TX_WR(sn), 1)))
#define getSn_TxMAX(heth, sn) (((uint16_t)getSn_TXBUF_SIZE(heth, sn)) << 10)
#define getSn_RxMAX(heth, sn) (((uint16_t)getSn_RXBUF_SIZE(heth, sn)) << 10)
#define getSn_DPORT(heth, sn)                            \
    (((uint16_t)WIZCHIP_READ(heth, Sn_DPORT(sn)) << 8) + \
     WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_DPORT(sn), 1)))
#define getSn_DIPR(heth, sn, dipr) WIZCHIP_READ_BUF(heth, Sn_DIPR(sn), dipr, 4)

/* Set socket registers values */
#define setSn_MR(heth, sn, mr) WIZCHIP_WRITE(heth, Sn_MR(sn), mr)
#define setSn_CR(heth, sn, cr) WIZCHIP_WRITE(heth, Sn_CR(sn), cr)
#define setSn_IR(heth, sn, ir) WIZCHIP_WRITE(heth, Sn_IR(sn), (ir & 0x1F))
#define setSn_PORT(heth, sn, port)                                              \
    {                                                                           \
        WIZCHIP_WRITE(heth, Sn_PORT(sn), (uint8_t)(port >> 8));                 \
        WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_PORT(sn), 1), (uint8_t)port); \
    }
#define setSn_TX_WR(heth, sn, txwr)                                              \
    {                                                                            \
        WIZCHIP_WRITE(heth, Sn_TX_WR(sn), (uint8_t)(txwr >> 8));                 \
        WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_TX_WR(sn), 1), (uint8_t)txwr); \
    }
#define setSn_RX_RD(heth, sn, rxrd)                                              \
    {                                                                            \
        WIZCHIP_WRITE(heth, Sn_RX_RD(sn), (uint8_t)(rxrd >> 8));                 \
        WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_RX_RD(sn), 1), (uint8_t)rxrd); \
    }
#define getSn_RX_RD(heth, sn)                            \
    (((uint16_t)WIZCHIP_READ(heth, Sn_RX_RD(sn)) << 8) + \
     WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_RX_RD(sn), 1)))
#define setSn_TXBUF_SIZE(heth, sn, txbufsize) WIZCHIP_WRITE(heth, Sn_TXBUF_SIZE(sn), txbufsize)
#define setSn_RXBUF_SIZE(heth, sn, rxbufsize) WIZCHIP_WRITE(heth, Sn_RXBUF_SIZE(sn), rxbufsize)

// From socket library file

#define SOCK_ANY_PORT_NUM 0xC000

#define SOCK_OK    1
#define SOCK_BUSY  0
#define SOCK_FATAL -1000

#define SOCK_ERROR         0
#define SOCKERR_SOCKNUM    (SOCK_ERROR - 1)
#define SOCKERR_SOCKOPT    (SOCK_ERROR - 2)
#define SOCKERR_SOCKINIT   (SOCK_ERROR - 3)
#define SOCKERR_SOCKCLOSED (SOCK_ERROR - 4)
#define SOCKERR_SOCKMODE   (SOCK_ERROR - 5)
#define SOCKERR_SOCKFLAG   (SOCK_ERROR - 6)
#define SOCKERR_SOCKSTATUS (SOCK_ERROR - 7)
#define SOCKERR_ARG        (SOCK_ERROR - 10)
#define SOCKERR_PORTZERO   (SOCK_ERROR - 11)
#define SOCKERR_IPINVALID  (SOCK_ERROR - 12)
#define SOCKERR_TIMEOUT    (SOCK_ERROR - 13)
#define SOCKERR_DATALEN    (SOCK_ERROR - 14)
#define SOCKERR_BUFFER     (SOCK_ERROR - 15)

#define SOCKFATAL_PACKLEN (SOCK_FATAL - 1)

// SOCKET FLAG
#define SF_TCP_NODELAY (Sn_MR_ND)
#define SF_IO_NONBLOCK 0x01

// UDP & MACRAW Packet Infomation
#define PACK_COMPLETED 0x00

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void chipSelect(ethernet_t *heth);
static void chipUnselect(ethernet_t *heth);
static void readBurst(ethernet_t *heth, uint8_t *buff, uint16_t len);
static void writeBurst(ethernet_t *heth, uint8_t *buff, uint16_t len);
static uint8_t readByte(ethernet_t *heth);

// From W5500 library file
uint8_t WIZCHIP_READ(ethernet_t *heth, uint32_t AddrSel);
void WIZCHIP_WRITE(ethernet_t *heth, uint32_t AddrSel, uint8_t wb);
void WIZCHIP_READ_BUF(ethernet_t *heth, uint32_t AddrSel, uint8_t *pBuf, uint16_t len);
void WIZCHIP_WRITE_BUF(ethernet_t *heth, uint32_t AddrSel, uint8_t *pBuf, uint16_t len);
uint16_t getSn_TX_FSR(ethernet_t *heth, uint8_t sn);
uint16_t getSn_RX_RSR(ethernet_t *heth, uint8_t sn);
void wiz_send_data(ethernet_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len);
void wiz_recv_data(ethernet_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len);

// From wizchip_conf library file
static void wizchip_sw_reset(ethernet_t *heth);
static int8_t wizchip_init(ethernet_t *heth, uint8_t *txsize, uint8_t *rxsize);
void wizchip_setnetinfo(ethernet_t *heth, netInfo_t *pnetinfo);

// From socket library file
int8_t socket(ethernet_t *heth, uint8_t sn, uint16_t port);
int8_t close_socket(ethernet_t *heth, uint8_t sn);
int8_t listen(ethernet_t *heth, uint8_t sn);
int8_t disconnect(ethernet_t *heth, uint8_t sn);
int32_t send(ethernet_t *heth, uint8_t sn, uint8_t *buf, uint16_t len);
int32_t recv(ethernet_t *heth, uint8_t sn, uint8_t *buf, uint16_t len);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Callback function to select the W5500 SPI peripheral
 * @param   heth Ethernet handler
 */
static void chipSelect(ethernet_t *heth) { stmSetGpio(heth->select, false); }

/*!
 * @brief   Callback function to unselect the W5500 SPI peripheral
 * @param   heth Ethernet handler
 */
static void chipUnselect(ethernet_t *heth) { stmSetGpio(heth->select, true); }

/*!
 * @brief   Callback function to read a multi-bytes message
 * @param   heth Ethernet handler
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
 */
static void readBurst(ethernet_t *heth, uint8_t *buff, uint16_t len) {
    HAL_SPI_Receive(heth->hspi, buff, len, TIME_OUT_MS);
}

/*!
 * @brief   Callback function to write a multi-bytes message
 * @param   heth Ethernet handler
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
 */
static void writeBurst(ethernet_t *heth, uint8_t *buff, uint16_t len) {
    HAL_SPI_Transmit(heth->hspi, buff, len, TIME_OUT_MS);
}

/*!
 * @brief   Callback function to read a byte
 * @param   heth Ethernet handler
 * @return  The byte that has been read
 */
static uint8_t readByte(ethernet_t *heth) {
    static uint8_t byte;
    readBurst(heth, &byte, sizeof(byte));
    return byte;
}

/*!
 * @brief   Read 1 byte from register
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   AddrSel Register address
 * @return  Register value
 */
uint8_t WIZCHIP_READ(ethernet_t *heth, uint32_t AddrSel) {
    uint8_t spi_data[3];

    chipSelect(heth);

    AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);
    spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
    spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
    spi_data[2] = (AddrSel & 0x000000FF) >> 0;

    writeBurst(heth, spi_data, 3);
    uint8_t ret = readByte(heth);

    chipUnselect(heth);

    return ret;
}

/*!
 * @brief   Write 1 byte on register
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   AddrSel Register address
 * @param   wb Byte to write
 */
void WIZCHIP_WRITE(ethernet_t *heth, uint32_t AddrSel, uint8_t wb) {
    uint8_t spi_data[4];

    chipSelect(heth);

    AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);
    spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
    spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
    spi_data[2] = (AddrSel & 0x000000FF) >> 0;
    spi_data[3] = wb;

    writeBurst(heth, spi_data, 4);

    chipUnselect(heth);
}

/*!
 * @brief   Read bytes from register
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   AddrSel Register address
 * @param   pBuf Buffer
 * @param   len Length
 */
void WIZCHIP_READ_BUF(ethernet_t *heth, uint32_t AddrSel, uint8_t *pBuf, uint16_t len) {
    uint8_t spi_data[3];

    chipSelect(heth);

    AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);
    spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
    spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
    spi_data[2] = (AddrSel & 0x000000FF) >> 0;

    writeBurst(heth, spi_data, 3);
    readBurst(heth, pBuf, len);

    chipUnselect(heth);
}

/*!
 * @brief   Write bytes on register
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   AddrSel Register address
 * @param   pBuf Buffer
 * @param   len Length
 */
void WIZCHIP_WRITE_BUF(ethernet_t *heth, uint32_t AddrSel, uint8_t *pBuf, uint16_t len) {
    uint8_t spi_data[3];

    chipSelect(heth);

    AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);
    spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
    spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
    spi_data[2] = (AddrSel & 0x000000FF) >> 0;

    writeBurst(heth, spi_data, 3);
    writeBurst(heth, pBuf, len);

    chipUnselect(heth);
}

/*!
 * @brief   Get Sn_TX_FSR register value
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @return  Register value
 */
uint16_t getSn_TX_FSR(ethernet_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();
    uint16_t val       = 1;
    uint16_t val1      = 0;

    while (val != val1) {
        val1 = WIZCHIP_READ(heth, Sn_TX_FSR(sn));
        val1 = (val1 << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn), 1));

        if (val1 != 0) {
            val = WIZCHIP_READ(heth, Sn_TX_FSR(sn));
            val = (val << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn), 1));
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return 0;
        }
    }

    return val;
}

/*!
 * @brief   Get Sn_RX_RSR register value
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @return  Register value
 */
uint16_t getSn_RX_RSR(ethernet_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();
    uint16_t val       = 1;
    uint16_t val1      = 0;

    while (val != val1) {
        val1 = WIZCHIP_READ(heth, Sn_RX_RSR(sn));
        val1 = (val1 << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn), 1));

        if (val1 != 0) {
            val = WIZCHIP_READ(heth, Sn_RX_RSR(sn));
            val = (val << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn), 1));
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return 0;
        }
    }

    return val;
}

/*!
 * @brief   Sends data on given socket
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @param   wizdata Buffer
 * @param   len Length
 */
void wiz_send_data(ethernet_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len) {
    uint16_t ptr     = 0;
    uint32_t addrsel = 0;

    if (len == 0) {
        return;
    }
    ptr     = getSn_TX_WR(heth, sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
    WIZCHIP_WRITE_BUF(heth, addrsel, wizdata, len);
    ptr += len;
    setSn_TX_WR(heth, sn, ptr);
}

/*!
 * @brief   Receives data from given socket
 * @note    From W5500 library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @param   wizdata Buffer
 * @param   len Length
 */
void wiz_recv_data(ethernet_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len) {
    uint16_t ptr     = 0;
    uint32_t addrsel = 0;

    if (len == 0) {
        return;
    }

    ptr     = getSn_RX_RD(heth, sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
    WIZCHIP_READ_BUF(heth, addrsel, wizdata, len);
    ptr += len;
    setSn_RX_RD(heth, sn, ptr);
}

/*!
 * @brief   Software reset
 * @note    From wizchip_conf library file
 * @param   heth Ethernet handler
 */
static void wizchip_sw_reset(ethernet_t *heth) {
    uint8_t gw[4], sn[4], sip[4];
    uint8_t mac[6];

    getSHAR(heth, mac);
    getGAR(heth, gw);
    getSUBR(heth, sn);
    getSIPR(heth, sip);

    setMR(heth, MR_RST);
    getMR(heth);  // for delay

    setSHAR(heth, mac);
    setGAR(heth, gw);
    setSUBR(heth, sn);
    setSIPR(heth, sip);
}

/*!
 * @brief   Chip initialization
 * @note    From wizchip_conf library file
 * @param   heth Ethernet handler
 * @param   txsize Size of chip socket TX buffers
 * @param   rxsize Size of chip socket RX buffers
 * @return  0 if OK, else -1
 */
static int8_t wizchip_init(ethernet_t *heth, uint8_t *txsize, uint8_t *rxsize) {
    int8_t tmp = 0;
    wizchip_sw_reset(heth);
    if (txsize) {
        tmp = 0;
        for (uint8_t i = 0; i < NO_OF_SOCKETS; i++) {
            tmp += txsize[i];
            if (tmp > 16) {
                return -1;
            }
        }
        for (uint8_t i = 0; i < NO_OF_SOCKETS; i++) {
            setSn_TXBUF_SIZE(heth, i, txsize[i]);
        }
    }
    if (rxsize) {
        tmp = 0;
        for (uint8_t i = 0; i < NO_OF_SOCKETS; i++) {
            tmp += rxsize[i];
            if (tmp > 16) {
                return -1;
            }
        }
        for (uint8_t i = 0; i < NO_OF_SOCKETS; i++) {
            setSn_RXBUF_SIZE(heth, i, rxsize[i]);
        }
    }
    return 0;
}

/*!
 * @brief   Sets network parameters
 * @note    From wizchip_conf library file
 * @param   heth Ethernet handler
 * @param   pnetInfo Network parameters
 */
void wizchip_setnetinfo(ethernet_t *heth, netInfo_t *pnetinfo) {
    setSHAR(heth, pnetinfo->mac);
    setGAR(heth, pnetinfo->gw);
    setSUBR(heth, pnetinfo->sn);
    setSIPR(heth, pnetinfo->ip);
}

/*!
 * @brief   Opens a TCP socket
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @param   port Port number
 * @return  Socket number if OK, else < 0
 */
int8_t socket(ethernet_t *heth, uint8_t sn, uint16_t port) {
    uint32_t timeStamp = HAL_GetTick();
    uint32_t taddr;

    getSIPR(heth, (uint8_t *)&taddr);

    if (taddr == 0) {
        return SOCKERR_SOCKINIT;
    }

    close_socket(heth, sn);

    setSn_MR(heth, sn, Sn_MR_TCP);

    if (!port) {
        port = heth->sock_any_port++;
        if (heth->sock_any_port == 0xFFF0) {
            heth->sock_any_port = SOCK_ANY_PORT_NUM;
        }
    }

    setSn_PORT(heth, sn, port);
    setSn_CR(heth, sn, Sn_CR_OPEN);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_io_mode &= ~(1 << sn);
    heth->sock_is_sending &= ~(1 << sn);

    while (getSn_SR(heth, sn) == SOCK_CLOSED) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    return (int8_t)sn;
}

/*!
 * @brief   Closes a TCP socket
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @return  1 if OK, else < 0
 */
int8_t close_socket(ethernet_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_CLOSE);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    setSn_IR(heth, sn, 0xFF);
    heth->sock_io_mode &= ~(1 << sn);
    heth->sock_is_sending &= ~(1 << sn);

    while (getSn_SR(heth, sn) != SOCK_CLOSED) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    return SOCK_OK;
}

/*!
 * @brief   Listen for client connection
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @return  1 if OK, else < 0
 */
int8_t listen(ethernet_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_LISTEN);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    if (getSn_SR(heth, sn) != SOCK_LISTEN) {
        close_socket(heth, sn);
        return SOCKERR_SOCKCLOSED;
    }

    return SOCK_OK;
}

/*!
 * @brief   Disconnect from the client
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @return  1 if OK, else < 0
 */
int8_t disconnect(ethernet_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_DISCON);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_is_sending &= ~(1 << sn);

    if (heth->sock_io_mode & (1 << sn)) {
        return SOCK_BUSY;
    }

    while (getSn_SR(heth, sn) != SOCK_CLOSED) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    return SOCK_OK;
}

/*!
 * @brief   Send data to client
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @param   buf Buffer
 * @param   len Length
 * @return  Length if OK, else < 0
 */
int32_t send(ethernet_t *heth, uint8_t sn, uint8_t *buf, uint16_t len) {
    uint32_t timeStamp = HAL_GetTick();
    uint8_t tmp        = 0;
    uint16_t freesize  = 0;

    tmp = getSn_SR(heth, sn);

    if (tmp != SOCK_ESTABLISHED && tmp != SOCK_CLOSE_WAIT) {
        return SOCKERR_SOCKSTATUS;
    }

    if (heth->sock_is_sending & (1 << sn)) {
        tmp = getSn_IR(heth, sn);
        if (tmp & Sn_IR_SENDOK) {
            setSn_IR(heth, sn, Sn_IR_SENDOK);
            heth->sock_is_sending &= ~(1 << sn);
        }
        else if (tmp & Sn_IR_TIMEOUT) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
        else {
            return SOCK_BUSY;
        }
    }

    freesize = getSn_TxMAX(heth, sn);

    if (len > freesize) {
        len = freesize;  // check size not to exceed MAX size.
    }

    while (1) {
        freesize = getSn_TX_FSR(heth, sn);
        tmp      = getSn_SR(heth, sn);

        if ((tmp != SOCK_ESTABLISHED) && (tmp != SOCK_CLOSE_WAIT)) {
            close_socket(heth, sn);
            return SOCKERR_SOCKSTATUS;
        }

        if ((heth->sock_io_mode & (1 << sn)) && (len > freesize)) {
            return SOCK_BUSY;
        }

        if (len <= freesize) {
            break;
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    wiz_send_data(heth, sn, buf, len);

    setSn_CR(heth, sn, Sn_CR_SEND);

    /* wait to process the command... */
    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_is_sending |= (1 << sn);

    return (int32_t)len;
}

/*!
 * @brief   Receives from client
 * @note    From socket library file
 * @param   heth Ethernet handler
 * @param   sn Socket number
 * @param   buf Buffer
 * @param   len Length
 * @return  Length if OK, else < 0
 */
int32_t recv(ethernet_t *heth, uint8_t sn, uint8_t *buf, uint16_t len) {
    uint32_t timeStamp = HAL_GetTick();
    uint8_t tmp        = 0;
    uint16_t recvsize  = 0;

    recvsize = getSn_RxMAX(heth, sn);
    if (recvsize < len) {
        len = recvsize;
    }

    while (1) {
        recvsize = getSn_RX_RSR(heth, sn);
        tmp      = getSn_SR(heth, sn);
        if (tmp != SOCK_ESTABLISHED) {
            if (tmp == SOCK_CLOSE_WAIT) {
                if (recvsize != 0) {
                    break;
                }
                else if (getSn_TX_FSR(heth, sn) == getSn_TxMAX(heth, sn)) {
                    close_socket(heth, sn);
                    return SOCKERR_SOCKSTATUS;
                }
            }
            else {
                close_socket(heth, sn);
                return SOCKERR_SOCKSTATUS;
            }
        }

        if ((heth->sock_io_mode & (1 << sn)) && (recvsize == 0)) {
            return SOCK_BUSY;
        }

        if (recvsize != 0) {
            break;
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    if (recvsize < len) {
        len = recvsize;
    }

    wiz_recv_data(heth, sn, buf, len);
    setSn_CR(heth, sn, Sn_CR_RECV);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    return (int32_t)len;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Checks if ethernet link is active
 * @param   heth Ethernet handler
 * @return  True if active
 */
bool isLinkOn(ethernet_t *heth) { return (getPHYCFGR(heth) & PHYCFGR_LNK_ON); }

/*!
 * @brief   Checks if PHY is enabled
 * @param   heth Ethernet handler
 * @return  True if enabled
 */
bool isPhyEnabled(ethernet_t *heth) {
    return ((getPHYCFGR(heth) & PHYCFGR_OPMDC_ALLA) == PHYCFGR_OPMDC_ALLA);
}

/*!
 * @brief   Activate / deactivate PHY
 * @param   heth Ethernet handler
 * @param   activate To choose
 * @return  0 if OK, else -1
 */
int setPhyState(ethernet_t *heth, bool activate) {
    // To get the current PHY confiduration register
    uint8_t tmp = getPHYCFGR(heth);

    // To to be able to change the power mode by software
    tmp |= PHYCFGR_OPMD;

    // Clear operation mode bits (OPMDC)
    tmp &= ~PHYCFGR_OPMDC_ALLA;

    // Set to All Capable mode
    if (activate) {
        tmp |= PHYCFGR_OPMDC_ALLA;
    }
    // Set to Power-Down mode
    else {
        tmp |= PHYCFGR_OPMDC_PDOWN;
    }

    // Send PHY parameters
    setPHYCFGR(heth, tmp);

    // Toggle reset bit (reset when bit is low)
    tmp &= ~PHYCFGR_RST;
    setPHYCFGR(heth, tmp);
    tmp |= PHYCFGR_RST;
    setPHYCFGR(heth, tmp);

    // Check if the new register value is as expected
    if (isPhyEnabled(heth) == activate) {
        return 0;
    }
    return -1;
}

/*!
 * @brief   Set ethernet port to MACRAW mode
 * @note    Used to make it invisible to a switch (no ARP answer)
 * @param   heth Ethernet handler
 */
void setMACRawMode(ethernet_t *heth) {
    uint32_t timeStamp = HAL_GetTick();

    // Socket 0 is changed to MACRAW mode
    setSn_CR(heth, 0, Sn_CR_CLOSE);
    uint8_t tmp = getSn_MR(heth, 0);
    tmp         = (tmp & ~0x0f) | Sn_MR_MACRAW;
    setSn_MR(heth, 0, tmp);
    setSn_CR(heth, 0, Sn_CR_OPEN);

    while (getSn_SR(heth, 0) != SOCK_MACRAW) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            USBnprintf("\r\nSTUCK\r\n");
            break;
        }
    }

    // All other sockets are closed
    for (uint16_t socketId = 1; socketId < 8; socketId++) {
        tmp = getSn_MR(heth, socketId);
        tmp = (tmp & ~0x0f) | Sn_MR_CLOSE;
        setSn_MR(heth, socketId, tmp);
    }
}

/*!
 * @brief   Set ethernet port to TCP mode
 * @note    When going from MACRAW to TCP, use sendGratuitousARP() before
 * @param   heth Ethernet handler
 */
void setTCPMode(ethernet_t *heth) {
    for (uint16_t socketId = 0; socketId < NO_OF_SOCKETS; socketId++) {
        uint8_t tmp = getSn_MR(heth, socketId);
        tmp         = (tmp & ~0x0f) | Sn_MR_TCP;
        setSn_MR(heth, socketId, tmp);
    }
    // No socket open yet
    heth->activeSocket = INVALID_SOCKET;
}

/*!
 * @brief   Sends ARP message to all network nodes to update ARP caches
 * @note    Used when going from MACRAW to TCP
 * @param   heth Ethernet handler
 */
void sendGratuitousARP(ethernet_t *heth) {
    static const uint8_t SOCKET_NUMBER = 0;
    static const uint8_t PACKET_SIZE   = 42;
    uint8_t packet[PACKET_SIZE];  // Total packet size (Ethernet Header + ARP Header)

    // Ethernet Header (14 bytes)
    memset(packet, 0xFF, 6);                   // Destination MAC: Broadcast (FF:FF:FF:FF:FF:FF)
    memcpy(&packet[6], heth->netInfo.mac, 6);  // Source MAC: W5500's MAC address
    packet[12] = 0x08;                         // Ethertype (0x0806 for ARP)
    packet[13] = 0x06;

    // ARP Header (28 bytes)
    packet[14] = 0x00;
    packet[15] = 0x01;  // Hardware Type: Ethernet (1)
    packet[16] = 0x08;
    packet[17] = 0x00;  // Protocol Type: IPv4 (0x0800)
    packet[18] = 0x06;  // Hardware size: 6 (MAC address length)
    packet[19] = 0x04;  // Protocol size: 4 (IPv4 address length)
    packet[20] = 0x00;
    packet[21] = 0x02;  // Opcode: ARP Reply (2)

    memcpy(&packet[22], heth->netInfo.mac, 6);  // Sender MAC Address
    memcpy(&packet[28], heth->netInfo.ip, 4);   // Sender IP Address
    memset(&packet[32], 0x00, 6);               // Target MAC Address (ignored for gratuitous ARP)
    memcpy(&packet[38], heth->netInfo.ip, 4);   // Target IP Address (same as sender IP)

    wiz_send_data(heth, SOCKET_NUMBER, packet, PACKET_SIZE);  // Send packet
    setSn_CR(heth, SOCKET_NUMBER, Sn_CR_SEND);                // Trigger send
    HAL_Delay(1);  // Small delay to make sure the packet is sent
}

/*!
 * @brief   Callback function to write a byte
 * @note    Should be called one time for each physical ethernet port
 * @param   heth Pointer to the ethernet handler
 * @param   hspi Pointer to associated SPI handler
 * @param   port Pointer to chip select GPIO block
 * @param   pin Chip select GPIO pin
 * @param   netInfo Network description
 * @param   txBuf Pointer to micro-controller TX buffer
 * @param   rxBuf Pointer to micro-controller RX buffer
 * @return  0 on success, else < 0
 */
int W5500Init(ethernet_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin,
              netInfo_t netInfo, char *txBuf, char *rxBuf) {
    heth->hspi    = hspi;
    heth->netInfo = netInfo;
    stmGpioInit(&heth->select, port, pin, STM_GPIO_OUTPUT);
    heth->lastRxTime = HAL_GetTick();
    heth->rxBuf      = rxBuf;
    heth->rxReady    = false;
    heth->txBuf      = txBuf;
    heth->txReady    = false;
    heth->lastRxTime = 0;

    heth->sock_any_port   = SOCK_ANY_PORT_NUM;
    heth->sock_io_mode    = 0;
    heth->sock_is_sending = 0;

    // Define the W5500 buffer sizes for each socket in kB (32 kB available in total)
    static uint8_t txBufSize[NO_OF_SOCKETS] = {8, 8};
    static uint8_t rxBufSize[NO_OF_SOCKETS] = {8, 8};

    if (wizchip_init(heth, txBufSize, rxBufSize) != 0) {
        return -1;
    }

    // Necessary to let the chip initialize before sending network information
    HAL_Delay(100);

    // Set MACRAW mode to not be visible on network
    setMACRawMode(heth);

    // Network parameters
    wizchip_setnetinfo(heth, &heth->netInfo);

    return 0;
}

/*!
 * @brief   Implementation of the TCP server
 * @param   heth Physical ethernet port used
 * @return  >= 0 on success, else < 0
 * @note    Should be used in the while(1) loop of the board for each physical ethernet port
 */
int W5500TCPServer(ethernet_t *heth) {
    for (uint8_t socketId = 0; socketId < NO_OF_SOCKETS; socketId++) {
        heth->sockets[socketId].status = getSn_SR(heth, socketId);

        switch (heth->sockets[socketId].status) {
            case SOCK_ESTABLISHED:
                // If no active socket is set, this is the first connection
                if (heth->activeSocket == INVALID_SOCKET) {
                    heth->activeSocket = socketId;
                }
                // If this is the active socket, handle communication
                if (socketId == heth->activeSocket) {
                    if (heth->txReady) {
                        send(heth, socketId, (uint8_t *)heth->txBuf, strlen(heth->txBuf));
                        heth->txReady = false;
                    }

                    // If the RX buffer contains data, receive it
                    if (getSn_RX_RSR(heth, socketId) > 0) {
                        recv(heth, socketId, (uint8_t *)heth->rxBuf, TCP_BUF_LEN);
                        heth->rxReady    = true;
                        heth->lastRxTime = HAL_GetTick();
                    }
                }
                else {
                    // New client detected, disconnect old one
                    disconnect(heth, heth->activeSocket);
                    heth->activeSocket = socketId;
                }
                break;

            case SOCK_CLOSE_WAIT:
                // Disconnect the client and reset the active socket
                disconnect(heth, socketId);
                heth->activeSocket = INVALID_SOCKET;
                break;

            case SOCK_CLOSED:
                // Open a new TCP socket to listen for new clients
                socket(heth, socketId, PORT);
                break;

            case SOCK_INIT:
                // Start listening for new connections
                listen(heth, socketId);
                break;

            default:
                // Waiting for a client connection
                break;
        }
    }

    return heth->activeSocket;
}
