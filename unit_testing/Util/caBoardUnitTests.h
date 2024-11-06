/*!
** @file   caBoardsUnitTests.h
** @author Luke W
** @date   09/08/2024
*/

#ifndef CA_BOARD_UNIT_TESTS_H_
#define CA_BOARD_UNIT_TESTS_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

#include "fake_stm32xxxx_hal.h"
#include "systemInfo.h"
#include "HAL_otp.h"
#include "fake_USBprint.h"

/***************************************************************************************************
** TYPEDEFS
***************************************************************************************************/

typedef void (*loopFunction_t)(const char *);

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class CaBoardUnitTest: public ::testing::Test
{
    public:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        CaBoardUnitTest(loopFunction_t loopFunction, BoardType board, pcbVersion pcbVer) {
            _loopFunction = loopFunction;
            bi.v2.boardType = board;
            bi.v2.pcbVersion = {
                pcbVer.major,
                pcbVer.minor
            };

            /* OTP code to allow initialisation of the board to pass */
            HAL_otpWrite(&bi);
            forceTick(0);
            hostUSBConnect();
        }

        virtual void writeBoardMessage(const char * msg) {
            hostUSBprintf(msg);
            _loopFunction(bootMsg);
        }

        /*!
        ** @brief Function to simulate a single tick
        **
        ** Child class must provide this function that simulates what happens in a single tick. The
        ** protected member "tickCounter" can be used to access the current tick; this can be useful
        ** for events that occur only once every X ticks
        */
        virtual void simTick() = 0;

        virtual void simTicks(int numTicks = 1) {
            int tickDest = tickCounter + numTicks;
            while(tickCounter < tickDest) {
                forceTick(++tickCounter);
                simTick();
            }
        }

        virtual void goToTick(int tickDest) {
            if(tickDest > tickCounter) {
                simTicks(tickDest - tickCounter);
            }
            else if (tickDest < tickCounter) {
                tickCounter = tickDest;
                forceTick(tickCounter);
                simTick();
            }
        }

        /*******************************************************************************************
        ** MEMBERS
        *******************************************************************************************/
        
        const char * bootMsg = "Boot Unit Test";
        loopFunction_t _loopFunction;
        int tickCounter = 0;

        BoardInfo bi = {
            .v2 = {
                .otpVersion = OTP_VERSION_2,
                .boardType  = 0,
                .subBoardType = 0,
                .reserved = {0},
                .pcbVersion = {0},
                .productionDate = 0
            }
        };
};

#endif /* CA_BOARD_UNIT_TESTS_H_ */