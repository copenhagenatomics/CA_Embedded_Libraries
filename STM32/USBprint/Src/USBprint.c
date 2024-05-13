/*
 * USBprint.c
 * provides a function similar to the standard printf with capability to print data over USB
 *  Created on: Apr 26, 2021
 *      Author: Alexander.mizrahi@copenhagenatomics.com
 */


#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "USBprint.h"
#include "usb_cdc_fops.h"

int USBnprintf(const char * format, ... )
{
    static char buffer[256];
    int len = snprintf(buffer, 3, "\r\n");

    va_list args;
    va_start (args, format);
    len += vsnprintf(&buffer[len], sizeof(buffer) - len, format, args);
    va_end (args);

    /* Error code captured in lower level module */
    ssize_t ret = usb_cdc_transmit((const uint8_t*)buffer, len);

    /* Since this function adds 2 characters to the transmit, remove those from the return value. 
    ** It is most likely a user will want to compare the length sent with the length of the input 
    ** string. */
    if(ret >= 2) {
        return ret - 2;
    }
    /* This situation should probably never happen, since this function always adds 2, but there 
    ** could be an issue in a lower level module */
    else if(ret >= 0) {
        return 0;
    }
    /* Any value less than 0 is an error, so pass that up */
    else {
        return ret;
    }
}

ssize_t writeUSB(const void *buf, size_t count)
{
    /* Error code captured in lower level module */
    return usb_cdc_transmit((const uint8_t*)buf, count);
}

size_t txAvailable()
{
    return usb_cdc_tx_available();
}

/*!
** @brief Returns whether the port is active or not
*/
bool isUsbPortOpen()
{
    return isComPortOpen();
}

/*!
** @brief Function to get data from the USB receive buffer
*/
int usbRx(uint8_t* buf)
{
    return usb_cdc_rx(buf);
}

/*!
** @brief Function to flush USB buffers
*/
void usbFlush()
{
    usb_cdc_rx_flush();
}

/*!
** @brief Returns if there has been an error in the USB stack
*/
uint32_t isUsbError() {
    return isCdcError();
}