/*!
** @file   caBoardsUnitTests.h
** @author Luke W
** @date   09/08/2024
*/

#pragma once

#include <string>
#include <functional>
#include <vector>

#include "HAL_otp.h"
#include "caBoardUnitTests.h"

/***************************************************************************************************
** CLASSES
***************************************************************************************************/

class SerialStatusTest {
    public:
        /*******************************************************************************************
        ** MEMBERS
        *******************************************************************************************/
        std::function<void()> boundInit;
        CaBoardUnitTest* testFixture;
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

/* These tests are implemented as functions. They have to be called as follows:
** * TEST_F(TestFixture, TestName) {}
*/

void goldenPathTest(SerialStatusTest& sst, const char* pass_string);
void incorrectBoardTest(SerialStatusTest& sst);
void statusPrintoutTest(SerialStatusTest& sst, std::vector<const char*> pass_string);
void serialPrintoutTest(SerialStatusTest& sst, const char* boardName, const char* cal_string=nullptr);

