/*!
 * @file    W5500.c
 * @brief   This file contains the functions implementing the ethernet protocol for the W5500
 * @note    It is based on the drivers files provided by the W5500 manufacturer
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
#include "W5500.h"
#include "stm32f4xx_hal.h"
#include "time32.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

typedef struct {
    uint8_t remoteIP[4];  // IP address of the client
    uint16_t remotePort;  // Port of the client
    uint8_t status;       // Status (Sn_SR register)
    bool error;           // Error detected
} socket_t;

#define NO_OF_SOCKETS 2     // 8 sockets supported
#define PORT          5000  // Communication port

#define TIME_OUT_MS 50

// W5500

#define _W5500_SPI_VDM_OP_           0x00
#define _W5500_IO_BASE_              0x00000000

#define _W5500_SPI_READ_			   (0x00 << 2) //< SPI interface Read operation in Control Phase
#define _W5500_SPI_WRITE_			   (0x01 << 2) //< SPI interface Write operation in Control Phase

#define WIZCHIP_CREG_BLOCK          0x00 	//< Common register block
#define WIZCHIP_SREG_BLOCK(N)       (1+4*N) //< Socket N register block
#define WIZCHIP_TXBUF_BLOCK(N)      (2+4*N) //< Socket N Tx buffer address block
#define WIZCHIP_RXBUF_BLOCK(N)      (3+4*N) //< Socket N Rx buffer address block

#define WIZCHIP_OFFSET_INC(ADDR, N)    (ADDR + (N<<8)) //< Increase offset address

#define MR                 (_W5500_IO_BASE_ + (0x0000 << 8) + (WIZCHIP_CREG_BLOCK << 3))
#define GAR                (_W5500_IO_BASE_ + (0x0001 << 8) + (WIZCHIP_CREG_BLOCK << 3))
#define SUBR               (_W5500_IO_BASE_ + (0x0005 << 8) + (WIZCHIP_CREG_BLOCK << 3))
#define SHAR               (_W5500_IO_BASE_ + (0x0009 << 8) + (WIZCHIP_CREG_BLOCK << 3))
#define SIPR               (_W5500_IO_BASE_ + (0x000F << 8) + (WIZCHIP_CREG_BLOCK << 3))

#define Sn_MR(N)           (_W5500_IO_BASE_ + (0x0000 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_CR(N)           (_W5500_IO_BASE_ + (0x0001 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_IR(N)           (_W5500_IO_BASE_ + (0x0002 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_SR(N)           (_W5500_IO_BASE_ + (0x0003 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_PORT(N)         (_W5500_IO_BASE_ + (0x0004 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_DIPR(N)         (_W5500_IO_BASE_ + (0x000C << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_DPORT(N)        (_W5500_IO_BASE_ + (0x0010 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_RXBUF_SIZE(N)   (_W5500_IO_BASE_ + (0x001E << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_TXBUF_SIZE(N)   (_W5500_IO_BASE_ + (0x001F << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_TX_FSR(N)       (_W5500_IO_BASE_ + (0x0020 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_RX_RSR(N)       (_W5500_IO_BASE_ + (0x0026 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_RX_RD(N)        (_W5500_IO_BASE_ + (0x0028 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))
#define Sn_TX_WR(N)        (_W5500_IO_BASE_ + (0x0024 << 8) + (WIZCHIP_SREG_BLOCK(N) << 3))

#define setSn_TXBUF_SIZE(heth, sn, txbufsize) WIZCHIP_WRITE(heth, Sn_TXBUF_SIZE(sn), txbufsize)
#define setSn_RXBUF_SIZE(heth, sn, rxbufsize) WIZCHIP_WRITE(heth, Sn_RXBUF_SIZE(sn), rxbufsize)


/* MODE register values */
#define MR_RST                       0x80
#define MR_WOL                       0x20
#define MR_PB                        0x10
#define MR_PPPOE                     0x08
#define MR_FARP                      0x02

/* Sn_MR Default values */
#define Sn_MR_MULTI                  0x80
#define Sn_MR_BCASTB                 0x40
#define Sn_MR_ND                     0x20
#define Sn_MR_UCASTB                 0x10
#define Sn_MR_MACRAW                 0x04
#define Sn_MR_IPRAW                  0x03
#define Sn_MR_UDP                    0x02
#define Sn_MR_TCP                    0x01
#define Sn_MR_CLOSE                  0x00
#define Sn_MR_MFEN                   Sn_MR_MULTI
#define Sn_MR_MMB                    Sn_MR_ND
#define Sn_MR_MIP6B                  Sn_MR_UCASTB
#define Sn_MR_MC                     Sn_MR_ND
#define SOCK_STREAM                  Sn_MR_TCP
#define SOCK_DGRAM                   Sn_MR_UDP

/* Sn_CR values */
#define Sn_CR_OPEN                   0x01
#define Sn_CR_LISTEN                 0x02
#define Sn_CR_CONNECT                0x04
#define Sn_CR_DISCON                 0x08
#define Sn_CR_CLOSE                  0x10
#define Sn_CR_SEND                   0x20
#define Sn_CR_SEND_MAC               0x21
#define Sn_CR_SEND_KEEP              0x22
#define Sn_CR_RECV                   0x40

/* Sn_IR values */
#define Sn_IR_SENDOK                 0x10
#define Sn_IR_TIMEOUT                0x08
#define Sn_IR_RECV                   0x04
#define Sn_IR_DISCON                 0x02
#define Sn_IR_CON                    0x01

/* Sn_SR values */
#define SOCK_CLOSED                  0x00
#define SOCK_INIT                    0x13
#define SOCK_LISTEN                  0x14
#define SOCK_SYNSENT                 0x15
#define SOCK_SYNRECV                 0x16
#define SOCK_ESTABLISHED             0x17
#define SOCK_FIN_WAIT                0x18
#define SOCK_CLOSING                 0x1A
#define SOCK_TIME_WAIT               0x1B
#define SOCK_CLOSE_WAIT              0x1C
#define SOCK_LAST_ACK                0x1D
#define SOCK_UDP                     0x22
#define SOCK_IPRAW                   0x32     /**< IP raw mode socket */
#define SOCK_MACRAW                  0x42

#define setMR(heth, mr)               WIZCHIP_WRITE(heth, MR, mr)
#define getMR(heth)                 WIZCHIP_READ(heth, MR)
#define setGAR(heth, gar)             WIZCHIP_WRITE_BUF(heth, GAR, gar, 4)
#define getGAR(heth, gar)             WIZCHIP_READ_BUF(heth, GAR, gar, 4)
#define setSUBR(heth, subr)           WIZCHIP_WRITE_BUF(heth, SUBR, subr, 4)
#define getSUBR(heth, subr)           WIZCHIP_READ_BUF(heth, SUBR, subr, 4)
#define setSHAR(heth, shar)           WIZCHIP_WRITE_BUF(heth, SHAR, shar, 6)
#define getSHAR(heth, shar)           WIZCHIP_READ_BUF(heth, SHAR, shar, 6)
#define setSIPR(heth, sipr)           WIZCHIP_WRITE_BUF(heth, SIPR, sipr, 4)
#define getSIPR(heth, sipr)           WIZCHIP_READ_BUF(heth, SIPR, sipr, 4)

#define setSn_MR(heth, sn, mr)        WIZCHIP_WRITE(heth, Sn_MR(sn), mr)
#define getSn_MR(heth, sn)            WIZCHIP_READ(heth, Sn_MR(sn))
#define setSn_CR(heth, sn, cr)        WIZCHIP_WRITE(heth, Sn_CR(sn), cr)
#define setSn_IR(heth, sn, ir)        WIZCHIP_WRITE(heth, Sn_IR(sn), (ir & 0x1F))
#define getSn_CR(heth, sn)            WIZCHIP_READ(heth, Sn_CR(sn))
#define getSn_SR(heth, sn)            WIZCHIP_READ(heth, Sn_SR(sn))
#define getSn_IR(heth, sn)            (WIZCHIP_READ(heth, Sn_IR(sn)) & 0x1F)
#define getSn_TXBUF_SIZE(heth, sn)    WIZCHIP_READ(heth, Sn_TXBUF_SIZE(sn))
#define getSn_RXBUF_SIZE(heth, sn)    WIZCHIP_READ(heth, Sn_RXBUF_SIZE(sn))
#define getSn_TX_WR(heth, sn)         (((uint16_t)WIZCHIP_READ(heth, Sn_TX_WR(sn)) << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_TX_WR(sn), 1)))	
#define getSn_TxMAX(heth, sn)         (((uint16_t)getSn_TXBUF_SIZE(heth, sn)) << 10)
#define getSn_RxMAX(heth, sn)         (((uint16_t)getSn_RXBUF_SIZE(heth, sn)) << 10)
#define getSn_DPORT(heth, sn)         (((uint16_t)WIZCHIP_READ(heth, Sn_DPORT(sn)) << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_DPORT(sn), 1)))
#define getSn_DIPR(heth, sn, dipr)    WIZCHIP_READ_BUF(heth, Sn_DIPR(sn), dipr, 4)
#define setSn_PORT(heth, sn, port)  { WIZCHIP_WRITE(heth, Sn_PORT(sn), (uint8_t)(port >> 8)); \
                                WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_PORT(sn),1), (uint8_t) port); }
#define setSn_TX_WR(heth, sn, txwr) { WIZCHIP_WRITE(heth, Sn_TX_WR(sn), (uint8_t)(txwr>>8)); \
                                WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_TX_WR(sn),1), (uint8_t) txwr); }
#define setSn_RX_RD(heth, sn, rxrd) { WIZCHIP_WRITE(heth, Sn_RX_RD(sn), (uint8_t)(rxrd>>8)); \
                                WIZCHIP_WRITE(heth, WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1), (uint8_t) rxrd); }
#define getSn_RX_RD(heth, sn)         (((uint16_t)WIZCHIP_READ(heth, Sn_RX_RD(sn)) << 8) + WIZCHIP_READ(heth, WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1)))

// wizchip_conf

typedef   uint8_t   iodata_t;

// socket

typedef enum
{
    SO_FLAG,
    SO_TTL,
    SO_TOS,
    SO_MSS,
    SO_DESTIP,
    SO_DESTPORT,
    SO_KEEPALIVESEND,
    SO_KEEPALIVEAUTO,
    SO_SENDBUF,
    SO_RECVBUF,
    SO_STATUS,
    SO_REMAINSIZE,
    SO_PACKINFO
}sockopt_type;

#define SOCK_ANY_PORT_NUM  0xC000

#define SOCK_OK               1        ///< Result is OK about socket process.
#define SOCK_BUSY             0        ///< Socket is busy on processing the operation. Valid only Non-block IO Mode.
#define SOCK_FATAL            -1000    ///< Result is fatal error about socket process.

#define SOCK_ERROR            0        
#define SOCKERR_SOCKNUM       (SOCK_ERROR - 1)     ///< Invalid socket number
#define SOCKERR_SOCKOPT       (SOCK_ERROR - 2)     ///< Invalid socket option
#define SOCKERR_SOCKINIT      (SOCK_ERROR - 3)     ///< Socket is not initialized or SIPR is Zero IP address when Sn_MR_TCP
#define SOCKERR_SOCKCLOSED    (SOCK_ERROR - 4)     ///< Socket unexpectedly closed.
#define SOCKERR_SOCKMODE      (SOCK_ERROR - 5)     ///< Invalid socket mode for socket operation.
#define SOCKERR_SOCKFLAG      (SOCK_ERROR - 6)     ///< Invalid socket flag
#define SOCKERR_SOCKSTATUS    (SOCK_ERROR - 7)     ///< Invalid socket status for socket operation.
#define SOCKERR_ARG           (SOCK_ERROR - 10)    ///< Invalid argument.
#define SOCKERR_PORTZERO      (SOCK_ERROR - 11)    ///< Port number is zero
#define SOCKERR_IPINVALID     (SOCK_ERROR - 12)    ///< Invalid IP address
#define SOCKERR_TIMEOUT       (SOCK_ERROR - 13)    ///< Timeout occurred
#define SOCKERR_DATALEN       (SOCK_ERROR - 14)    ///< Data length is zero or greater than buffer max size.
#define SOCKERR_BUFFER        (SOCK_ERROR - 15)    ///< Socket buffer is not enough for data communication.

#define SOCKFATAL_PACKLEN     (SOCK_FATAL - 1)     ///< Invalid packet length. Fatal Error.

/*
 * SOCKET FLAG
 */
#define SF_TCP_NODELAY         (Sn_MR_ND)          ///< In @ref Sn_MR_TCP, Use to nodelayed ack.

#define SF_IO_NONBLOCK           0x01              ///< Socket nonblock io mode. It used parameter in \ref socket().

/*
 * UDP & MACRAW Packet Infomation
 */
#define PACK_COMPLETED           0x00              ///< In Non-TCP packet, It indicates to complete to receive a packet. (When W5300, This flag can be applied)

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void chipSelect(ethernetHandler_t *heth);
static void chipUnselect(ethernetHandler_t *heth);
static void readBurst(ethernetHandler_t *heth, uint8_t *buff, uint16_t len);
static void writeBurst(ethernetHandler_t *heth, uint8_t *buff, uint16_t len);
static uint8_t readByte(ethernetHandler_t *heth);

// W5500
uint8_t WIZCHIP_READ(ethernetHandler_t *heth, uint32_t AddrSel);
void WIZCHIP_WRITE(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t wb);
void WIZCHIP_READ_BUF(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t* pBuf, uint16_t len);
void WIZCHIP_WRITE_BUF(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t* pBuf, uint16_t len);
uint16_t getSn_TX_FSR(ethernetHandler_t *heth, uint8_t sn);
uint16_t getSn_RX_RSR(ethernetHandler_t *heth, uint8_t sn);
void wiz_send_data(ethernetHandler_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len);
void wiz_recv_data(ethernetHandler_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len);

// wizchip_conf
static void wizchip_sw_reset(ethernetHandler_t *heth);
static int8_t wizchip_init(ethernetHandler_t *heth, uint8_t* txsize, uint8_t* rxsize);
void wizchip_setnetinfo(ethernetHandler_t *heth, netInfo_t* pnetinfo);

// socket
int8_t socket(ethernetHandler_t *heth, uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag);
int8_t close_socket(ethernetHandler_t *heth, uint8_t sn);
int8_t listen(ethernetHandler_t *heth, uint8_t sn);
int8_t disconnect(ethernetHandler_t *heth, uint8_t sn);
int32_t send(ethernetHandler_t *heth, uint8_t sn, uint8_t *buf, uint16_t len);
int32_t recv(ethernetHandler_t *heth, uint8_t sn, uint8_t *buf, uint16_t len);
int8_t getsockopt(ethernetHandler_t *heth, uint8_t sn, sockopt_type sotype, void* arg);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static socket_t sockets[NO_OF_SOCKETS] = {0};
static uint16_t noOfOpenSockets = 0;

uint8_t  sock_pack_info[_WIZCHIP_SOCK_NUM_] = {0,};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Callback function to select the W5500 SPI peripheral
*/
static void chipSelect(ethernetHandler_t *heth)
{
    stmSetGpio(heth->select, false);
}

/*!
 * @brief   Callback function to unselect the W5500 SPI peripheral
*/
static void chipUnselect(ethernetHandler_t *heth)
{
    stmSetGpio(heth->select, true);
}

/*!
 * @brief   Callback function to read a multi-bytes message
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
*/
static void readBurst(ethernetHandler_t *heth, uint8_t* buff, uint16_t len)
{
    HAL_SPI_Receive(heth->hspi, buff, len, HAL_MAX_DELAY);
}

/*!
 * @brief   Callback function to write a multi-bytes message
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
*/
static void writeBurst(ethernetHandler_t *heth, uint8_t* buff, uint16_t len)
{
    HAL_SPI_Transmit(heth->hspi, buff, len, HAL_MAX_DELAY);
}

/*!
 * @brief   Callback function to read a byte
 * @return  The byte that has been read
*/
static uint8_t readByte(ethernetHandler_t *heth)
{
    static uint8_t byte;
    readBurst(heth, &byte, sizeof(byte));
    return byte;
}

/**************************************************************
*               From W5500
***************************************************************/

uint8_t WIZCHIP_READ(ethernetHandler_t *heth, uint32_t AddrSel) {
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

void WIZCHIP_WRITE(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t wb) {
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
            
void WIZCHIP_READ_BUF(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t* pBuf, uint16_t len) {
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

void WIZCHIP_WRITE_BUF(ethernetHandler_t *heth, uint32_t AddrSel, uint8_t* pBuf, uint16_t len) {
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

uint16_t getSn_TX_FSR(ethernetHandler_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();
    uint16_t val = 1;
    uint16_t val1 = 0;

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

uint16_t getSn_RX_RSR(ethernetHandler_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();
    uint16_t val = 1;
    uint16_t val1 = 0;

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

void wiz_send_data(ethernetHandler_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len) {
    uint16_t ptr = 0;
    uint32_t addrsel = 0;

    if(len == 0) {
        return;
    }
    ptr = getSn_TX_WR(heth, sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
    WIZCHIP_WRITE_BUF(heth, addrsel, wizdata, len);
    ptr += len;
    setSn_TX_WR(heth, sn,ptr);
}

void wiz_recv_data(ethernetHandler_t *heth, uint8_t sn, uint8_t *wizdata, uint16_t len) {
    uint16_t ptr = 0;
    uint32_t addrsel = 0;

    if(len == 0) {
        return;
    }

    ptr = getSn_RX_RD(heth, sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
    WIZCHIP_READ_BUF(heth, addrsel, wizdata, len);
    ptr += len;
    setSn_RX_RD(heth, sn, ptr);
}

/**************************************************************
*               From wizchip_conf
***************************************************************/

static void wizchip_sw_reset(ethernetHandler_t *heth) {
    uint8_t gw[4], sn[4], sip[4];
    uint8_t mac[6];

    getSHAR(heth, mac);
    getGAR(heth, gw);
    getSUBR(heth, sn);
    getSIPR(heth, sip);

    setMR(heth, MR_RST);
    getMR(heth); // for delay

    setSHAR(heth, mac);
    setGAR(heth, gw);
    setSUBR(heth, sn);
    setSIPR(heth, sip);
}

static int8_t wizchip_init(ethernetHandler_t *heth, uint8_t *txsize, uint8_t *rxsize) {
    int8_t i;
    int8_t tmp = 0;
    wizchip_sw_reset(heth);
    if(txsize) {
        tmp = 0;
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            tmp += txsize[i];
            if(tmp > 16) {
                return -1;
            }
        }
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            setSn_TXBUF_SIZE(heth, i, txsize[i]);
        }	
    }
    if (rxsize) {
        tmp = 0;
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            tmp += rxsize[i];
            if(tmp > 16) return -1;
        }
        for (i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
            setSn_RXBUF_SIZE(heth, i, rxsize[i]);
        }
    }
    return 0;
}

void wizchip_setnetinfo(ethernetHandler_t *heth, netInfo_t* pnetinfo) {
    setSHAR(heth, pnetinfo->mac);
    setGAR(heth, pnetinfo->gw);
    setSUBR(heth, pnetinfo->sn);
    setSIPR(heth, pnetinfo->ip);
}

/**************************************************************
*               From socket
***************************************************************/

int8_t socket(ethernetHandler_t *heth, uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag) {
    uint32_t timeStamp = HAL_GetTick();
    uint32_t taddr;

    getSIPR(heth, (uint8_t*)&taddr);

    if (taddr == 0) {
        return SOCKERR_SOCKINIT;
    }

    if ((flag & 0x04) != 0) {
        return SOCKERR_SOCKFLAG;
    }

    if(flag != 0)
    {
        if ((flag & (SF_TCP_NODELAY | SF_IO_NONBLOCK)) == 0) {
            return SOCKERR_SOCKFLAG;
        }
    }

    close_socket(heth, sn);

    setSn_MR(heth, sn, (protocol | (flag & 0xF0)));

    if (!port) {
        port = heth->sock_any_port++;
        if(heth->sock_any_port == 0xFFF0) {
            heth->sock_any_port = SOCK_ANY_PORT_NUM;
        }
    }

    setSn_PORT(heth, sn,port);	
    setSn_CR(heth, sn,Sn_CR_OPEN);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_io_mode &= ~(1 <<sn);
    heth->sock_io_mode |= ((flag & SF_IO_NONBLOCK) << sn);   
    heth->sock_is_sending &= ~(1<<sn);
    heth->sock_remained_size[sn] = 0;
    sock_pack_info[sn] = PACK_COMPLETED;

    while (getSn_SR(heth, sn) == SOCK_CLOSED) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    return (int8_t)sn;
}	   

int8_t close_socket(ethernetHandler_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_CLOSE);

    while(getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    setSn_IR(heth, sn, 0xFF);
    heth->sock_io_mode &= ~(1<<sn);
    heth->sock_is_sending &= ~(1<<sn);
    heth->sock_remained_size[sn] = 0;
    sock_pack_info[sn] = 0;

    while(getSn_SR(heth, sn) != SOCK_CLOSED) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    return SOCK_OK;
}

int8_t listen(ethernetHandler_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_LISTEN);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    if (getSn_SR(heth, sn) != SOCK_LISTEN)
    {
        close_socket(heth, sn);
        return SOCKERR_SOCKCLOSED;
    }

    return SOCK_OK;
}

int8_t disconnect(ethernetHandler_t *heth, uint8_t sn) {
    uint32_t timeStamp = HAL_GetTick();

    setSn_CR(heth, sn, Sn_CR_DISCON);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_is_sending &= ~(1<<sn);

    if (heth->sock_io_mode & (1<<sn)) {
        return SOCK_BUSY;
    }

    while(getSn_SR(heth, sn) != SOCK_CLOSED)
    {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    return SOCK_OK;
}

int32_t send(ethernetHandler_t *heth, uint8_t sn, uint8_t * buf, uint16_t len) {
    uint32_t timeStamp = HAL_GetTick();
    uint8_t tmp = 0;
    uint16_t freesize = 0;
    
    tmp = getSn_SR(heth, sn);

    if (tmp != SOCK_ESTABLISHED && tmp != SOCK_CLOSE_WAIT) {
        return SOCKERR_SOCKSTATUS;
    }
    
    if (heth->sock_is_sending & (1<<sn)) {
        tmp = getSn_IR(heth, sn);
        if (tmp & Sn_IR_SENDOK) {
            setSn_IR(heth, sn, Sn_IR_SENDOK);
            heth->sock_is_sending &= ~(1<<sn);         
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
        len = freesize; // check size not to exceed MAX size.
    }

    while(1) {
        freesize = getSn_TX_FSR(heth, sn);
        tmp = getSn_SR(heth, sn);

        if ((tmp != SOCK_ESTABLISHED) && (tmp != SOCK_CLOSE_WAIT)) {
            close_socket(heth, sn);
            return SOCKERR_SOCKSTATUS;
        }

        if((heth->sock_io_mode & (1<<sn)) && (len > freesize)) {
            return SOCK_BUSY;
        }
        
        if(len <= freesize) {
            break;
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    wiz_send_data(heth, sn, buf, len);
    
    setSn_CR(heth, sn,Sn_CR_SEND);

    /* wait to process the command... */
    while(getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    heth->sock_is_sending |= (1 << sn);

    return (int32_t)len;
}

int32_t recv(ethernetHandler_t *heth, uint8_t sn, uint8_t * buf, uint16_t len) {
    uint32_t timeStamp = HAL_GetTick();
    uint8_t  tmp = 0;
    uint16_t recvsize = 0;
    
    recvsize = getSn_RxMAX(heth, sn);
    if(recvsize < len) {
        len = recvsize;
    }

    while(1) {
        recvsize = getSn_RX_RSR(heth, sn);
        tmp = getSn_SR(heth, sn);
        if (tmp != SOCK_ESTABLISHED) {
            if (tmp == SOCK_CLOSE_WAIT) {
                if(recvsize != 0) {
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

        if((heth->sock_io_mode & (1<<sn)) && (recvsize == 0)) {
            return SOCK_BUSY;
        }

        if(recvsize != 0) {
            break;
        }

        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }

    if(recvsize < len) {
        len = recvsize;
    }

    wiz_recv_data(heth, sn, buf, len);
    setSn_CR(heth, sn,Sn_CR_RECV);

    while (getSn_CR(heth, sn)) {
        if (tdiff_u32(HAL_GetTick(), timeStamp) > TIME_OUT_MS) {
            close_socket(heth, sn);
            return SOCKERR_TIMEOUT;
        }
    }
    
    return (int32_t)len;
}

int8_t getsockopt(ethernetHandler_t *heth, uint8_t sn, sockopt_type sotype, void* arg) {
    switch(sotype)
    {
        case SO_DESTIP:
            getSn_DIPR(heth, sn, (uint8_t*)arg);
            break;
        case SO_DESTPORT:  
            *(uint16_t*) arg = getSn_DPORT(heth, sn);
            break;
        case SO_STATUS:
            *(uint8_t*) arg = getSn_SR(heth, sn);
            break;
        default:
            return SOCKERR_SOCKOPT;
    }
    return SOCK_OK;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Callback function to write a byte
 * @param   heth Pointer to the ethernet handler
 * @param   hspi Pointer to associated SPI handler
 * @param   port Pointer to chip select GPIO block
 * @param   pin Chip select GPIO pin
 * @param   netInfo Network description
 * @param   sendBuf Pointer to micro-controller tx buffer
 * @return  0 on success, else negative value
 * @note    Should be called one time for each physical ethernet port
*/
int W5500Init(ethernetHandler_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, netInfo_t netInfo, char *sendBuf) {
    heth->hspi = hspi;
    heth->netInfo = netInfo;
    stmGpioInit(&heth->select, port, pin, STM_GPIO_OUTPUT);
    heth->timeStamp = 0;
    heth->sendBuf = sendBuf;

    heth->sock_any_port = SOCK_ANY_PORT_NUM;
    heth->sock_io_mode = 0;
    heth->sock_is_sending = 0;
    for (uint8_t i = 0; i < _WIZCHIP_SOCK_NUM_; i++) {
        heth->sock_remained_size[i] = 0;
    }

    // Define the buffer sizes for the 8 sockets in kB (32 kB available in total)
    static uint8_t txBufSize[8] = {8, 8, 0, 0, 0, 0, 0, 0};
    static uint8_t rxBufSize[8] = {8, 8, 0, 0, 0, 0, 0, 0};

    if (wizchip_init(heth, txBufSize, rxBufSize) != 0)
    {
        return -1;
    }

    // Necessary to let the chip initialize before sending network information
    HAL_Delay(100);

    // To send the network parameters to the W5500
    wizchip_setnetinfo(heth, &heth->netInfo);

    // TCP initialization
    for (uint16_t socketId = 0; socketId < NO_OF_SOCKETS; socketId++) {
        disconnect(heth, socketId);
    }
    noOfOpenSockets = 0;

    return 0;
}

/*!
 * @brief   Implementation of the TCP server
 * @param   heth Physical ethernet port used
 * @return  >=0 on success, else negative value
 * @note    Should be used in the while(1) loop of the board for each physical ethernet port
*/
int W5500TCPServer(ethernetHandler_t* heth) {
    static const uint8_t INVALID_SOCKET = 0xffu;
    static uint8_t activeSocket         = INVALID_SOCKET;  // Start with an invalid socket

    for (uint8_t socketId = 0; socketId < NO_OF_SOCKETS; socketId++) {
        sockets[socketId].status = getSn_SR(heth, socketId);

        switch (sockets[socketId].status) {
            case SOCK_ESTABLISHED:

                // If no active socket is set, this is the first connection
                if (activeSocket == INVALID_SOCKET) {
                    activeSocket = socketId;
                }
                // If this is the active socket, handle communication
                if (socketId == activeSocket) {
                    if (heth->newADCReady) {
                        send(heth, socketId, (uint8_t*)heth->sendBuf, strlen(heth->sendBuf));
                        heth->newADCReady = false;
                    }

                    // If the RX buffer contains data, receive it
                    if (getSn_RX_RSR(heth, socketId) > 0) {
                        recv(heth, socketId, (uint8_t*)heth->recvBuf, TCP_BUF_LEN);
                        heth->newMessage = true;
                        heth->timeStamp = HAL_GetTick();
                    }
                } else {
                    // New client detected, disconnect old one first
                    disconnect(heth, activeSocket);  // Disconnect the previous client
                    activeSocket = socketId;    // Assign the new active socket
                }
                break;
            
            case SOCK_CLOSE_WAIT:
                // Disconnect the client and reset the active socket
                disconnect(heth, socketId);
                activeSocket = INVALID_SOCKET;
                break;

            case SOCK_CLOSED:
                // Open a new TCP socket to listen for new clients
                if (socketId != activeSocket) {
                    socket(heth, socketId, Sn_MR_TCP, PORT, 0);
                }
                break;

            case SOCK_INIT:
                // Start listening for new connections
                listen(heth, socketId);
                break;

            case SOCK_LISTEN:
                // Waiting for a client connection
                break;

            default:
                sockets[socketId].error = true;
                break;
        }
    }

    return activeSocket;
}
