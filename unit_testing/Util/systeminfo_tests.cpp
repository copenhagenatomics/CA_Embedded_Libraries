/*!
** @file   systeminfo_tests.cpp
** @author Luke W
** @date   05/11/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

/* Fakes */

/* Real supporting units */

/* UUT */
#include "systeminfo.c"

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class TestSystemInfo: public ::testing::Test
{
    public: 
        /*******************************************************************************************
        ** PUBLIC METHODS
        *******************************************************************************************/
        TestSystemInfo() {}

    private:
        /*******************************************************************************************
        ** PRIVATE METHODS
        *******************************************************************************************/
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(TestSystemInfo, testBoardVersion) {
    /* Setup fake board info with correct version number */
    BoardInfo bi = {
        .v2 = {
            .otpVersion = OTP_VERSION_2,
            .boardType = AC_Board,
            .pcbVersion = {1, 1},
        }
    };
    HAL_otpWrite(&bi);

    /* Different test cases smaller, equal and larger for both major/minor */
    EXPECT_EQ( 0, boardSetup(AC_Board, {0, 0}));
    /* Error flag is retained between tests, so make sure to clear it */
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ( 0, boardSetup(AC_Board, {0, 1}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ( 0, boardSetup(AC_Board, {0, 2}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ( 0, boardSetup(AC_Board, {1, 0}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ( 0, boardSetup(AC_Board, {1, 1}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ(-1, boardSetup(AC_Board, {1, 2}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ(-1, boardSetup(AC_Board, {2, 0}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ(-1, boardSetup(AC_Board, {2, 1}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ(-1, boardSetup(AC_Board, {2, 2}));
    bsClearField(0xFFFFFFFF);
    EXPECT_EQ(-1, boardSetup(DC_Board, {1, 1}));
}
