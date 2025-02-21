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
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "StmGpio.h"
#include "W5500.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

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

#define setSn_TXBUF_SIZE(sn, txbufsize) WIZCHIP_WRITE(Sn_TXBUF_SIZE(sn), txbufsize)
#define setSn_RXBUF_SIZE(sn, rxbufsize) WIZCHIP_WRITE(Sn_RXBUF_SIZE(sn), rxbufsize)


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

#define setMR(mr)               WIZCHIP_WRITE(MR,mr)
#define getMR()                 WIZCHIP_READ(MR)
#define setGAR(gar)             WIZCHIP_WRITE_BUF(GAR,gar,4)
#define getGAR(gar)             WIZCHIP_READ_BUF(GAR,gar,4)
#define setSUBR(subr)           WIZCHIP_WRITE_BUF(SUBR, subr,4)
#define getSUBR(subr)           WIZCHIP_READ_BUF(SUBR, subr, 4)
#define setSHAR(shar)           WIZCHIP_WRITE_BUF(SHAR, shar, 6)
#define getSHAR(shar)           WIZCHIP_READ_BUF(SHAR, shar, 6)
#define setSIPR(sipr)           WIZCHIP_WRITE_BUF(SIPR, sipr, 4)
#define getSIPR(sipr)           WIZCHIP_READ_BUF(SIPR, sipr, 4)

#define setSn_MR(sn, mr)        WIZCHIP_WRITE(Sn_MR(sn),mr)
#define getSn_MR(sn)            WIZCHIP_READ(Sn_MR(sn))
#define setSn_CR(sn, cr)        WIZCHIP_WRITE(Sn_CR(sn), cr)
#define setSn_IR(sn, ir)        WIZCHIP_WRITE(Sn_IR(sn), (ir & 0x1F))
#define getSn_CR(sn)            WIZCHIP_READ(Sn_CR(sn))
#define getSn_SR(sn)            WIZCHIP_READ(Sn_SR(sn))
#define getSn_IR(sn)            (WIZCHIP_READ(Sn_IR(sn)) & 0x1F)
#define getSn_TXBUF_SIZE(sn)    WIZCHIP_READ(Sn_TXBUF_SIZE(sn))
#define getSn_RXBUF_SIZE(sn)    WIZCHIP_READ(Sn_RXBUF_SIZE(sn))
#define getSn_TX_WR(sn)         (((uint16_t)WIZCHIP_READ(Sn_TX_WR(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_WR(sn),1)))	
#define getSn_TxMAX(sn)         (((uint16_t)getSn_TXBUF_SIZE(sn)) << 10)
#define getSn_RxMAX(sn)         (((uint16_t)getSn_RXBUF_SIZE(sn)) << 10)
#define getSn_DPORT(sn)         (((uint16_t)WIZCHIP_READ(Sn_DPORT(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_DPORT(sn),1)))
#define getSn_DIPR(sn, dipr)    WIZCHIP_READ_BUF(Sn_DIPR(sn), dipr, 4)
#define setSn_PORT(sn, port)  { WIZCHIP_WRITE(Sn_PORT(sn),   (uint8_t)(port >> 8)); \
                                WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_PORT(sn),1), (uint8_t) port); }
#define setSn_TX_WR(sn, txwr) { WIZCHIP_WRITE(Sn_TX_WR(sn),   (uint8_t)(txwr>>8)); \
                                WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_TX_WR(sn),1), (uint8_t) txwr); }
#define setSn_RX_RD(sn, rxrd) { WIZCHIP_WRITE(Sn_RX_RD(sn),   (uint8_t)(rxrd>>8)); \
                                WIZCHIP_WRITE(WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1), (uint8_t) rxrd); }
#define getSn_RX_RD(sn)         (((uint16_t)WIZCHIP_READ(Sn_RX_RD(sn)) << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RD(sn),1)))

// wizchip_conf

#define _WIZCHIP_IO_MODE_NONE_          0x0000
#define _WIZCHIP_IO_MODE_BUS_           0x0100 /**< Bus interface mode */
#define _WIZCHIP_IO_MODE_SPI_           0x0200 /**< SPI interface mode */

#define _WIZCHIP_IO_MODE_SPI_VDM_       (_WIZCHIP_IO_MODE_SPI_ + 1) /**< SPI interface mode for variable length data*/
#define _WIZCHIP_IO_MODE_SPI_FDM_       (_WIZCHIP_IO_MODE_SPI_ + 2) /**< SPI interface mode for fixed length data mode*/
#define _WIZCHIP_IO_MODE_SPI_5500_      (_WIZCHIP_IO_MODE_SPI_ + 3) /**< SPI interface mode for fixed length data mode*/

#define _WIZCHIP_ID_                    "W5500\0"

#define _WIZCHIP_IO_MODE_               _WIZCHIP_IO_MODE_SPI_VDM_

typedef   uint8_t   iodata_t;

typedef struct __WIZCHIP
{
    uint16_t  if_mode;               ///< host interface mode
    uint8_t   id[8];                 ///< @b WIZCHIP ID such as @b 5100, @b 5100S, @b 5200, @b 5500, and so on.
    struct _CRIS {
        void (*_enter)  (void);       ///< crtical section enter 
        void (*_exit) (void);         ///< critial section exit  
    }CRIS;
    struct _CS {
        void (*_select)  (void);      ///< @ref \_WIZCHIP_ selected
        void (*_deselect)(void);      ///< @ref \_WIZCHIP_ deselected
    }CS;
    union _IF {	  
        struct {
            iodata_t  (*_read_data)   (uint32_t AddrSel);
            void      (*_write_data)  (uint32_t AddrSel, iodata_t wb);
        } BUS;      
        struct {
            uint8_t (*_read_byte)   (void);
            void    (*_write_byte)  (uint8_t wb);
            void    (*_read_burst)  (uint8_t* pBuf, uint16_t len);
            void    (*_write_burst) (uint8_t* pBuf, uint16_t len);
        } SPI;
    }IF;
}_WIZCHIP;

// socket

#define CHECK_SOCKNUM()         do{ if(sn > _WIZCHIP_SOCK_NUM_)         return SOCKERR_SOCKNUM;     } while(0);
#define CHECK_SOCKMODE(mode)    do{ if((getSn_MR(sn) & 0x0F) != mode)   return SOCKERR_SOCKMODE;    } while(0);
#define CHECK_SOCKINIT()        do{ if((getSn_SR(sn) != SOCK_INIT))     return SOCKERR_SOCKINIT;    } while(0);
#define CHECK_SOCKDATA()        do{ if(len == 0)                        return SOCKERR_DATALEN;     } while(0);

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

#define SOCKET                uint8_t  ///< SOCKET type define for legacy driver

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
#define SF_ETHER_OWN           (Sn_MR_MFEN)        ///< In @ref Sn_MR_MACRAW, Receive only the packet as broadcast, multicast and own packet
#define SF_IGMP_VER2           (Sn_MR_MC)          ///< In @ref Sn_MR_UDP with \ref SF_MULTI_ENABLE, Select IGMP version 2.   
#define SF_TCP_NODELAY         (Sn_MR_ND)          ///< In @ref Sn_MR_TCP, Use to nodelayed ack.
#define SF_MULTI_ENABLE        (Sn_MR_MULTI)       ///< In @ref Sn_MR_UDP, Enable multicast mode.

#define SF_BROAD_BLOCK         (Sn_MR_BCASTB)   ///< In @ref Sn_MR_UDP or @ref Sn_MR_MACRAW, Block broadcast packet. Valid only in W5500
#define SF_MULTI_BLOCK         (Sn_MR_MMB)      ///< In @ref Sn_MR_MACRAW, Block multicast packet. Valid only in W5500
#define SF_IPv6_BLOCK          (Sn_MR_MIP6B)    ///< In @ref Sn_MR_MACRAW, Block IPv6 packet. Valid only in W5500
#define SF_UNI_BLOCK           (Sn_MR_UCASTB)   ///< In @ref Sn_MR_UDP with \ref SF_MULTI_ENABLE. Valid only in W5500

#define SF_IO_NONBLOCK           0x01              ///< Socket nonblock io mode. It used parameter in \ref socket().

/*
 * UDP & MACRAW Packet Infomation
 */
#define PACK_FIRST               0x80              ///< In Non-TCP packet, It indicates to start receiving a packet. (When W5300, This flag can be applied)
#define PACK_REMAINED            0x01              ///< In Non-TCP packet, It indicates to remain a packet to be received. (When W5300, This flag can be applied)
#define PACK_COMPLETED           0x00              ///< In Non-TCP packet, It indicates to complete to receive a packet. (When W5300, This flag can be applied)

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void wizchipSelect();
static void wizchipUnselect();
static void wizchipReadBurst(uint8_t* buff, uint16_t len);
static void wizchipWriteBurst(uint8_t* buff, uint16_t len);
static void ethPortSelect(ethernetHandler_t *heth);

// W5500
uint8_t WIZCHIP_READ(uint32_t AddrSel);
void WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb);
void WIZCHIP_READ_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len);
void WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len);
uint16_t getSn_TX_FSR(uint8_t sn);
uint16_t getSn_RX_RSR(uint8_t sn);
void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len);
void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len);

// wizchip_conf
static_wizchip_conf_t get_static_wizchip_conf(void);
void set_static_wizchip_conf(static_wizchip_conf_t static_wizchip_conf);
void wizchip_cris_enter(void);
void wizchip_cris_exit(void);
void wizchip_cs_select(void);
void wizchip_cs_deselect(void);
iodata_t wizchip_bus_readdata(uint32_t AddrSel);
void wizchip_bus_writedata(uint32_t AddrSel, iodata_t wb);
static void reg_wizchip_cs_cbfunc(void(*cs_sel)(void), void(*cs_desel)(void));
static void reg_wizchip_spi_cbfunc(uint8_t (*spi_rb)(void), void (*spi_wb)(uint8_t wb));
static void reg_wizchip_spiburst_cbfunc(void (*spi_rb)(uint8_t* pBuf, uint16_t len), void (*spi_wb)(uint8_t* pBuf, uint16_t len));
static void wizchip_sw_reset(void);
static int8_t wizchip_init(uint8_t* txsize, uint8_t* rxsize);
void wizchip_setnetinfo(wiz_NetInfo* pnetinfo);

// socket
static_socket_t get_static_socket(void);
void set_static_socket(static_socket_t static_socket);
int8_t socket(uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag);
int8_t close_socket(uint8_t sn);
int8_t listen(uint8_t sn);
int8_t disconnect(uint8_t sn);
int32_t send(uint8_t sn, uint8_t *buf, uint16_t len);
int32_t recv(uint8_t sn, uint8_t *buf, uint16_t len);
int8_t  getsockopt(uint8_t sn, sockopt_type sotype, void* arg);

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

// Can change over time is more than one physical Ethernet port is used
static ethernetHandler_t *currentEthernet = NULL;

// wizchip_conf

_WIZCHIP WIZCHIP =
{
    _WIZCHIP_IO_MODE_,
    _WIZCHIP_ID_ ,
    {
        wizchip_cris_enter,
        wizchip_cris_exit
    },
    {
        wizchip_cs_select,
        wizchip_cs_deselect
    },
    {
        {
            wizchip_bus_readdata,
            wizchip_bus_writedata
        },

    }
};

static uint8_t    _DNS_[4];      // DNS server ip address
static dhcp_mode  _DHCP_;        // DHCP mode

// socket

static uint16_t sock_any_port = SOCK_ANY_PORT_NUM;
static uint16_t sock_io_mode = 0;
static uint16_t sock_is_sending = 0;
static uint16_t sock_remained_size[_WIZCHIP_SOCK_NUM_] = {0,0,};
uint8_t  sock_pack_info[_WIZCHIP_SOCK_NUM_] = {0,};

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief   Callback function to select the W5500 SPI peripheral
*/
static void wizchipSelect()
{
    // Chip selected when the pin is grounded
    stmSetGpio(currentEthernet->select, false);
}

/*!
 * @brief   Callback function to unselect the W5500 SPI peripheral
*/
static void wizchipUnselect()
{
    stmSetGpio(currentEthernet->select, true);
}

/*!
 * @brief   Callback function to read a multi-bytes message
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
*/
static void wizchipReadBurst(uint8_t* buff, uint16_t len)
{
    HAL_SPI_Receive(currentEthernet->hspi, buff, len, HAL_MAX_DELAY);
}

/*!
 * @brief   Callback function to write a multi-bytes message
 * @param   buff Pointer to the buffer
 * @param   len Maximum length of the buffer
*/
static void wizchipWriteBurst(uint8_t* buff, uint16_t len)
{
    HAL_SPI_Transmit(currentEthernet->hspi, buff, len, HAL_MAX_DELAY);
}

/*!
 * @brief   Callback function to read a byte
 * @return  The byte that has been read
*/
static uint8_t wizchipReadByte()
{
    static uint8_t byte;
    wizchipReadBurst(&byte, sizeof(byte));
    return byte;
}

/*!
 * @brief   Callback function to write a byte
 * @param   byte The byte to be written
*/
static void wizchipWriteByte(uint8_t byte)
{
    wizchipWriteBurst(&byte, sizeof(byte));
}

/*!
 * @brief   Selection of the ethernet port
 * @param   heth Pointer to the ethernet handler to be selected
*/
static void ethPortSelect(ethernetHandler_t *heth)
{
    currentEthernet->static_wizchip_conf = get_static_wizchip_conf();
    currentEthernet->static_socket = get_static_socket();

    currentEthernet = heth;

    set_static_wizchip_conf(currentEthernet->static_wizchip_conf);
    set_static_socket(currentEthernet->static_socket);
}

/**************************************************************
*               From W5500
***************************************************************/

uint8_t WIZCHIP_READ(uint32_t AddrSel)
{
    uint8_t ret;
    uint8_t spi_data[3];

    WIZCHIP.CS._select();

    AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

    if(!WIZCHIP.IF.SPI._read_burst || !WIZCHIP.IF.SPI._write_burst) 	// byte operation
    {
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
    }
    else																// burst operation
    {
        spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
        spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
        spi_data[2] = (AddrSel & 0x000000FF) >> 0;
        WIZCHIP.IF.SPI._write_burst(spi_data, 3);
    }
    ret = WIZCHIP.IF.SPI._read_byte();

    WIZCHIP.CS._deselect();
    return ret;
}

void WIZCHIP_WRITE(uint32_t AddrSel, uint8_t wb)
{
    uint8_t spi_data[4];
    WIZCHIP.CS._select();

    AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

    if(!WIZCHIP.IF.SPI._write_burst) 	// byte operation
    {
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
        WIZCHIP.IF.SPI._write_byte(wb);
    }
    else									// burst operation
    {
        spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
        spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
        spi_data[2] = (AddrSel & 0x000000FF) >> 0;
        spi_data[3] = wb;
        WIZCHIP.IF.SPI._write_burst(spi_data, 4);
    }

    WIZCHIP.CS._deselect();
}
            
void WIZCHIP_READ_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
    uint8_t spi_data[3];
    uint16_t i;

    WIZCHIP.CS._select();

    AddrSel |= (_W5500_SPI_READ_ | _W5500_SPI_VDM_OP_);

    if(!WIZCHIP.IF.SPI._read_burst || !WIZCHIP.IF.SPI._write_burst) 	// byte operation
    {
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
        for(i = 0; i < len; i++)
            pBuf[i] = WIZCHIP.IF.SPI._read_byte();
    }
    else																// burst operation
    {
        spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
        spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
        spi_data[2] = (AddrSel & 0x000000FF) >> 0;
        WIZCHIP.IF.SPI._write_burst(spi_data, 3);
        WIZCHIP.IF.SPI._read_burst(pBuf, len);
    }

    WIZCHIP.CS._deselect();
}

void WIZCHIP_WRITE_BUF(uint32_t AddrSel, uint8_t* pBuf, uint16_t len)
{
    uint8_t spi_data[3];
    uint16_t i;

    WIZCHIP.CS._select();

    AddrSel |= (_W5500_SPI_WRITE_ | _W5500_SPI_VDM_OP_);

    if(!WIZCHIP.IF.SPI._write_burst) 	// byte operation
    {
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x00FF0000) >> 16);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x0000FF00) >>  8);
        WIZCHIP.IF.SPI._write_byte((AddrSel & 0x000000FF) >>  0);
        for(i = 0; i < len; i++)
            WIZCHIP.IF.SPI._write_byte(pBuf[i]);
    }
    else									// burst operation
    {
        spi_data[0] = (AddrSel & 0x00FF0000) >> 16;
        spi_data[1] = (AddrSel & 0x0000FF00) >> 8;
        spi_data[2] = (AddrSel & 0x000000FF) >> 0;
        WIZCHIP.IF.SPI._write_burst(spi_data, 3);
        WIZCHIP.IF.SPI._write_burst(pBuf, len);
    }

    WIZCHIP.CS._deselect();
}

uint16_t getSn_TX_FSR(uint8_t sn)
{
    uint16_t val=0,val1=0;

    do
    {
        val1 = WIZCHIP_READ(Sn_TX_FSR(sn));
        val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
        if (val1 != 0)
        {
          val = WIZCHIP_READ(Sn_TX_FSR(sn));
          val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_TX_FSR(sn),1));
        }
    }while (val != val1);
    return val;
}

uint16_t getSn_RX_RSR(uint8_t sn)
{
    uint16_t val=0,val1=0;

    do
    {
        val1 = WIZCHIP_READ(Sn_RX_RSR(sn));
        val1 = (val1 << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
        if (val1 != 0)
        {
          val = WIZCHIP_READ(Sn_RX_RSR(sn));
          val = (val << 8) + WIZCHIP_READ(WIZCHIP_OFFSET_INC(Sn_RX_RSR(sn),1));
        }
    }while (val != val1);
    return val;
}

void wiz_send_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
    uint16_t ptr = 0;
    uint32_t addrsel = 0;
    if(len == 0)  return;
    ptr = getSn_TX_WR(sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_TXBUF_BLOCK(sn) << 3);
    WIZCHIP_WRITE_BUF(addrsel,wizdata, len);
    ptr += len;
    setSn_TX_WR(sn,ptr);
}

void wiz_recv_data(uint8_t sn, uint8_t *wizdata, uint16_t len)
{
    uint16_t ptr = 0;
    uint32_t addrsel = 0;
    if(len == 0) return;
    ptr = getSn_RX_RD(sn);
    addrsel = ((uint32_t)ptr << 8) + (WIZCHIP_RXBUF_BLOCK(sn) << 3);
    WIZCHIP_READ_BUF(addrsel, wizdata, len);
    ptr += len;
    setSn_RX_RD(sn,ptr);
}

/**************************************************************
*               From wizchip_conf
***************************************************************/

static_wizchip_conf_t get_static_wizchip_conf(void)
{
    static_wizchip_conf_t static_wizchip_conf;
    for (int i = 0; i < 4; i++)
    {
        static_wizchip_conf.dns[i] = _DNS_[i];
    }
    static_wizchip_conf.dhcp = _DHCP_;
    return static_wizchip_conf;
}

void set_static_wizchip_conf(static_wizchip_conf_t static_wizchip_conf)
{
    for (int i = 0; i < 4; i++)
    {
        _DNS_[i] = static_wizchip_conf.dns[i];
    }
    _DHCP_ = static_wizchip_conf.dhcp;
}

void wizchip_cris_enter(void) {}

void wizchip_cris_exit(void) {}

void wizchip_cs_select(void) {}

void wizchip_cs_deselect(void) {}

iodata_t wizchip_bus_readdata(uint32_t AddrSel) { return * ((volatile iodata_t *)((ptrdiff_t) AddrSel)); }

void wizchip_bus_writedata(uint32_t AddrSel, iodata_t wb) { *((volatile iodata_t*)((ptrdiff_t)AddrSel)) = wb; }

static void reg_wizchip_cs_cbfunc(void(*cs_sel)(void), void(*cs_desel)(void))
{
    WIZCHIP.CS._select   = cs_sel;
    WIZCHIP.CS._deselect = cs_desel;
}

static void reg_wizchip_spi_cbfunc(uint8_t (*spi_rb)(void), void (*spi_wb)(uint8_t wb))
{
    WIZCHIP.IF.SPI._read_byte   = spi_rb;
    WIZCHIP.IF.SPI._write_byte  = spi_wb;
}

static void reg_wizchip_spiburst_cbfunc(void (*spi_rb)(uint8_t* pBuf, uint16_t len), void (*spi_wb)(uint8_t* pBuf, uint16_t len))
{
    WIZCHIP.IF.SPI._read_burst   = spi_rb;
    WIZCHIP.IF.SPI._write_burst  = spi_wb;
}

static void wizchip_sw_reset(void)
{
    uint8_t gw[4], sn[4], sip[4];
    uint8_t mac[6];
    getSHAR(mac);
    getGAR(gw);
    getSUBR(sn);
    getSIPR(sip);
    setMR(MR_RST);
    getMR(); // for delay
    setSHAR(mac);
    setGAR(gw);
    setSUBR(sn);
    setSIPR(sip);
}

static int8_t wizchip_init(uint8_t* txsize, uint8_t* rxsize)
{
    int8_t i;
    int8_t tmp = 0;
    wizchip_sw_reset();
    if(txsize)
    {
        tmp = 0;
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
        {
            tmp += txsize[i];
            if(tmp > 16) return -1;
        }
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
        {
            setSn_TXBUF_SIZE(i, txsize[i]);
        }	
    }
    if(rxsize)
    {
        tmp = 0;
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
        {
            tmp += rxsize[i];
            if(tmp > 16) return -1;
        }
        for(i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
        {
            setSn_RXBUF_SIZE(i, rxsize[i]);
        }
    }
    return 0;
}

void wizchip_setnetinfo(wiz_NetInfo* pnetinfo)
{
    setSHAR(pnetinfo->mac);
    setGAR(pnetinfo->gw);
    setSUBR(pnetinfo->sn);
    setSIPR(pnetinfo->ip);
    _DNS_[0] = pnetinfo->dns[0];
    _DNS_[1] = pnetinfo->dns[1];
    _DNS_[2] = pnetinfo->dns[2];
    _DNS_[3] = pnetinfo->dns[3];
    _DHCP_   = pnetinfo->dhcp;
}

/**************************************************************
*               From socket
***************************************************************/

static_socket_t get_static_socket(void)
{
    static_socket_t static_socket;
    static_socket.sockAnyPort = sock_any_port;
    static_socket.sockIoMode = sock_io_mode;
    static_socket.sockIsSending = sock_is_sending;
    for (int i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
    {
        static_socket.sockRemainedSize[i] = sock_remained_size[i];
    }
    return static_socket;
}

void set_static_socket(static_socket_t static_socket)
{
    sock_any_port = static_socket.sockAnyPort;
    sock_io_mode = static_socket.sockIoMode;
    sock_is_sending = static_socket.sockIsSending;
    for (int i = 0; i < _WIZCHIP_SOCK_NUM_; i++)
    {
        sock_remained_size[i] = static_socket.sockRemainedSize[i];
    }
}

int8_t socket(uint8_t sn, uint8_t protocol, uint16_t port, uint8_t flag)
{
    CHECK_SOCKNUM();
    switch(protocol)
    {
        case Sn_MR_TCP :
            {
                uint32_t taddr;
                getSIPR((uint8_t*)&taddr);
                if(taddr == 0) return SOCKERR_SOCKINIT;
         break;
            }
        case Sn_MR_UDP :
        case Sn_MR_MACRAW :
        case Sn_MR_IPRAW :
            break;
        default :
            return SOCKERR_SOCKMODE;
    }
    if((flag & 0x04) != 0) return SOCKERR_SOCKFLAG;
        
    if(flag != 0)
    {
        switch(protocol)
        {
            case Sn_MR_TCP:
                  if((flag & (SF_TCP_NODELAY|SF_IO_NONBLOCK))==0) return SOCKERR_SOCKFLAG;
                break;
            case Sn_MR_UDP:
                if(flag & SF_IGMP_VER2)
                {
                    if((flag & SF_MULTI_ENABLE)==0) return SOCKERR_SOCKFLAG;
                }
                if(flag & SF_UNI_BLOCK)
                {
                    if((flag & SF_MULTI_ENABLE) == 0) return SOCKERR_SOCKFLAG;
                }
                break;
            default:
                break;
        }
    }
    close_socket(sn);
    setSn_MR(sn, (protocol | (flag & 0xF0)));
    if(!port)
    {
        port = sock_any_port++;
        if(sock_any_port == 0xFFF0) sock_any_port = SOCK_ANY_PORT_NUM;
    }
    setSn_PORT(sn,port);	
    setSn_CR(sn,Sn_CR_OPEN);
    while(getSn_CR(sn));
    sock_io_mode &= ~(1 <<sn);
    sock_io_mode |= ((flag & SF_IO_NONBLOCK) << sn);   
    sock_is_sending &= ~(1<<sn);
    sock_remained_size[sn] = 0;
    sock_pack_info[sn] = PACK_COMPLETED;
    while(getSn_SR(sn) == SOCK_CLOSED);
    return (int8_t)sn;
}	   

int8_t close_socket(uint8_t sn)
{
    CHECK_SOCKNUM();
    setSn_CR(sn,Sn_CR_CLOSE);
    while( getSn_CR(sn) );
    setSn_IR(sn, 0xFF);
    sock_io_mode &= ~(1<<sn);
    sock_is_sending &= ~(1<<sn);
    sock_remained_size[sn] = 0;
    sock_pack_info[sn] = 0;
    while(getSn_SR(sn) != SOCK_CLOSED);
    return SOCK_OK;
}

int8_t listen(uint8_t sn)
{
    CHECK_SOCKNUM();
    CHECK_SOCKMODE(Sn_MR_TCP);
    CHECK_SOCKINIT();
    setSn_CR(sn,Sn_CR_LISTEN);
    while(getSn_CR(sn));
    while(getSn_SR(sn) != SOCK_LISTEN)
    {
        close_socket(sn);
        return SOCKERR_SOCKCLOSED;
    }
    return SOCK_OK;
}

int8_t disconnect(uint8_t sn)
{
    CHECK_SOCKNUM();
    CHECK_SOCKMODE(Sn_MR_TCP);
    setSn_CR(sn,Sn_CR_DISCON);
    /* wait to process the command... */
    while(getSn_CR(sn));
    sock_is_sending &= ~(1<<sn);
    if(sock_io_mode & (1<<sn)) return SOCK_BUSY;
    while(getSn_SR(sn) != SOCK_CLOSED)
    {
        if(getSn_IR(sn) & Sn_IR_TIMEOUT)
        {
            close_socket(sn);
            return SOCKERR_TIMEOUT;
        }
    }
    return SOCK_OK;
}

int32_t send(uint8_t sn, uint8_t * buf, uint16_t len)
{
    uint8_t tmp=0;
    uint16_t freesize=0;
    
    CHECK_SOCKNUM();
    CHECK_SOCKMODE(Sn_MR_TCP);
    CHECK_SOCKDATA();
    tmp = getSn_SR(sn);
    if(tmp != SOCK_ESTABLISHED && tmp != SOCK_CLOSE_WAIT) return SOCKERR_SOCKSTATUS;
    if( sock_is_sending & (1<<sn) )
    {
        tmp = getSn_IR(sn);
        if(tmp & Sn_IR_SENDOK)
        {
            setSn_IR(sn, Sn_IR_SENDOK);
            sock_is_sending &= ~(1<<sn);         
        }
        else if(tmp & Sn_IR_TIMEOUT)
        {
            close_socket(sn);
            return SOCKERR_TIMEOUT;
        }
        else return SOCK_BUSY;
    }
    freesize = getSn_TxMAX(sn);
    if (len > freesize) len = freesize; // check size not to exceed MAX size.
    while(1)
    {
        freesize = getSn_TX_FSR(sn);
        tmp = getSn_SR(sn);
        if ((tmp != SOCK_ESTABLISHED) && (tmp != SOCK_CLOSE_WAIT))
        {
            close_socket(sn);
            return SOCKERR_SOCKSTATUS;
        }
        if( (sock_io_mode & (1<<sn)) && (len > freesize) ) return SOCK_BUSY;
        if(len <= freesize) break;
    }
    wiz_send_data(sn, buf, len);
    
    setSn_CR(sn,Sn_CR_SEND);
    /* wait to process the command... */
    while(getSn_CR(sn));
    sock_is_sending |= (1 << sn);
    //M20150409 : Explicit Type Casting
    //return len;
    return (int32_t)len;
}

int32_t recv(uint8_t sn, uint8_t * buf, uint16_t len)
{
    uint8_t  tmp = 0;
    uint16_t recvsize = 0;

    CHECK_SOCKNUM();
    CHECK_SOCKMODE(Sn_MR_TCP);
    CHECK_SOCKDATA();
    
    recvsize = getSn_RxMAX(sn);
    if(recvsize < len) len = recvsize;
    while(1)
    {
        recvsize = getSn_RX_RSR(sn);
        tmp = getSn_SR(sn);
        if (tmp != SOCK_ESTABLISHED)
        {
            if(tmp == SOCK_CLOSE_WAIT)
            {
                if(recvsize != 0) break;
                else if(getSn_TX_FSR(sn) == getSn_TxMAX(sn))
                {
                    close_socket(sn);
                    return SOCKERR_SOCKSTATUS;
                }
            }
            else
            {
                close_socket(sn);
                return SOCKERR_SOCKSTATUS;
            }
        }
        if((sock_io_mode & (1<<sn)) && (recvsize == 0)) return SOCK_BUSY;
        if(recvsize != 0) break;
    };

    if(recvsize < len) len = recvsize;   
    wiz_recv_data(sn, buf, len);
    setSn_CR(sn,Sn_CR_RECV);
    while(getSn_CR(sn));
    
    return (int32_t)len;
}

int8_t  getsockopt(uint8_t sn, sockopt_type sotype, void* arg)
{
    CHECK_SOCKNUM();
    switch(sotype)
    {
        case SO_DESTIP:
            getSn_DIPR(sn, (uint8_t*)arg);
            break;
        case SO_DESTPORT:  
            *(uint16_t*) arg = getSn_DPORT(sn);
            break;
        case SO_STATUS:
            *(uint8_t*) arg = getSn_SR(sn);
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
int W5500Init(ethernetHandler_t *heth, SPI_HandleTypeDef *hspi, GPIO_TypeDef *port, uint16_t pin, wiz_NetInfo netInfo, char *sendBuf)
{
    currentEthernet = heth;

    currentEthernet->hspi = hspi;
    currentEthernet->netInfo = netInfo;
    stmGpioInit(&currentEthernet->select, port, pin, STM_GPIO_OUTPUT);
    currentEthernet->static_wizchip_conf = get_static_wizchip_conf();
    currentEthernet->static_socket = get_static_socket();
    currentEthernet->timeStamp = 0;
    currentEthernet->sendBuf = sendBuf;

    // Register W5500 callbacks
    reg_wizchip_cs_cbfunc(wizchipSelect, wizchipUnselect);
    reg_wizchip_spi_cbfunc(wizchipReadByte, wizchipWriteByte);
    reg_wizchip_spiburst_cbfunc(wizchipReadBurst, wizchipWriteBurst);

    // Define the buffer sizes for the 8 sockets in kB (32 kB available in total)
    static uint8_t txBufSize[8] = {2};
    static uint8_t rxBufSize[8] = {2};

    if (wizchip_init(txBufSize, rxBufSize) != 0)
    {
        return -1;
    }

    // To send the network parameters to the W5500
    wizchip_setnetinfo(&currentEthernet->netInfo);

    return 0;
}

/*!
 * @brief   Implementation of the TCP server
 * @param   heth Physical ethernet port used
 * @return  >=0 on success, else negative value
 * @note    Should be used in the while(1) loop of the board for each physical ethernet port
*/
int W5500TCPServer(ethernetHandler_t *heth)
{
    ethPortSelect(heth);

    static uint8_t socketNumber = 0;                // 8 sockets per ethernet are supported by the w5500 chip. Only one of them is implemented
    static const uint16_t PORT = 5000;              // Communication port

    static uint8_t remoteIP[4] = {0};               // IP address of the client
    static uint16_t remotePort = 0;                 // Port of the client

    uint8_t socketStatus = getSn_SR(socketNumber);  // Socket status register

    // Client connected
    if (socketStatus == SOCK_ESTABLISHED)
    {
        // To get the client's information
        getsockopt(socketNumber, SO_DESTIP, &remoteIP);
        getsockopt(socketNumber, SO_DESTPORT, &remotePort);

        // Should be every 100ms
        if (currentEthernet->newADCReady)
        {
            send(socketNumber, (uint8_t *) currentEthernet->sendBuf, strlen(currentEthernet->sendBuf));
            currentEthernet->newADCReady = false;
        }

        // If the RX buffer of the W5500 contains a message
        if (getSn_RX_RSR(socketNumber) > 0)
        {
            recv(socketNumber, (uint8_t *) currentEthernet->recvBuf, TCP_BUF_LEN);
            currentEthernet->newMessage = true;
            currentEthernet->timeStamp = HAL_GetTick();
        }
        return 0;
    }
    else if (socketStatus == SOCK_CLOSED)
    {
        socket(socketNumber, Sn_MR_TCP, PORT, 0); // Opening a new TCP socket
        return 1;
    }
    else if (socketStatus == SOCK_INIT)
    {
        listen(socketNumber); // Listening for a client connection
        return 2;
    }
    else if (socketStatus == SOCK_CLOSE_WAIT)
    {
        disconnect(socketNumber);
        return 3;
    }
    else if (socketStatus == SOCK_LISTEN) // Waiting for a connection request
    {
        return 4;
    }
    return -1;
}
