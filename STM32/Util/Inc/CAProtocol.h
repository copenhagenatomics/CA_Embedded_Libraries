/*
 * CAProtocol.h
 *
 *  Created on: Oct 6, 2021
 *      Author: agp
 */

#ifndef INC_CAPROTOCOL_H_
#define INC_CAPROTOCOL_H_

#include <stdbool.h>
#include "HAL_otp.h"

/***************************************************************************************************
** STRUCTURES
***************************************************************************************************/

typedef struct
{
    int port;
    double alpha;
    double beta;
    int threshold;
} CACalibration;

typedef int (*ReaderFn)(uint8_t* rxBuf);
typedef struct
{
    // Called if message is not found. Overwrite to get info about invalid input
    void (*undefined)(const char* inputString);

    // system info request using "Serial". These should be overwritten
    void (*printHeader)();
    // system info request using "Status". These should be overwritten with additional 
    // board specific status codes.
    void (*printStatus)();
    // system info request using "StatusDef". These should be overwritten with additional 
    // board specific status codes.
    void (*printStatusDef)();
    void (*jumpToBootLoader)();

    // Calibration request.
    void (*calibration)(int noOfCalibrations, const CACalibration* calibrations);
    void (*calibrationRW)(bool write);     // Read or write calibration values to flash

    // Data logger request on port (range [1:N]), Zero means stop.
    void (*logging)(int port);

    // OTP read/write. Don't enable write of OTP data in application shipped
    // to customer. The OTP write is a production operation only!
    void (*otpRead)();
    void (*otpWrite)(BoardInfo *boardInfo);

    struct CAProtocolData *data; // Private data for CAProtocol.
} CAProtocolCtx;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void inputCAProtocol(CAProtocolCtx* ctx);
void initCAProtocol(CAProtocolCtx* ctx, ReaderFn fn);
void flushCAProtocol(CAProtocolCtx* ctx);

#endif /* INC_CAPROTOCOL_H_ */
