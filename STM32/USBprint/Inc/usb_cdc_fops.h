/*
 * This file should replace the implementation in USB_DEVICE/App/usbd_cdc_if.
 * The remaining handling USB initialization should be re-used (for now).
 * What must be done in USB_DEVICE/App/usb_device.c is to overwrite the
 * USBD_Interface_fops_FS with our own reader/write functions. Add this
 * include file to USB_DEVICE/App/usb_device.c and add this line:
 *
 * USBD_Interface_fops_FS = usb_cdc_fops;
 *
 * in function void MX_USB_DEVICE_Init(void)
 *
 * The reason to do overwrite is that the usb cdc interface needs changes.
 * Theses changes is required for _ALL_ boards since USB_DEVICE resides
 * in the project directory and not as a library, so to prevent copy/paste
 * this new usb_cdc_fops module has been introduced.
 */
#ifndef USB_CDC_FOPS_H
#define USB_CDC_FOPS_H

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#include "usbd_cdc.h"

#ifdef __cplusplus
    extern "C" {
#endif

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define CIRCULAR_BUFFER_SIZE 1024

#define CDC_ERROR_NONE              0x00000000U
#define CDC_ERROR_DELAYED_TRANSMIT  0x00000001U
#define CDC_ERROR_TRANSMIT          0x00000002U
#define CDC_ERROR_CROPPED_TRANSMIT  0x00000004U

/***************************************************************************************************
** PUBLIC OBJECT DECLARATION
***************************************************************************************************/

extern USBD_CDC_ItfTypeDef usb_cdc_fops;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

ssize_t usb_cdc_transmit(const uint8_t* Buf, uint16_t len);
size_t usb_cdc_tx_available();
int usb_cdc_rx(uint8_t* buf);
void usb_cdc_rx_flush();
bool isComPortOpen();
uint32_t isCdcError();

#ifdef __cplusplus
}
#endif

#endif
