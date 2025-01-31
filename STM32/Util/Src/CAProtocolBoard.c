/** 
 ******************************************************************************
 * @file    CAProtocolBoard.c
 * @brief   This file contains board specific input handlers
 * @date:   31 Jan 2025
 * @author: Matias
 ******************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>  

#include "CAProtocolBoard.h"

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef struct CAProtocolData {
    size_t len;         // Length of current data.
    uint8_t buf[512];   // Buffer for the string fetched from the circular buffer.
    ReaderFn rxReader;  // Reader for the buffer
} CAProtocolData;

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Common input handler for AC and DC boards
*/
void ACDCBoardHandler(CAProtocolCtx* ctx) {

    // A message is received i.e. a zero terminated string
    char* input = (char *)ctx->data->buf;

    if (strncmp(input, "all on", 6) == 0) {
        /* There could be an extra argument after "on" (required for AC board, optional for DC board
        ** at time of writing) */
        char *argv[3] = { 0 }; // There should not be more then 3 args.
        int count = getArgs(input, ' ', argv, 3);

        if (count == 3)
        {
            (void) sscanf(argv[2], "%d", &count);
            ctx->allOn(true, count);
        }
        else
        {
            if (ctx->allOn) 
            {
                ctx->allOn(true, -1);
            }
        }
    }
    else if (strncmp(input, "all off", 7) == 0)
    {
        if (ctx->allOn) {
            ctx->allOn(false, -1);
        }
    }
    else if (input[0] == 'p' && strnlen(input, 14) <= 14) // 14 since that is length of pXX on YY ZZZ%
    {
        char cmd[13];
        int port;
        /* Valid commands are:
           all on - turn all ports on indefinitely
           all off - turn all ports off
           pX on - turn off port number X
           pX off - turn on port number X indefinitely 'always on'
           pX on YY - turn on port number X for YY seconds
           pX on ZZZ% - turn on port number X on ZZ percent of the time using PWM 'always on'
           pX on YY ZZZ% - turn on port number X for YY seconds ZZ percent of the time using PWM */
        if (sscanf(input, "p%d %[onf]", &port, cmd) != 2) {
            HALundefined(input);
        }
        if (!ctx->portState) {
            HALundefined(input);
        }
        if (strncmp(cmd, "off", 3) == 0) {
            ctx->portState(port, false, 0, -1);
        }
        else if (strncmp(cmd, "on", 2) == 0)
        {
            char *argv[4] = { 0 }; // There should not be more then 4 args.
            int count = getArgs(input, ' ', argv, 4);
            char percent = 0;

            switch(count)
            {
                case 2: { // pX on
                    ctx->portState(port, true, 100, -1);
                    break;
                }
                case 3: // pX on ZZZ% or YY
                {
                    int argc = sscanf(argv[2], "%d%c", &count, &percent);
                    if (argc == 2 && percent == '%') {
                        ctx->portState(port, true, count, -1);
                    }
                    else if (argc == 1) {
                        ctx->portState(port, true, 100, count);
                    }
                    else {
                        HALundefined(input);
                    }
                    break;
                }
                case 4: // pX on YY ZZZ%
                {
                    int tmp;
                    if (sscanf(argv[2], "%d", &tmp) == 1 && sscanf(argv[3], "%d%c", &count, &percent) == 2 && percent == '%') {
                        ctx->portState(port, true, count, tmp);
                    }
                    else {
                        HALundefined(input);
                    }
                    break;
                }
            }
        }
    }
    else
    {
        HALundefined(input);
    }
}