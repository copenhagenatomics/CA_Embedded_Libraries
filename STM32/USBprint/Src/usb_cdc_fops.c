/*!
** @file usb_cdc_fops.h
** @author Luke W
** @date   Apr 2024
*/

#include <assert.h>
#include <stdbool.h>

#if defined(STM32F401xC)
  #include "stm32f4xx_hal.h"
#elif defined(STM32H753xx)
  #include "stm32h7xx_hal.h"
#endif

#include "usb_cdc_fops.h"
#include "circular_buffer.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define CDC_INIT_TIME 10

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int8_t CDC_Init_FS(void);
static int8_t CDC_DeInit_FS(void);
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
static int8_t CDC_Receive_FS(uint8_t* pbuf, uint32_t *Len);
static int8_t CDC_TransmitCplt_FS(uint8_t *pbuf, uint32_t *Len, uint8_t epnum);

/***************************************************************************************************
** PUBLIC OBJECTS
***************************************************************************************************/

extern USBD_HandleTypeDef hUsbDeviceFS;
USBD_CDC_ItfTypeDef usb_cdc_fops =
{
        CDC_Init_FS,
        CDC_DeInit_FS,
        CDC_Control_FS,
        CDC_Receive_FS,
        CDC_TransmitCplt_FS
};

/***************************************************************************************************
** PRIVATE OBJECTS
***************************************************************************************************/

static USBD_CDC_LineCodingTypeDef LineCoding = {
        115200, /* baud rate     */
        0x00,   /* stop bits-1   */
        0x00,   /* parity - none */
        0x08    /* nb. of bits 8 */
};

typedef enum {
        closed,
        preOpen,
        open
    } comport_t;

// Internal data for rx/tx
static struct
{
    struct {
        cbuf_handle_t ctx;
        uint8_t irqBuf[CIRCULAR_BUFFER_SIZE];   // lower layer buffer for IRQ USB_CDC driver callback
    } tx, rx;
    comport_t isComPortOpen;
    unsigned long portOpenTime;
} usb_cdc_if = { {0}, {0}, closed, 0};

static volatile uint32_t usb_error = CDC_ERROR_NONE;

static circular_buf_t   tx_cb;
static uint8_t          tx_buf[CIRCULAR_BUFFER_SIZE] = {0};
static circular_buf_t   rx_cb;
static uint8_t          rx_buf[CIRCULAR_BUFFER_SIZE] = {0};


/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

bool isComPortOpen() {
    // The USB CDC needs to be initialised and open for some time before the COM port is opened properly
    if (usb_cdc_if.isComPortOpen == open ||
       (HAL_GetTick() - usb_cdc_if.portOpenTime > CDC_INIT_TIME && usb_cdc_if.isComPortOpen == preOpen))
    {
        usb_cdc_if.isComPortOpen = open;
        return true;
    }
    return false;
}

void usb_cdc_rx_flush()
{
    if (!usb_cdc_if.tx.ctx)
        return; // Error, USB CDC is not initialized

    circular_buf_reset(usb_cdc_if.rx.ctx);
}

int usb_cdc_rx(uint8_t* rxByte)
{
    if (!usb_cdc_if.tx.ctx)
        return -1; // Error, USB CDC is not initialized

    return circular_buf_get(usb_cdc_if.rx.ctx, rxByte);
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to send over USB IN endpoint are sent over CDC interface
  *         through this function.
  *
  * @param  Buf: Buffer of data to be sent
  * @param  Len: Number of data to be sent (in bytes)
  * @retval USBD_OK if all operations are OK else USBD_FAIL or USBD_BUSY
  */
ssize_t usb_cdc_transmit(const uint8_t* Buf, uint16_t Len)
{
    if (!usb_cdc_if.tx.ctx)
        return -1; // Error, USB CDC is not initialized

    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;

    if (hcdc->TxState != 0)
    {
        // If the buffer does not contain enough space to hold the message return.
        if (circular_buf_capacity(usb_cdc_if.tx.ctx) < Len)
        {
            return -2;
        }
        // USB CDC is transmitting data to the network. Leave transmit handling to CDC_TransmitCplt_FS
        for (int len = 0;len < Len; len++)
        {
            if (circular_buf_put(usb_cdc_if.tx.ctx, *Buf))
                return len; // len < Len since not enough space in buffer. Should never occur.
            Buf++;
        }
        return Len;
    }

    // Fill in the data from buffer directly, no need copy bytes
    if (Len > sizeof(usb_cdc_if.tx.irqBuf))
    {
        // remaining bytes could be moved to circular buffer but in this case,
        // system is possible in a lack of resources. That problem can not be solved hear.
        Len = sizeof(usb_cdc_if.tx.irqBuf);
        usb_error |= CDC_ERROR_CROPPED_TRANSMIT;
    }
    else {
        usb_error &= ~CDC_ERROR_CROPPED_TRANSMIT;
    }

    memcpy(usb_cdc_if.tx.irqBuf, Buf, Len);
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, usb_cdc_if.tx.irqBuf, Len);
    if (USBD_CDC_TransmitPacket(&hUsbDeviceFS) != USBD_OK) {
        usb_error |= CDC_ERROR_TRANSMIT;
        return -1; // Something went wrong in IO layer.
    }
    else {
        usb_error &= ~(CDC_ERROR_DELAYED_TRANSMIT | CDC_ERROR_TRANSMIT);
    }

    // All good.

    return Len;
}

size_t usb_cdc_tx_available()
{
    if (!usb_cdc_if.tx.ctx)
        return 0; // Error, USB CDC is not initialised, for now return 0.

    return circular_buf_capacity(usb_cdc_if.tx.ctx) - circular_buf_size(usb_cdc_if.tx.ctx);
}


uint32_t isCdcError() {
    return usb_error;
}

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/**
  * @brief  Initializes the CDC media low layer over the FS USB IP
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Init_FS(void)
{
    /* Only way subsequent USBD_CDC functions can fail is if this parameter is NULL */
    assert(hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId] != NULL);
    
    // Setup TX Buffer
    USBD_CDC_SetTxBuffer(&hUsbDeviceFS, usb_cdc_if.tx.irqBuf, 0);
    usb_cdc_if.tx.ctx = circular_buf_init_static(&tx_cb, tx_buf, CIRCULAR_BUFFER_SIZE);

    // Setup RX Buffer
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, usb_cdc_if.rx.irqBuf);
    usb_cdc_if.rx.ctx = circular_buf_init_static(&rx_cb, rx_buf, CIRCULAR_BUFFER_SIZE);

    // Default is no host attached.
    usb_cdc_if.isComPortOpen = closed;

    return (USBD_OK);
}

/**
  * @brief  DeInitializes the CDC media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_DeInit_FS(void)
{
    // Nothing hear, never called.
    return (USBD_OK);
}

/**
  * @brief  Manage the CDC class requests
  * @param  cmd: Command code
  * @param  pbuf: Buffer containing command data (request parameters)
  * @param  length: Number of data to be sent (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length)
{
    switch(cmd)
    {
        case CDC_SEND_ENCAPSULATED_COMMAND:    break;
        case CDC_GET_ENCAPSULATED_RESPONSE:    break;
        case CDC_SET_COMM_FEATURE:             break;
        case CDC_GET_COMM_FEATURE:             break;
        case CDC_CLEAR_COMM_FEATURE:           break;

    /*******************************************************************************/
    /* Line Coding Structure                                                       */
    /*-----------------------------------------------------------------------------*/
    /* Offset | Field       | Size | Value  | Description                          */
    /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
    /* 4      | bCharFormat |   1  | Number | Stop bits                            */
    /*                                        0 - 1 Stop bit                       */
    /*                                        1 - 1.5 Stop bits                    */
    /*                                        2 - 2 Stop bits                      */
    /* 5      | bParityType |  1   | Number | Parity                               */
    /*                                        0 - None                             */
    /*                                        1 - Odd                              */
    /*                                        2 - Even                             */
    /*                                        3 - Mark                             */
    /*                                        4 - Space                            */
    /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
    /*******************************************************************************/
    case CDC_SET_LINE_CODING:
        LineCoding.bitrate = (uint32_t)(pbuf[0] | (pbuf[1] << 8) |
                (pbuf[2] << 16) | (pbuf[3] << 24));
        LineCoding.format = pbuf[4];
        LineCoding.paritytype = pbuf[5];
        LineCoding.datatype = pbuf[6];
    break;

    case CDC_GET_LINE_CODING:
        pbuf[0] = (uint8_t)(LineCoding.bitrate);
        pbuf[1] = (uint8_t)(LineCoding.bitrate >> 8);
        pbuf[2] = (uint8_t)(LineCoding.bitrate >> 16);
        pbuf[3] = (uint8_t)(LineCoding.bitrate >> 24);
        pbuf[4] = LineCoding.format;
        pbuf[5] = LineCoding.paritytype;
        pbuf[6] = LineCoding.datatype;
    break;

    case CDC_SET_CONTROL_LINE_STATE:
        if ((((USBD_SetupReqTypedef *) pbuf)->wValue & 0x0001) == 0)
        {
            usb_cdc_if.isComPortOpen = closed;
        }
        else
        {
            usb_cdc_if.isComPortOpen = preOpen;
            usb_cdc_if.portOpenTime = HAL_GetTick();
        }
    break;

    case CDC_SEND_BREAK:    break;
    default:                break;
  }

  return (USBD_OK);
}

/**
  * @brief  Data received over USB OUT endpoint are sent over CDC interface
  *         through this function.
  *
  *         @note
  *         This function will issue a NAK packet on any OUT packet received on
  *         USB endpoint until exiting this function. If you exit this function
  *         before transfer is complete on CDC interface (ie. using DMA controller)
  *         it will result in receiving more data while previous ones are still
  *         not sent.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_Receive_FS(uint8_t* Buf, uint32_t *Len)
{
    /* Only way subsequent USBD_CDC functions can fail is if this parameter is NULL */
    assert(hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId] != NULL);

    uint16_t len = (uint16_t)*Len;

    // Update circular buffer with incoming values
    for(uint8_t i = 0; i < len; i++) {
        circular_buf_put(usb_cdc_if.rx.ctx, Buf[i]);
    }

    memset(Buf, '\0', len); // clear the buffer

    /* Prepare for a new reception */
    USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &Buf[0]);
    USBD_CDC_ReceivePacket(&hUsbDeviceFS);

    return (USBD_OK);
}

/**
  * @brief  CDC_TransmitCplt_FS
  *         Data transmited callback
  *
  *         @note
  *         This function is IN transfer complete callback used to inform user that
  *         the submitted Data is successfully sent over USB.
  *
  * @param  Buf: Buffer of data to be received
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CDC_TransmitCplt_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
    /* Only way subsequent USBD_CDC functions can fail is if this parameter is NULL */
    assert(hUsbDeviceFS.pClassDataCmsit[hUsbDeviceFS.classId] != NULL);

    uint8_t result = USBD_OK;

    // Fill in the next number of bytes.
    uint16_t len = 0;
    for (uint8_t *ptr = usb_cdc_if.tx.irqBuf; ptr < &usb_cdc_if.tx.irqBuf[sizeof(usb_cdc_if.tx.irqBuf)]; ptr++)
    {
        if (circular_buf_get(usb_cdc_if.tx.ctx, ptr) != 0) {
            break; // No more bytes ready to transmit.
        }
        len++; // Yet another byte to send.
    }

    if (len != 0)
    {
        USBD_CDC_SetTxBuffer(&hUsbDeviceFS, usb_cdc_if.tx.irqBuf, len);
        result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
    }

    if(result != USBD_OK) {
        usb_error |= CDC_ERROR_DELAYED_TRANSMIT;
    }
    else {
        usb_error &= ~(CDC_ERROR_DELAYED_TRANSMIT | CDC_ERROR_TRANSMIT);
    }

    return result;
}
