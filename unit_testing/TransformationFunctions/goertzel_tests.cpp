/*!
** @file   goertzel_tests.cpp
** @author Matias
** @date   23/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */

/* Real supporting units */

/* UUT */
#include "goertzel.h"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class GoertzelTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        GoertzelTest() {}

        static void generateSine(int32_t* pData, int noOfSamples, float amplitude, float offset, float freq, float fs)
        {
            const float Ts = 1.0/fs;
            for (int i = 0; i<noOfSamples; i++)
                pData[i] = offset + (amplitude * sin( 2*M_PI*(i*Ts)*freq ));
        }

    // Variables needed for the sine generation and the goertzel filter
    float fs = 50000.0;
    int noOfSamples = 50;
    int targetFreq = 2000;
    float peakToPeak = 16777216.0;
    float amplitude = peakToPeak/2.0;
    float offset = 0;
    float vToUnit = 1;
    float vRange = 3.3;
    float gain = 1;
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(GoertzelTest, testNoOffset)
{
    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, vRange/2.0, tol);
    EXPECT_EQ(ret, 1);
}


TEST_F(GoertzelTest, testPositiveOffset)
{
    // The offset should not affect the output
    offset = amplitude;

    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, vRange/2.0, tol);
    EXPECT_EQ(ret, 1);
}


TEST_F(GoertzelTest, testMCUGain)
{
    // Update MCU gain which should simply scale the output by the 
    // corresponding factor
    gain = 10.0;

    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, (vRange/2.0)/gain, tol);
    EXPECT_EQ(ret, 1);
}

TEST_F(GoertzelTest, testVToUnitGain)
{
    // Update the vToUnit scalar to go from Voltage to any other unit
    // Since the Goertzel is bias independent it does not need a vToUnitBias
    vToUnit = 5;

    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, (vRange/2.0)*vToUnit, tol);
    EXPECT_EQ(ret, 1);
}

TEST_F(GoertzelTest, testAllGains)
{
    // Having both the MCU gain and vToUnit scalar should scale the output
    // correspondingly 
    vToUnit = 10;
    gain = 32.1;

    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, ((vRange/2.0)/gain)*vToUnit, tol);
    EXPECT_EQ(ret, 1);
}

TEST_F(GoertzelTest, testMultipleCalls)
{
    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    float tol = 1e-6;

    // One sample is fed to the filter at the time. 
    // It should only compute the magnitude ones the filter has run
    // for noOfSamples iterations.
    for (int i = 0; i < noOfSamples; i++)
    {
        int ret = computeSignalPower(&pData[i], 1, 1, 0, &mag);
        if (i==(noOfSamples-1))
        {
            // Return value is 1 when the magnitude has been updated.
            EXPECT_NEAR(mag, vRange/2.0, tol);
            EXPECT_EQ(ret, 1);
            continue;
        }
        // Return value is 0 when the magnitude has not been updated.
        EXPECT_EQ(mag, 0);
        EXPECT_EQ(ret, 0);
    }

    // Reset the magnitude and run a single iteration of filtering
    // The magnitude should not be updated and the return value should be 0.
    mag=0;
    int ret = computeSignalPower(&pData[0], 1, 1, 0, &mag);
    EXPECT_EQ(mag, 0);
    EXPECT_EQ(ret, 0);
}

TEST_F(GoertzelTest, testDifferingTargetAndSineFrequency)
{
    float sineFreq = 1000;
    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, sineFreq, fs);

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;

    // As there is no component of the target frequency in the input
    // signal the magnitude should be very close to 0.
    EXPECT_NEAR(mag, 0, tol);
    EXPECT_EQ(ret, 1);
}

TEST_F(GoertzelTest, testDoubleHarmonySine)
{
    // Add two sines with different frequencies. One with the target
    // frequency of the Goertzel filter and one which is some distance away
    // from the target frequency with same amplitude.
    // The filter should not be affected by the second frequency. 
    float secondHarmonyFreq = 40000;

    // Initialise the Goertzel filter
    GoertzelInit(peakToPeak, vRange, gain, targetFreq, fs, noOfSamples, vToUnit);

    // Create a sine that is offset to the middle point of the ADC with an
    // amplitude that fills the whole ADC range.
    int32_t pData[noOfSamples] = {0};
    generateSine(pData, noOfSamples, amplitude, offset, targetFreq, fs);

    int32_t pData2nd[noOfSamples] = {0};
    generateSine(pData2nd, noOfSamples, amplitude/2, offset, secondHarmonyFreq, fs);

    // Combine sines
    for (int i = 0; i < noOfSamples; i++)
    {
        pData[i] += pData2nd[i];
    }

    // Filter the sine through the Goertzel filter
    float mag=0;
    int ret = computeSignalPower(pData, 1, noOfSamples, 0, &mag);

    float tol = 1e-6;
    EXPECT_NEAR(mag, vRange/2.0, tol);
    EXPECT_EQ(ret, 1);
}