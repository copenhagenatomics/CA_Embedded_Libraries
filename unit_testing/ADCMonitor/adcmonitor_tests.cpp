/*!
** @file   adcmonitor_tests.cpp
** @author Matias
** @date   17/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */
#include "fake_stm32xxxx_hal.h"
/* Real supporting units */

/* UUT */
#include "ADCmonitor.c"

using ::testing::AnyOf;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class ADCMonitorTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        ADCMonitorTest() {}
 
        static void generateSine(int16_t* pData, int noOfChannels, int noOfSamples, int channel,  int amplitudeOffset, int amplitude, int freq)
        {
            const float Ts = 1.0/10000.0;
            for (int i = 0; i<noOfSamples; i++)
                pData[noOfChannels*i + channel] = amplitudeOffset + (amplitude * sin( 2*M_PI*(i*Ts)*freq ));
        }

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
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(ADCMonitorTest, testADCMean)
{
    const int noOfSamples = 100;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2];

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1 + 1;
        pData[noOfChannels*i+1] = i*2 + 1;
    }

    ADC_HandleTypeDef foo = { { 2 }, 0, 0 };
    ADCMonitorInit(&foo, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&foo); 

    EXPECT_EQ(ADCMean(pData, 0), 50.5);
    EXPECT_EQ(ADCMean(pData, 1), 100);
}

TEST_F(ADCMonitorTest, testADCMeanBitShift)
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


    EXPECT_EQ(ADCMeanBitShift(pData, 0, 8), 127);
    EXPECT_EQ(ADCMeanBitShift(pData, 1, 8), 255);
}

TEST_F(ADCMonitorTest, testADCAbsMean)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    generateSine(pData, noOfChannels, noOfSamples, 0, 0, 2047, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 1, 0, 1023, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    EXPECT_EQ(ADCAbsMean(pData, 0), 1259.60);
    EXPECT_EQ(ADCAbsMean(pData, 1), 629.20);
}

TEST_F(ADCMonitorTest, testADCmax)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 5;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1;
        pData[noOfChannels*i+1] = i*2;
    }

    const int AMPLITUDE = 2047; 
    const int OFFSET_1 = 2047;
    const int OFFSET_2 = 0;
    const int OFFSET_3 = -2047;

    generateSine(pData, noOfChannels, noOfSamples, 2, OFFSET_1, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 3, OFFSET_2, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 4, OFFSET_3, AMPLITUDE, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy); 

    EXPECT_EQ(ADCmax(pData,0), noOfSamples-1);
    EXPECT_EQ(ADCmax(pData,1), (noOfSamples-1)*2);
    EXPECT_EQ(ADCmax(pData,2), 3993);
    EXPECT_EQ(ADCmax(pData,3), 1946);
    EXPECT_EQ(ADCmax(pData,4), -100);
}

TEST_F(ADCMonitorTest, testADCmin)
{
    const int noOfSamples = 1000;
    const int noOfChannels = 5;
    int16_t pData[noOfSamples*noOfChannels*2] = {0};

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[noOfChannels*i] = i*1 + 12;
        pData[noOfChannels*i+1] = -i*2;
    }

    const int AMPLITUDE = 2047;
    const int OFFSET_1 = 2047;
    const int OFFSET_2 = 0;
    const int OFFSET_3 = -2047;
    generateSine(pData, noOfChannels, noOfSamples, 2, OFFSET_1, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 3, OFFSET_2, AMPLITUDE, 1000);
    generateSine(pData, noOfChannels, noOfSamples, 4, OFFSET_3, AMPLITUDE, 1000);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy);

    EXPECT_EQ(ADCmin(pData,0), 12);
    EXPECT_EQ(ADCmin(pData,1), (1-noOfSamples)*2);
    EXPECT_EQ(ADCmin(pData,2), 100);
    EXPECT_EQ(ADCmin(pData,3), -1946);
    EXPECT_EQ(ADCmin(pData,4), -3993);
}

TEST_F(ADCMonitorTest, testADCSetOffset)
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
        EXPECT_EQ(pData[i], 0);
    }
}

TEST_F(ADCMonitorTest, testCMAverage)
{
    const int noOfSamples = 10;
    int16_t pData[noOfSamples*4*2];

    for (int i = 0; i<noOfSamples; i++)
    {
        pData[4*i] = (i % 10) * 20;
    }
    ADC_HandleTypeDef dummy = { { 4 } };
    ADCMonitorInit(&dummy, pData, noOfSamples*4*2);

    EXPECT_EQ(cmaAvarage(pData, 0, 85, 5), 112);
}

TEST_F(ADCMonitorTest, testADCrms)
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

    EXPECT_NEAR(ADCrms(pData,0), 2506.606445, tol);
    EXPECT_NEAR(ADCrms(pData,1), 2170.527588, tol);
}

TEST_F(ADCMonitorTest, testADCTrueRms)
{
    const float tol = 0.0001;
    const int noOfSamples = 1000;
    const int noOfChannels = 2;
    int16_t pData[noOfSamples*noOfChannels*2];
    uint16_t noOfPointsRMS = 990;

    generateSine(pData, noOfChannels, noOfSamples, 0, 2047, 2047, 909);
    generateSine(pData, noOfChannels, noOfSamples, 1, 2047, 1023, 909);

    ADC_HandleTypeDef dummy = { { noOfChannels } };
    ADCMonitorInit(&dummy, pData, noOfSamples*noOfChannels*2);
    HAL_ADC_ConvHalfCpltCallback(&dummy);

    EXPECT_FLOAT_EQ(ADCTrueRms(pData,0, 1100), ADCrms(pData,0));

    EXPECT_NEAR(ADCTrueRms(pData,0, noOfPointsRMS), 2506.729307, tol);
    EXPECT_NEAR(ADCTrueRms(pData,1, noOfPointsRMS), 2170.621540, tol);
}

TEST_F(ADCMonitorTest, testSine)
{
    const int noOfSamples = 120;
    int16_t pData[noOfSamples*4*2];
    generate4Sines(pData, noOfSamples, 0, 10);

    ADC_HandleTypeDef dommy = { { 4 } };
    ADCMonitorInit(&dommy, pData, noOfSamples*4*2);

    SineWave s = sineWave(pData, 4, noOfSamples, 0);
    EXPECT_EQ(s.begin, 9);
    EXPECT_EQ(s.end, 117);

    s = sineWave(pData, 4, noOfSamples, 1);
    EXPECT_EQ(s.begin, 15);
    EXPECT_EQ(s.end, 105);

    s = sineWave(pData, 4, noOfSamples, 2);
    EXPECT_EQ(s.begin, 3);
    EXPECT_EQ(s.end, 111);
}