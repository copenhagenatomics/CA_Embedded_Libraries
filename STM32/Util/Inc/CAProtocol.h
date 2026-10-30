/*
 * CAProtocol.h
 *
 *  Created on: Oct 6, 2021
 *      Author: agp
 */

#ifndef INC_CAPROTOCOL_H_
#define INC_CAPROTOCOL_H_

#include <stdint.h>

/***************************************************************************************************
** STRUCTURES
***************************************************************************************************/

typedef struct {
    int port;
    double alpha;
    double beta;
    double gamma;
    int noOfValues;
} CACalibration;

typedef int (*ReaderFn)(uint8_t* rxBuf);

typedef struct {
    // Called if message is not found. Overwrite to get info about invalid input
    void (*undefined)(const char* inputString);

    // System info request using "Serial". These should be overwritten
    void (*printHeader)();

    // Status request using "Status". These should be overwritten with additional
    // board specific status codes.
    void (*printStatus)();

    // Status definition request using "StatusDef". These should be overwritten with additional
    // board specific status codes.
    void (*printStatusDef)();

    // Output info request using "OutputDef". These should be overwritten with additional channel
    // info.
    void (*printOutputDef)();

    // Bootloader request using "DFU".
    void (*jumpToBootLoader)();

    // Calibration request.
    void (*calibration)(int noOfCalibrations, const CACalibration* calibrations);

    // Prints calibration stored in volatile memory ("CAL r" command)
    void (*readCalibrationFromRAM)();

    // Prints calibration stored in non-volatile memory ("CAL m" command)
    void (*readCalibrationFromFlash)();

    // Write the current volatile calibration into non-volatile memory ("CAL w" command)
    void (*writeCalibrationToFlash)();

    // Data logger request on port (range [1:N]), Zero means stop.
    void (*logging)(int port);

    // OTP read
    void (*otpRead)();

    // Print generic uptime information
    void (*uptime)(const char* inputString);

    struct CAProtocolData* data;  // Private data for CAProtocol.
} CAProtocolCtx;

/***************************************************************************************************
** PUBLIC FUNCTION DECLARATIONS
***************************************************************************************************/

void inputCAProtocol(CAProtocolCtx* ctx);
void initCAProtocol(CAProtocolCtx* ctx, ReaderFn fn);
void flushCAProtocol(CAProtocolCtx* ctx);

#endif /* INC_CAPROTOCOL_H_ */
