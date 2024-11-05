/*!
** @file   adcmonitor_tests.cpp
** @author Matias
** @date   17/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

#include <stdio.h>
#include <string.h>
#include <queue>

/* Fakes */

/* Real supporting units */

/* UUT */
#include "CAProtocol.c"

using ::testing::AnyOf;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using namespace std;

/***************************************************************************************************
** HELPER CLASS / FUNCTIONS
***************************************************************************************************/

class PortCfg
{
    public:
        PortCfg(bool _state, int _duration, int _percent) : state(_state), duration(_duration), percent(_percent) {};
        PortCfg() : state(false), duration(-1), percent(-1) {};
        inline bool operator==(const PortCfg& rhs)
        {
            return (state == rhs.state && duration == rhs.duration && percent == rhs.percent);
        }

        bool state;
        int duration;
        int percent;
};

static struct {
    CACalibration cal[10];
    int noOfCalibration;
} calData;
void CACalibrationCb(int noOfPorts, const CACalibration *catAr) {
    calData.noOfCalibration = noOfPorts;
    memcpy(calData.cal, catAr, sizeof(calData.cal));
}
int calCompare(int noOfPorts, const CACalibration* catAr)
{
    if (noOfPorts != calData.noOfCalibration)
        return 1;
    for (int i=0; i<noOfPorts; i++)
    {
        if (calData.cal[i].port  != catAr[i]. port ||
            calData.cal[i].alpha != catAr[i].alpha ||
            calData.cal[i].beta  != catAr[i]. beta)
        { return 2; }
    }

    return 0; // All good
}

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class TestCAProtocol: public ::testing::Test
{
    public: 
        /*******************************************************************************************
        ** PUBLIC METHODS
        *******************************************************************************************/
        TestCAProtocol()
        {
            caProto.calibration = CACalibrationCb;
            caProto.allOn = TestCAProtocol::allOn;
            caProto.portState = TestCAProtocol::portState;
            caProto.undefined = TestCAProtocol::undefined;
            initCAProtocol(&caProto, testReader);
            reset();
        }

        void reset() {
            while(!testString.empty())
                testString.pop();
            portCtrl.allOn = 0;
            portCtrl.undefCall = 0;
        }

        int testCalibration(const char* input, int noOfPorts, const CACalibration calAray[])
        {
            while(*input != 0) {
                testString.push(*input);
                input++;
            }
            inputCAProtocol(&caProto);
            return calCompare(noOfPorts, calAray);
        }

        int testPortCtrl(const char* input, int port, const PortCfg& cfg)
        {
            while(*input != 0) {
                testString.push(*input);
                input++;
            }
            inputCAProtocol(&caProto);
            return portCtrl.port[port] == cfg;
        }

        static struct PortCtrl{
            int allOn;
            PortCfg port[12 + 1];
            int undefCall;
        } portCtrl;

    private:
        /*******************************************************************************************
        ** PRIVATE METHODS
        *******************************************************************************************/
        CAProtocolCtx caProto;
        static std::queue<uint8_t> testString;
        static int testReader(uint8_t* rxBuf)
        {
            *rxBuf = testString.front();
            testString.pop();
            return 0;
        }

        static void allOn(bool state, int duration_ms) {
            portCtrl.allOn++;
        }

        static void portState(int port, bool state, int percent, int duration)
        {
            if (port > 0 && port <= 12) {
                portCtrl.port[port] = { state, percent, duration };
            }
        }

        static void undefined(const char* input)
        {
            portCtrl.undefCall++;
        }
};

std::queue<uint8_t> TestCAProtocol::testString;
TestCAProtocol::PortCtrl TestCAProtocol::portCtrl;

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(TestCAProtocol, testCalibration)
{
    reset();

    EXPECT_EQ(testCalibration("CAL 3,0.05,1.56\r", 1, (const CACalibration[]) {{3, 0.05, 1.56}}), 0);
    EXPECT_EQ(testCalibration("CAL 3,0.05,1.56 2,344,36\n\r", 2, (const CACalibration[]) {{3, 0.05, 1.56},{2, 344, 36}}), 0);
    EXPECT_EQ(testCalibration("CAL 3,0.05,1.56 2,0.04,.36\n", 2, (const CACalibration[]) {{3, 0.05, 1.56},{2, 0.04, 0.36}}), 0);
}

TEST_F(TestCAProtocol, testPortCtrl)
{
    reset();

    testPortCtrl("all on\r\n", -1, PortCfg());
    EXPECT_EQ(portCtrl.allOn, 1);

    testPortCtrl("all off\r\n", -1, PortCfg());
    EXPECT_EQ(portCtrl.allOn, 2);

    reset();
    EXPECT_NE(testPortCtrl("p10 off\r\n", 10, PortCfg(false, 0, -1)), 0);
    EXPECT_NE(testPortCtrl("p10 on\r\n", 10, PortCfg(true, 100, -1)), 0);
    EXPECT_NE(testPortCtrl("p9 off\r\n", 9, PortCfg(false, 0, -1)), 0);
    EXPECT_NE(testPortCtrl("p9 on\r\n", 9, PortCfg(true, 100, -1)), 0);
    EXPECT_NE(testPortCtrl("p8 on 50\r\n", 8, PortCfg(true, 100, 50)), 0);
    EXPECT_NE(testPortCtrl("p8 on 50%\r\n", 8, PortCfg(true, 50, -1)), 0);
    EXPECT_NE(testPortCtrl("p8 off 50%\r\n", 8, PortCfg(false, 0, -1)), 0);
    EXPECT_NE(testPortCtrl("p8 on 22 60%\r\n", 8, PortCfg(true, 60, 22)), 0);
    EXPECT_NE(testPortCtrl("p11 on 50%\r\n", 11, PortCfg(true, 50, -1)), 0);
    EXPECT_NE(testPortCtrl("p11 off 50%\r\n", 11, PortCfg(false, 0, -1)), 0);

    // Test some error cases
    EXPECT_NE(testPortCtrl("p7 60\r\n", 7, PortCfg()), 0);
    EXPECT_NE(testPortCtrl("p7 60%\r\n", 7, PortCfg()), 0);
    EXPECT_NE(testPortCtrl("p7 52 60%\r\n", 7, PortCfg()), 0);
    EXPECT_NE(testPortCtrl("p7 on 60e\r\n", 7, PortCfg()), 0);
    EXPECT_NE(testPortCtrl("p7 on 52 60\r\n", 7, PortCfg()), 0);
    EXPECT_NE(testPortCtrl("p7 sdfs 52 60%\r\n", 7, PortCfg()), 0);
    EXPECT_EQ(portCtrl.undefCall, 6);
}