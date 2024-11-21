/*!
** @file   serialStatus_tests.h
** @author Luke W
** @date   09/08/2024
*/

#ifndef SERIAL_STATUS_TESTS_H_
#define SERIAL_STATUS_TESTS_H_

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

/* These tests are implemented as functions. They have to be called as follows from the main test 
** file:
**   TEST_F(TestFixture, TestName) {
**       goldenPathTest(x, y, z);
**   }
*/

void goldenPathTest(SerialStatusTest& sst, const char* pass_string, int firstPrintTick=100);
void incorrectBoardTest(SerialStatusTest& sst, int firstPrintTick=100);
void statusPrintoutTest(SerialStatusTest& sst, std::vector<const char*> pass_string);
void statusDefPrintoutTest(SerialStatusTest& sst, std::vector<const char*> pass_string);
void serialPrintoutTest(SerialStatusTest& sst, const char* boardName, const char* cal_string=nullptr);

#endif /* SERIAL_STATUS_TESTS_H_ */