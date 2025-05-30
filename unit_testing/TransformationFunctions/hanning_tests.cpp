/*!
 * @file    hanning_tests.cpp
 * @brief   Hanning window unit tests
 * @date    11/03/2024
 * @author  Timothé D
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */

/* Real supporting units */

/* UUT */
#include "hanning.h"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class HanningTest: public ::testing::Test
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        HanningTest() {}

        static void generateSine(int16_t* pData, uint32_t noOfSamples, uint32_t noOfChannels, uint16_t channel, float amplitude, float phase, float freq, float fs)
        {
            const float Ts = 1.0/fs;
            for (uint32_t i = 0; i < noOfSamples; i++)
                pData[i * noOfChannels + channel] = amplitude * sin( 2*M_PI*(i*Ts)*freq + phase);
        }

        static void generateSineFloat(float* pData, uint32_t noOfSamples, uint32_t noOfChannels, uint16_t channel, float amplitude, float phase, float freq, float fs)
        {
            const float Ts = 1.0/fs;
            for (uint32_t i = 0; i < noOfSamples; i++)
                pData[i * noOfChannels + channel] = amplitude * sin( 2*M_PI*(i*Ts)*freq + phase );
        }
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(HanningTest, testHanningInit)
{
    uint32_t noOfSamples = 4096;
    float coef[noOfSamples] = {0};
    hanningInit(coef, noOfSamples);

    float tol = 1e-8;

    EXPECT_NEAR(coef[0], 0.0, tol);
    EXPECT_NEAR(coef[500], 0.14006373, tol);
    EXPECT_NEAR(coef[noOfSamples - 1], 0.0, tol);

    for (uint32_t i = 1; i < noOfSamples/2; i++)
    {
        EXPECT_TRUE(coef[i] > coef[i-1]);
    }
    for (uint32_t i = noOfSamples/2 + 1; i < noOfSamples; i++)
    {
        EXPECT_TRUE(coef[i] < coef[i-1]);
    }
}

TEST_F(HanningTest, testHanning)
{
    uint32_t noOfSamples = 4096;
    uint32_t noOfChannels = 2;
    int16_t pData[noOfSamples * noOfChannels] = {0};
    float coef[noOfSamples] = {0};
    float amplitude = 15000;
    float phase = 1.0;
    float freq = 50;
    float fs = 400;

    hanningInit(coef, noOfSamples);

    hanning(coef, pData, noOfChannels, noOfSamples, 1);

    for (uint32_t i = 0; i < noOfSamples; i++)
    {
        EXPECT_EQ(pData[i*noOfChannels + 0], 0);
        EXPECT_EQ(pData[i*noOfChannels + 1], 0);
    }

    generateSine(pData, noOfSamples, noOfChannels, 1, amplitude, phase, freq, fs);
    hanning(coef, pData, noOfChannels, noOfSamples, 1);

    for (uint32_t i = 0; i < noOfSamples; i++)
    {
        EXPECT_EQ(pData[i*noOfChannels + 0], 0);
    }

    EXPECT_EQ(pData[0*noOfChannels + 1], 0);
    EXPECT_EQ(pData[2001*noOfChannels + 1], 14636);
    EXPECT_EQ(pData[(noOfSamples - 1)*noOfChannels + 1], 0);
}

TEST_F(HanningTest, testHanningFloatDirect)
{
    uint32_t noOfSamples = 4096;
    uint32_t noOfChannels = 2;
    float pData[noOfSamples * noOfChannels] = {0};
    float amplitude = 15000;
    float phase = 1.0;
    float freq = 50;
    float fs = 400;

    float tol = 1e-6;

    hanningFloatDirect(pData, noOfChannels, noOfSamples, 1);

    for (uint32_t i = 0; i < noOfSamples; i++)
    {
        EXPECT_NEAR(pData[i*noOfChannels + 0], 0, tol);
        EXPECT_NEAR(pData[i*noOfChannels + 1], 0, tol);
    }

    generateSineFloat(pData, noOfSamples, noOfChannels, 1, amplitude, phase, freq, fs);
    hanningFloatDirect(pData, noOfChannels, noOfSamples, 1);

    for (uint32_t i = 0; i < noOfSamples; i++)
    {
        EXPECT_NEAR(pData[i*noOfChannels + 0], 0, tol);
    }

    EXPECT_NEAR(pData[0*noOfChannels + 1], 0, tol);
    EXPECT_NEAR(pData[2001*noOfChannels + 1], 14637.2177734, tol);
    EXPECT_NEAR(pData[(noOfSamples - 1)*noOfChannels + 1], 0, tol);
}
