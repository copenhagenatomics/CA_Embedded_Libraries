/*!
** @file   lowpassfilter_tests.cpp
** @author Matias
** @date   23/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */

/* Real supporting units */
#include "array-math.h"

/* UUT */
#include "lowpassFilter.h"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class LowPassFilterTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        LowPassFilterTest() {}

        static void generateSine(float* pData, int noOfSamples, float amplitude, float freq, float fs)
        {
            const float Ts = 1.0/fs;
            for (int i = 0; i<noOfSamples; i++)
                pData[i] = amplitude * sin( 2*M_PI*(i*Ts)*freq );
        }
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(LowPassFilterTest, testAlphaSetting)
{
    LowpassFilter lpfilter;

    // Check that setting alpha within the range works
    float alpha = 0.5;
    InitLowpassFilterAlpha(&lpfilter, alpha);
    EXPECT_EQ(lpfilter.alpha, alpha);
    EXPECT_EQ(lpfilter.out, 0);

    // Check that setting alpha below allowed range sets it to 0
    alpha = -0.5;
    InitLowpassFilterAlpha(&lpfilter, alpha);
    EXPECT_EQ(lpfilter.alpha, 0);
    EXPECT_EQ(lpfilter.out, 0);

    // Check that setting alpha above allowed range sets it to 1
    alpha = 1.1;
    InitLowpassFilterAlpha(&lpfilter, alpha);
    EXPECT_EQ(lpfilter.alpha, 1);
    EXPECT_EQ(lpfilter.out, 0);
}

TEST_F(LowPassFilterTest, testCutOffFreqSetting)
{
    LowpassFilter lpfilter;
    
    // Cut off frequency corresponds to the -3dB level point
    float cutOffFrequency = 50.0f;
    float fs = 2000.0f;

    InitLowpassFilter(&lpfilter, cutOffFrequency, fs);

    // Alpha should be within bounds
    EXPECT_GT(lpfilter.alpha, 0);
    EXPECT_LT(lpfilter.alpha, 1);
    EXPECT_EQ(lpfilter.out, 0);

    // Generate a sine with frequency of 50Hz and with fs = 2000
    // There are 2000/50 points per wave so N=400 equals 100 full waves
    const int N = 400;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    generateSine(sineArray, N, amplitude, cutOffFrequency, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i<N; i++)
    {
        sineFiltered[i] = UpdateLowpassFilter(&lpfilter, sineArray[i]);
    }
    
    /*  Find the largest component in the last wave where the filter should have settled.
     *  This should be amplitude/sqrt(2) = 0.7071
     *  We use the max_element since the filter introduces some phase shift
     *  and it is therefore easier than finding the exact index.
     */
    double max = 0;
    max_element(&sineFiltered[360], 40, &max);

    float tol = 0.0025;
    EXPECT_NEAR(max, 0.7071, tol);
}

TEST_F(LowPassFilterTest, testStopBand)
{
    LowpassFilter lpfilter;
    
    // Cut off frequency corresponds to the -3dB level point
    float cutOffFrequency = 50.0f;
    float fs = 40000.0f;

    InitLowpassFilter(&lpfilter, cutOffFrequency, fs);

    // Alpha should be within bounds
    EXPECT_GT(lpfilter.alpha, 0);
    EXPECT_LT(lpfilter.alpha, 1);
    EXPECT_EQ(lpfilter.out, 0);

    // Construct a sine with a frequency of 5kHz (well beyond the cutoff frequency)
    const int N = 40000;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    float sineFreq = 5000;
    generateSine(sineArray, N, amplitude, sineFreq, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i<N; i++)
    {
        sineFiltered[i] = UpdateLowpassFilter(&lpfilter, sineArray[i]);
    }
    
    /*  Find the largest component in the last wave where the filter should have settled.
     *  Here we simply check that the magnitude of the signal is greatly reduced
     *  We use the max_element to find the largest element of the last 10000 points.
     */
    double max = 0;
    max_element(&sineFiltered[30000], 10000, &max);
    EXPECT_LT(max, 1.0f/100.0f);
}


TEST_F(LowPassFilterTest, testPassBand)
{
    LowpassFilter lpfilter;
    
    // Cut off frequency corresponds to the -3dB level point
    float cutOffFrequency = 50.0f;
    float fs = 40000.0f;

    InitLowpassFilter(&lpfilter, cutOffFrequency, fs);

    // Alpha should be within bounds
    EXPECT_GT(lpfilter.alpha, 0);
    EXPECT_LT(lpfilter.alpha, 1);
    EXPECT_EQ(lpfilter.out, 0);

    // Generate a sine with frequency of 50Hz and with fs = 2000
    // There are 2000/50 points per wave so N=400 equals 100 full waves
    const int N = 40000;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    float sineFreq = 5;
    generateSine(sineArray, N, amplitude, sineFreq, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i<N; i++)
    {
        sineFiltered[i] = UpdateLowpassFilter(&lpfilter, sineArray[i]);
    }
    
    /*  Find the largest component in the last wave where the filter should have settled.
     *  Here we simply check that the magnitude of the signal is close to the original amplitude
     *  We use the max_element to find the largest element of the last 10000 points.
     */
    double max = 0;
    max_element(&sineFiltered[30000], 10000, &max);
    EXPECT_GT(max, 0.99);
}