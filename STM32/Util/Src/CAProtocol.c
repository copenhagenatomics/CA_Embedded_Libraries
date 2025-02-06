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
static void otp_write(CAProtocolCtx* ctx, const char* input);
static int CAgetMsg(CAProtocolCtx* ctx);

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

static void calibration(CAProtocolCtx* ctx, const char* input) {
    char* idx = strchr((char*)input, ' ');
    CACalibration cal[MAX_NO_CALIBRATION];
    int noOfCalibrations = 0;

    if (!idx) {
        ctx->undefined(input);  // arguments.
        return;
    }
    if (idx[1] == 'w' || idx[1] == 'r') {
        if (ctx->calibrationRW) {
            ctx->calibrationRW(idx[1] == 'w');
        }
        return;
    }

    // Next follow a port,alpha,beta entry
    while (idx != NULL && noOfCalibrations < MAX_NO_CALIBRATION) {
        int port;
        double alpha, beta;
        int threshold;
        idx++;
        // Threshold is optional and therefore only 3 values need to be set.
        // For any board that uses the threshold value an additional check should
        // be made before using for any calibration.
        if (sscanf(idx, "%d,%lf,%lf,%d", &port, &alpha, &beta, &threshold) >= 3) {
            cal[noOfCalibrations] = (CACalibration){port, alpha, beta, threshold};
            noOfCalibrations++;
        }
        idx = strchr(idx, ' ');  // get the next space.
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
static void otp_write(CAProtocolCtx* ctx, const char* input) {
    uint32_t OTPVersion;
    uint32_t BoardType;
    uint32_t SubBoardType;
    uint32_t PCBversion[2];
    uint32_t date;

    // Check for supported version.
    if (sscanf(input, "OTP w %02" PRIu32, &OTPVersion) != 1) {
        return;  // Parse failure, Invalid format of version field.
    }
    if (OTPVersion > OTP_VERSION || OTPVersion == 0) {
        return;  // Not supported version of the OTP data.
    }

    BoardInfo info;
    memset(info.data, 0, sizeof(info.data));

    switch (OTPVersion) {
        case OTP_VERSION_1:
            break;

        case OTP_VERSION_2:
            // During write only the current version is supported.
            if (sscanf(input,
                       "OTP w %02" PRIu32 " %02" PRIu32 " %02" PRIu32 " %02" PRIu32 ".%02" PRIu32
                       " %" PRIu32,
                       &OTPVersion, &BoardType, &SubBoardType, &PCBversion[1], &PCBversion[0],
                       &date) == 6) {
                if (BoardType < 0xFF && PCBversion[1] <= 0xFF && PCBversion[0] <= 0xFF) {
                    info.v2.otpVersion = OTP_VERSION_2;
                    info.v2.boardType = BoardType & 0xFF;
                    info.v2.subBoardType = SubBoardType & 0xFF;
                    info.v2.pcbVersion.major = PCBversion[1] & 0xFF;
                    info.v2.pcbVersion.minor = PCBversion[0] & 0xFF;
                    info.v2.productionDate = date;
                    ctx->otpWrite(&info);
                    return;
                }
            }
            break;

        default:
            break;
    }

    // Invalid input format.
    ctx->undefined(input);
}

static int CAgetMsg(CAProtocolCtx* ctx) {
    CAProtocolData* protocolData = ctx->data;
    int msgLen = 0;

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
    else if (strncmp(input, "OTP", 3) == 0) {
        if (input[4] == 'r') {
            if (ctx->otpRead) {
                ctx->otpRead();
                parseError = 0;
            }
        }
        else if (input[4] == 'w') {
            if (ctx->otpWrite) {
                otp_write(ctx, input);
                parseError = 0;
            }
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
    ctx->data->len = 0;
    ctx->data->rxReader = fn;
}

void flushCAProtocol(CAProtocolCtx* ctx) {
    ctx->data->len = 0;
    memset(ctx->data->buf, 0, sizeof(ctx->data->buf));
}
