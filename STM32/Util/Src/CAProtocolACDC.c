/**
 ******************************************************************************
 * @file    CAProtocolACDC.c
 * @brief   This file contains the input handler for AC and DC boards
 * @date:   31 Jan 2025
 * @author: Matias
 ******************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CAProtocolACDC.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#define MAX_CMD_CHARS 13

/***************************************************************************************************
** PRIVATE FUNCTION DECLARATIONS
***************************************************************************************************/

static int getArgs(char *input, char delim, char **argv, int max_len);

/***************************************************************************************************
** PRIVATE OBJECT DECLARATIONS
***************************************************************************************************/

/* Buffer for preventing modifications to original message. 2x MAX_CMD_CHARS to allow for sensible 
** number of additional spaces, trailiing newline, etc...*/
char buf[2 * MAX_CMD_CHARS] = {0}; 

/***************************************************************************************************
** PRIVATE FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Extracts arguments from a given input string
**
** @param[in]  input   Input string
** @param[in]  delim   Delimiter used to separate arguments
** @param[out] argv    Pointer to a list of arguments
** @param[in]  max_len Maximum number of arguments that can be stored in args
*/
static int getArgs(char *input, char delim, char **argv, int max_len) {
    int len           = strlen(input);
    bool search_delim = false;
    int k             = 0;

    for (int i = 0; i < len; i++) {
        if (!search_delim && input[i] != delim) {
            argv[k++]    = input + i;
            search_delim = true;
        }
        else if (search_delim && input[i] == delim) {
            input[i]     = '\0';
            search_delim = false;
        }

        if (k >= max_len) {
            break;  // Avoid buffer overflow.
        }
    }

    return k;
}

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
** @brief Common input handler for AC and DC boards
*/
void ACDCInputHandler(ACDCProtocolCtx *ctx, const char *input) {
    int duration;
    if (sscanf(input, "all on %d", &duration) == 1) {
        if (ctx->allOn) {
            ctx->allOn(true, duration);
        }
        else {
            HALundefined(input);
        }
    }
    else if (strncmp(input, "all off", 7) == 0) {
        if (ctx->allOn) {
            ctx->allOn(false, -1);
        }
        else {
            HALundefined(input);
        }
    }
    else if (input[0] == 'p' &&
             strnlen(input, 14) <= 14)  // 14 since that is length of pXX on YY ZZZ%
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
        else if (!ctx->portState) {
            HALundefined(input);
        }
        else if (strncmp(cmd, "off", 3) == 0) {
            ctx->portState(port, false, 0, -1);
        }
        else if (strncmp(cmd, "on", 2) == 0) {
            /* getArgs modifies the message, so retain the original for debug printout in case of 
            ** error */
            strncpy(buf, input, sizeof(buf) - 1);
            buf[sizeof(buf) - 1] = '\0';  // Ensure some null termination

            char *argv[4] = {0};  // There should not be more then 4 args.
            int count = getArgs(buf, ' ', argv, 4);
            char percent = 0;

            switch (count) {
                case 2: {  // pX on
                    ctx->portState(port, true, 100, -1);
                    break;
                }
                case 3:  // pX on ZZZ% or YY
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
                case 4:  // pX on YY ZZZ%
                {
                    int tmp;
                    if (sscanf(argv[2], "%d", &tmp) == 1 &&
                        sscanf(argv[3], "%d%c", &count, &percent) == 2 && percent == '%') {
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
    else {
        HALundefined(input);
    }
}