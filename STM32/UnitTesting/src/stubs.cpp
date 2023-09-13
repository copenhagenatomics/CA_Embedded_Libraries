#include <stdio.h>
#include <string.h>
#include <queue>
extern "C" {
    #include "stm32f4xx_hal.h"
    #include <ADCMonitor.h>
    #include <CAProtocol.h>
}
#include <math.h>
#include <assert.h>

// HW depended functions, stub these.
void HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length) {}
void HAL_Delay(uint32_t var) {}
int USBnprintf(const char * format, ... ) {
    return 0;
}
void JumpToBootloader() {};
void CAPrintStatus(bool printStart) {};


static void generate4Sines(int16_t* pData, int length, int offset, int freq)
{
    for (int i = 0; i<length; i++)
    {
        pData[4*i+0] = 2041 + (2041.0 * sin( (i*freq + offset      )/180.0 * M_PI));
        pData[4*i+1] = 2041 + (2041.0 * sin( (i*freq + offset + 120)/180.0 * M_PI));
        pData[4*i+2] = 2041 + (2041.0 * sin( (i*freq + offset + 240)/180.0 * M_PI));
        pData[4*i+3] = (42 + i) & 0xFFFF;
    }
}

static void generateSine(int16_t* pData, int noOfChannels, int noOfSamples, int channel,  int amplitudeOffset, int amplitude, int freq)
{
    const float Ts = 1.0/10000.0;
    for (int i = 0; i<noOfSamples; i++)
        pData[noOfChannels*i + channel] = amplitudeOffset + (amplitude * sin( 2*M_PI*(i*Ts)*freq ));
}

// Helper function to be used during debug.
static void debugPData(const int16_t* pData, int length, int channel)
{
    for (int i = 0; i<length; i++)
    {
        if ((i) % 10 == 0)
            printf("%4d: ", i);

        printf("%4d ",pData[4*i+channel]);
        if ((i+1) % 10 == 0)
            printf("\n");
    }
}

int testSine()
{
    // Create an array used for buffer data.
    const int noOfSamples = 120;
    int16_t pData[noOfSamples*4*2];
    generate4Sines(pData, noOfSamples, 0, 10);

    ADC_HandleTypeDef dommy = { { 4 } };
    ADCMonitorInit(&dommy, pData, noOfSamples*4*2);

    SineWave s = sineWave(pData, 4, noOfSamples, 0);

    if (s.begin != 9 || s.end != 117)
        return __LINE__;
    s = sineWave(pData, 4, noOfSamples, 1);
    if (s.begin != 15 || s.end != 105)
        return __LINE__;
    s = sineWave(pData, 4, noOfSamples, 2);
    if (s.begin != 3 || s.end != 111)
        return __LINE__;
    return 0;
}

int testADCrms()
{
    const float tol = 0.0001;
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2];

    generateSine(pData, noOfChannels, noOfSamples, 0, 2047, 2047, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 1, 2047, 1023, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    if ((ADCrms(pData,0) - 2506.606445) > tol)
        return __LINE__;
    if ((ADCrms(pData,1) - 2170.527588) > tol)
        return __LINE__;
    return 0;
}

int testADCMean()
{
    const int noOfSamples = 100;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2];

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1;
        pData[noOfChannels*i+1] = i*2;
    }

    ADC_HandleTypeDef dummy = { { 2 } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    if (ADCMean(pData,0) != 49.50)
        return __LINE__;
    if (ADCMean(pData,1) != 99)
        return __LINE__;
    return 0;
}

int testADCMeanBitShift()
{
    const int noOfSamples = 256;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2];

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i;
        pData[noOfChannels*i+1] = i*2;
    }

    ADC_HandleTypeDef dummy = { { 2 } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    if (ADCMeanBitShift(pData,0,8) != 127)
        return __LINE__;
    if (ADCMeanBitShift(pData,1,8) != 255)
        return __LINE__;
    return 0;
}

// TODO: Implementation
int testADCAbsMean()
{
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    generateSine(pData, noOfChannels, noOfSamples, 0, 0, 2047, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 1, 0, 1023, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    if (ADCAbsMean(pData,0) != 1259.60)
        return __LINE__;
    if (ADCAbsMean(pData,1) != 629.20)
        return __LINE__;

    return 0;
}

int testADCmax()
{
    const int noOfSamples = 1000;
    const int noOfChannels = 3;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1;
        pData[noOfChannels*i+1] = i*2;
    }

    const int amplitude = 2047; 
    const int offset = 2047;
    generateSine(pData, noOfChannels, noOfSamples, 2, offset, amplitude, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    if (ADCmax(pData,0) != noOfSamples-1)
        return __LINE__;
    if (ADCmax(pData,1) != (noOfSamples-1)*2)
        return __LINE__;
    if (ADCmax(pData,2) != 3993)
        return __LINE__;
    return 0;
}

int testADCSetOffset()
{
    const int noOfSamples = 1000;
    const int noOfChannels = 3;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    int16_t dcValues[3] = {2055, 4085, 16};
    for (int i = 0; i < noOfSamples; i++)
    {
        pData[noOfChannels*i] = dcValues[0];
        pData[noOfChannels*i+1] = dcValues[1];
        pData[noOfChannels*i+2] = dcValues[2];
    }

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    ADCSetOffset(pData, -dcValues[0], 0);
    ADCSetOffset(pData, -dcValues[1], 1);
    ADCSetOffset(pData, -dcValues[2], 2);

    for (int i = 0; i < noOfChannels*noOfSamples; i++)
    {
        if (pData[i] != 0)
            return __LINE__;
    }
    return 0;
}

int testCMAverage()
{
    const int noOfSamples = 10;
    int16_t pData[noOfSamples*4*2];

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[4*i] = (i % 10) * 20;
    }
    ADC_HandleTypeDef dummy = { { 4 } };
    ADCMonitorInit(&dummy, pData, noOfSamples*4*2);

    if (cmaAvarage(pData, 0, 85, 5) != 112)
        return __LINE__;

    return 0;
}

static struct {
    CACalibration cal[10];
    int noOfCallibration;
} calData;
void CAClibrationCb(int noOfPorts, const CACalibration *catAr) {
    calData.noOfCallibration = noOfPorts;
    memcpy(calData.cal, catAr, sizeof(calData.cal));
}
int calCompare(int noOfPorts, const CACalibration* catAr)
{
    if (noOfPorts != calData.noOfCallibration)
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

class TestCAProtocol
{
public:
    TestCAProtocol()
    {
        caProto.calibration = CAClibrationCb;
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
    CAProtocolCtx caProto;;
    static std::queue<uint8_t> testString;
    static int testReader(uint8_t* rxBuf)
    {
        *rxBuf = testString.front();
        testString.pop();
        return 0;
    }
    static void allOn(bool state) {
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

int testCalibration()
{
    TestCAProtocol caProtocol;
    caProtocol.reset();

    if (caProtocol.testCalibration("CAL 3,0.05,1.56\r", 1, (const CACalibration[]) {{3, 0.05, 1.56}}))
        return __LINE__;
    if (caProtocol.testCalibration("CAL 3,0.05,1.56 2,344,36\n\r", 2, (const CACalibration[]) {{3, 0.05, 1.56},{2, 344, 36}}))
        return __LINE__;
    if (caProtocol.testCalibration("CAL 3,0.05,1.56 2,0.04,.36\n", 2, (const CACalibration[]) {{3, 0.05, 1.56},{2, 0.04, 0.36}}))
        return __LINE__;
    return 0; // All good.
}

int testPortCtrl()
{
    TestCAProtocol caProtocol;
    caProtocol.reset();

    caProtocol.testPortCtrl("all on\r\n", -1, PortCfg());
    if (caProtocol.portCtrl.allOn != 1) return __LINE__;
    caProtocol.testPortCtrl("all off\r\n", -1, PortCfg());
    if (caProtocol.portCtrl.allOn != 2) return __LINE__;

    caProtocol.reset();
    if (!caProtocol.testPortCtrl("p10 off\r\n", 10, PortCfg(false, 0, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p10 on\r\n", 10, PortCfg(true, 100, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p9 off\r\n", 9, PortCfg(false, 0, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p9 on\r\n", 9, PortCfg(true, 100, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p8 on 50\r\n", 8, PortCfg(true, 100, 50))) return __LINE__;
    if (!caProtocol.testPortCtrl("p8 on 50%\r\n", 8, PortCfg(true, 50, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p8 off 50%\r\n", 8, PortCfg(false, 0, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p8 on 22 60%\r\n", 8, PortCfg(true, 60, 22))) return __LINE__;
    if (!caProtocol.testPortCtrl("p11 on 50%\r\n", 11, PortCfg(true, 50, -1))) return __LINE__;
    if (!caProtocol.testPortCtrl("p11 off 50%\r\n", 11, PortCfg(false, 0, -1))) return __LINE__;

    // Test some error cases
    if (!caProtocol.testPortCtrl("p7 60\r\n", 7, PortCfg())) return __LINE__;
    if (!caProtocol.testPortCtrl("p7 60%\r\n", 7, PortCfg())) return __LINE__;
    if (!caProtocol.testPortCtrl("p7 52 60%\r\n", 7, PortCfg())) return __LINE__;
    if (!caProtocol.testPortCtrl("p7 on 60e\r\n", 7, PortCfg())) return __LINE__;
    if (!caProtocol.testPortCtrl("p7 on 52 60\r\n", 7, PortCfg())) return __LINE__;
    if (!caProtocol.testPortCtrl("p7 sdfs 52 60%\r\n", 7, PortCfg())) return __LINE__;
    if (caProtocol.portCtrl.undefCall != 6)  return __LINE__;
    return 0;
}

int main(int argc, char *argv[])
{
    int line = 0;
    if (line = testSine()) { printf("testSine failed at line %d\n", line); }
    if (line = testADCMean()) { printf("testADCMean failed at line %d\n", line); }
    if (line = testADCrms()) { printf("testADCrms failed at line %d\n", line); }
    if (line = testADCMeanBitShift()) { printf("testADCMeanBitShift failed at line %d\n", line); }
    if (line = testADCAbsMean()) { printf("testADCAbsMean failed at line %d\n", line); }
    if (line = testADCmax()) { printf("testADCmax failed at line %d\n", line); }
    if (line = testADCSetOffset()) { printf("testADCSetOffset failed at line %d\n", line); }
    if (line = testCMAverage()) { printf("testCMAverage failed at line %d\n", line); }
    if (line = testCalibration()) { printf("testCAProtocol failed at line %d\n", line); }
    if (line = testPortCtrl()) { printf("testPortCtrl failed at line %d\n", line); }
    if (line == 0) { printf("All tests run successfully\n"); }

    return 0;
}
