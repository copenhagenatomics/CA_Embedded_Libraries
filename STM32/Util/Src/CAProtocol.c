/*
 * CAProtocol.c
 *
 *  Created on: Oct 6, 2021
 *      Author: agp
 */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAProtocol.h"
#include "CAProtocolStm.h"

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

#define MAX_NO_CALIBRATION 12

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef struct CAProtocolData {
    size_t len;         // Length of current data.
    uint8_t buf[512];   // Buffer for the string fetched from the circular buffer.
    ReaderFn rxReader;  // Reader for the buffer
} CAProtocolData;

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static void calibration(CAProtocolCtx* ctx, const char* input);
static void logging(CAProtocolCtx* ctx, const char* input);
static int CAgetMsg(CAProtocolCtx* ctx);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

static void calibration(CAProtocolCtx* ctx, const char* input) {
    char* idx = strchr((char*)input, ' ');
    CACalibration cal[MAX_NO_CALIBRATION];
    int noOfCalibrations = 0;

    // Incorrect format
    if (!idx) {
        ctx->undefined(input);
        return;
    }

    // Single-letter subcommands
    if (idx[2] == '\0') {
        void (*fn)(void) = NULL;

        // Prints calibration stored in RAM
        if (idx[1] == 'r') {
            fn = ctx->readCalibrationFromRAM;
        }
        // Prints calibration stored in flash
        else if (idx[1] == 'm') {
            fn = ctx->readCalibrationFromFlash;
        }
        // Write calibration stored in RAM into flash
        else if (idx[1] == 'w') {
            fn = ctx->writeCalibrationToFlash;
        }

        if (fn) {
            fn();
        }
        else {
            ctx->undefined(input);
        }
        return;
    }

    // Going through each calibration
    while (idx != NULL && noOfCalibrations < MAX_NO_CALIBRATION) {
        int port, noOfValues;
        double alpha, beta, gamma;
        idx++;

        // Beta and gamma are optional and therefore only 2 values need to be set.
        // For any board that uses beta/gamma value, noOfValues needs to be checked
        noOfValues = sscanf(idx, "%d,%lf,%lf,%lf", &port, &alpha, &beta, &gamma);
        if (noOfValues >= 2) {
            cal[noOfCalibrations] = (CACalibration){port, alpha, beta, gamma, noOfValues};
            noOfCalibrations++;
        }

        // Get the next space.
        idx = strchr(idx, ' ');
    }

    if (noOfCalibrations != 0) {
        ctx->calibration(noOfCalibrations, cal);
    }
    else {
        ctx->undefined(input);
    }
}

static void logging(CAProtocolCtx* ctx, const char* input) {
    char* idx = strchr((char*)input, ' ');
    int port;

    if (!idx) {
        ctx->undefined(input);  // arguments.
        return;
    }

    idx++;
    if (sscanf(idx, "p%d", &port) == 1) {
        ctx->logging(port);
    }
    else {
        ctx->undefined(input);
    }
}

#if (OTP_VERSION != OTP_VERSION_2)
#error "Update of CAProtocol required"
#endif

static int CAgetMsg(CAProtocolCtx* ctx) {
    CAProtocolData* protocolData = ctx->data;
    int msgLen                   = 0;

    while (msgLen == 0) {
        uint8_t rxByte;

        if (protocolData->rxReader(&rxByte)) {
            return 0;  // No more data in buffer => no message.
        }

        protocolData->buf[protocolData->len] = rxByte;
        if (rxByte == '\r' || rxByte == '\n') {
            if (protocolData->len == 0) {
                continue;  // Ignore this zero length message.
            }

            // A valid string is received.
            msgLen = protocolData->len;
            break;
        }

        protocolData->len++;  // Continue receive from buffer.
        if (protocolData->len == (sizeof(protocolData->buf) - 1)) {
            // Buffer overflow!! This is a protocol error or garbage from peer.
            // No string should be bigger then protocolData buffer size.
            // Simulate a string since there might be something of value.
            msgLen = protocolData->len;
            break;
        }
    }

    if (msgLen != 0) {
        // enforce zero terminate of string.
        protocolData->buf[protocolData->len] = 0;
        // Reset for receive of a new message. Caller must handle message or save it locally.
        protocolData->len = 0;
    }

    return msgLen;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

void inputCAProtocol(CAProtocolCtx* ctx) {
    int parseError = 1;

    int msgLen = CAgetMsg(ctx);
    if (msgLen == 0) {
        return;  // No message received
    }

    // A message is received i.e. a zero terminated string
    char* input = (char*)ctx->data->buf;

    if (strncmp(input, "Serial", 6) == 0) {
        if (ctx->printHeader) {
            ctx->printHeader();
            parseError = 0;
        }
    }
    else if (strncmp(input, "OutputDef", 9) == 0) {
        CAPrintOutputDef(true);
        if (ctx->printOutputDef) {
            ctx->printOutputDef();  // Print board specific part of statusdefinition message
        }
        CAPrintOutputDef(false);  // Print end of status definition message
        parseError = 0;
    }
    else if (strncmp(input, "StatusDef", 9) == 0) {
        CAPrintStatusDef(true);  // Print start of status definition message
        if (ctx->printStatusDef) {
            ctx->printStatusDef();  // Print board specific part of statusdefinition message
        }
        CAPrintStatusDef(false);  // Print end of status definition message
        parseError = 0;
    }
    else if (strncmp(input, "Status", 6) == 0) {
        CAPrintStatus(true);  // Print start of status message
        if (ctx->printStatus) {
            ctx->printStatus();  // Print board specific part of status message
        }
        CAPrintStatus(false);  // Print end of status message
        parseError = 0;
    }
    else if (strncmp(input, "DFU", 3) == 0) {
        if (ctx->jumpToBootLoader) {
            ctx->jumpToBootLoader();
            parseError = 0;
        }
    }
    else if (strncmp(input, "CAL", 3) == 0) {
        if (ctx->calibration) {
            calibration(ctx, input);
            parseError = 0;
        }
    }
    else if (strncmp(input, "LOG", 3) == 0) {
        if (ctx->logging) {
            logging(ctx, input);
            parseError = 0;
        }
    }
    else if (strncmp(input, "OTP r", 5) == 0) {
        if (ctx->otpRead) {
            ctx->otpRead();
            parseError = 0;
        }
    }
    else if (strncmp(input, "uptime", 6) == 0) {
        if (ctx->uptime) {
            ctx->uptime(input);
            parseError = 0;
        }
    }
    else if (ctx->undefined) {
        ctx->undefined(input);
        parseError = 0;
    }

    if (parseError) {
        HALundefined(input);
    }
}

void initCAProtocol(CAProtocolCtx* ctx, ReaderFn fn) {
    ctx->data = (CAProtocolData*)malloc(sizeof(CAProtocolData));
    memset(ctx->data->buf, 0, sizeof(ctx->data->buf));
    ctx->data->len      = 0;
    ctx->data->rxReader = fn;
}

void flushCAProtocol(CAProtocolCtx* ctx) {
    ctx->data->len = 0;
    memset(ctx->data->buf, 0, sizeof(ctx->data->buf));
}
