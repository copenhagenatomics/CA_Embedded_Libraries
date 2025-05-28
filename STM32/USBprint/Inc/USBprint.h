/*
 * USBprint.h
 *
 *  Created on: Apr 26, 2021
 *      Author: alema
 */

#ifndef INC_USBPRINT_H_
#define INC_USBPRINT_H_

#include <unistd.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief Adds a formatted string at the end of a buffer
 * @param b Buffer
 * @param l Length already written (is updated)
 * @param ... Formatted string
 */
#define CA_SNPRINTF(b, l, ...) l += snprintf(&b[l], sizeof(b) - l, __VA_ARGS__)

// Wrap vsnprintf(char *str, size_t size, const char *format, va_list ap)
// and send data to USB port. The n indicates that buffer overflow is handled.
// String/n is max 256 bytes.
int USBnprintf(const char * format, ... );

// Same interface ansi C write, same return values.
ssize_t writeUSB(const void *buf, size_t count);

// Return the number of bytes possible to write to buffer.
size_t txAvailable();

bool isUsbPortOpen();

int usbRx(uint8_t* buf);

void usbFlush();

uint32_t isUsbError();

#endif /* INC_USBPRINT_H_ */
