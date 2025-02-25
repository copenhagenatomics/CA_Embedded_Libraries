/*!
** @file   notchfilter_tests.cpp
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
#include "notchFilter.h"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class NotchFilterTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        NotchFilterTest() {}

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

TEST_F(NotchFilterTest, testInitialisation)
{
    NotchFilter filter;
    float centerFreqHz = 2000;
    float notchWidthHz = 400;
    float fs = 40000;
    float Ts = 1.0f/fs;

    // Check that inputs/outputs are not set yet
    for (int i = 0; i < 3; i++)
    {
        EXPECT_NE(filter.x[i], 0);
        EXPECT_NE(filter.y[i], 0);
    }

    InitNotchFilter(&filter, centerFreqHz, notchWidthHz, Ts);

    // Check that all inputs/outputs are set to 0
    for (int i = 0; i < 3; i++)
    {
        ASSERT_EQ(filter.x[i], 0);
        ASSERT_EQ(filter.y[i], 0);
    }
}

TEST_F(NotchFilterTest, testNotchFrequency)
{
    NotchFilter filter;
    float centerFreqHz = 2000.0f;
    float notchWidthHz = 400.0f;
    float fs = 40000;
    float Ts = 1.0f/fs;

    InitNotchFilter(&filter, centerFreqHz, notchWidthHz, Ts);

    // Generate a sine with frequency of 2kHz and with fs = 40kHz
    // There are 40kHz/2kHz points per wave so N=400 equals 20 full waves
    const int N = 400;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    generateSine(sineArray, N, amplitude, centerFreqHz, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i < N; i++)
    {
        sineFiltered[i] = UpdateNotchFilter(&filter, sineArray[i]);
    }

    /*  Find the max of the last part of the filtered signal 
     *  and expect that it should be greatly reduced at the notch
     *  frequency. 
     */
    double max = 0;
    int idx = 100;
    max_element(&sineFiltered[N-idx], idx, &max);
    EXPECT_LE(max, 1e-4);
}

TEST_F(NotchFilterTest, testPassFrequencyLow)
{
    NotchFilter filter;
    float centerFreqHz = 2000.0f;
    float notchWidthHz = 400.0f;
    float fs = 40000;
    float Ts = 1.0f/fs;

    InitNotchFilter(&filter, centerFreqHz, notchWidthHz, Ts);

    // Generate a sine with frequency of 2kHz and with fs = 40kHz
    // There are 40kHz/2kHz points per wave so N=400 equals 20 full waves
    const int N = 400;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    float sineFreq = 500;
    generateSine(sineArray, N, amplitude, sineFreq, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i < N; i++)
    {
        sineFiltered[i] = UpdateNotchFilter(&filter, sineArray[i]);
    }

    /*  Find the max of the last part of the filtered signal 
     *  and expect that it should be almost equal to input signal
     */
    double max = 0;
    int idx = 100;
    float tol = 1e-2;
    max_element(&sineFiltered[N-idx], idx, &max);
    EXPECT_NEAR(max, 1, tol);
}

TEST_F(NotchFilterTest, testPassFrequencyHigh)
{
    NotchFilter filter;
    float centerFreqHz = 2000.0f;
    float notchWidthHz = 400.0f;
    float fs = 40000;
    float Ts = 1.0f/fs;

    InitNotchFilter(&filter, centerFreqHz, notchWidthHz, Ts);

    // Generate a sine with frequency of 2kHz and with fs = 40kHz
    // There are 40kHz/2kHz points per wave so N=400 equals 20 full waves
    const int N = 400;
    float sineArray[N] = {0};
    float amplitude = 1.0f;
    float sineFreq = 4500;
    generateSine(sineArray, N, amplitude, sineFreq, fs);

    // Run series through the filter
    double sineFiltered[N] = {0};
    for (int i = 0; i < N; i++)
    {
        sineFiltered[i] = UpdateNotchFilter(&filter, sineArray[i]);
    }

    /*  Find the max of the last part of the filtered signal 
     *  and expect that it should be almost equal to input signal
     */
    double max = 0;
    int idx = 100;
    float tol = 1e-2;
    max_element(&sineFiltered[N-idx], idx, &max);
    EXPECT_NEAR(max, 1, tol);
}
