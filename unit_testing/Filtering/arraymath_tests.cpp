/*!
** @file   arraymath_tests.cpp
** @author Luke W
** @date   17/04/2024
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cmath>

/* Fakes */

/* Real supporting units */

/* UUT */
#include "array-math.h"

using ::testing::AnyOf;
using ::testing::AllOf;
using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class ArrayMathTest: public ::testing::Test 
{
    protected:
        /*******************************************************************************************
        ** METHODS
        *******************************************************************************************/
        ArrayMathTest() {}
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(ArrayMathTest, testMaxElement)
{
    double testBuf[100] = {0};

    for(int i = 0; i < 100; i++) {
        testBuf[i] = 100 * std::sin(2.0 * M_PI * i / 100.0);
    }

    double result;
    EXPECT_EQ(0, max_element(testBuf, 100, &result));
    EXPECT_EQ(100.0, result);

    /* Check larger negative numbers don't influence the outcome */
    testBuf[13] = -DBL_MAX;
    EXPECT_EQ(0, max_element(testBuf, 100, &result));
    EXPECT_EQ(100.0, result);

    /* Try a different larger positive number */
    testBuf[58] = DBL_MAX;
    EXPECT_EQ(0, max_element(testBuf, 100, &result));
    EXPECT_EQ(DBL_MAX, result);

    /* Check error returned if 0 length selected */
    result = -DBL_MAX;
    EXPECT_EQ(-1, max_element(testBuf, 0, &result));
    EXPECT_EQ(-DBL_MAX, result);
}

TEST_F(ArrayMathTest, testMinElement)
{
    double testBuf[100] = {0};

    for(int i = 0; i < 100; i++) {
        testBuf[i] = 100 * std::sin(2.0 * M_PI * i / 100.0);
    }

    double result;
    EXPECT_EQ(0, min_element(testBuf, 100, &result));
    EXPECT_EQ(-100.0, result);

    /* Check larger positive numbers don't influence the outcome */
    testBuf[13] = DBL_MAX;
    EXPECT_EQ(0, min_element(testBuf, 100, &result));
    EXPECT_EQ(-100.0, result);

    /* Try a different larger negative number */
    testBuf[58] = -DBL_MAX;
    EXPECT_EQ(0, min_element(testBuf, 100, &result));
    EXPECT_EQ(-DBL_MAX, result);

    /* Check error returned if 0 length selected */
    result = -DBL_MAX;
    EXPECT_EQ(-1, min_element(testBuf, 0, &result));
    EXPECT_EQ(-DBL_MAX, result);
}

TEST_F(ArrayMathTest, testMeanElement)
{
    double testBuf[100] = {0};

    for(int i = 0; i < 100; i++) {
        testBuf[i] = 100 * std::sin(2.0 * M_PI * i / 100.0);
    }

    double result;
    EXPECT_EQ(0, mean_element(testBuf, 100, &result));
    /* Due to floating point errors, the result is not quite zero. This is acceptable */
    EXPECT_NEAR(0.0, result, 1E-10);

    /* Check some other numbers influence the outcome */
    testBuf[0] = -DBL_MAX;
    EXPECT_EQ(0, mean_element(testBuf, 100, &result));
    EXPECT_DOUBLE_EQ(-DBL_MAX/100, result);

    /* Try a different larger positive number */
    testBuf[0] = 5000;
    EXPECT_EQ(0, mean_element(testBuf, 100, &result));
    EXPECT_DOUBLE_EQ(50, result);

    /* Check error returned if 0 length selected */
    result = -DBL_MAX;
    EXPECT_EQ(-1, mean_element(testBuf, 0, &result));
    EXPECT_DOUBLE_EQ(-DBL_MAX, result);
}

TEST_F(ArrayMathTest, testMvgAverage)
{
    unsigned int len = 100;
    double testBuf[len] = {0};
    moving_avg_cbuf_t test_cb;

    /* Initialise moving average handle */
    EXPECT_EQ(0, maInit(&test_cb, testBuf, len));

    /* Add single value and calculate the average */
    double testValue1 = 1;
    double avg = maMean(&test_cb, testValue1);
    EXPECT_EQ(avg, testValue1/test_cb.cbuf_t.len);

    double sum = testValue1;
    // Add a sequence of numbers matching the filter length.
    double testValue = 0;
    for (int i = 1; i<len; i++)
    {
        // Test a range with positive and negative numbers
        testValue = i-len/2;
        sum += testValue;
        avg = maMean(&test_cb, testValue);
        
        EXPECT_EQ(sum, test_cb.sum);
        EXPECT_EQ(avg, sum/len);
    }

    // Check that the circular function works
    sum -= testValue1; 
    testValue = 1000;
    sum += testValue;
    avg = maMean(&test_cb, testValue);
    EXPECT_EQ(avg, sum/len);
}

TEST_F(ArrayMathTest, testMvgVariance)
{
    unsigned int len = 5;
    double testBuf[len] = {0};
    moving_avg_cbuf_t test_cb;

    /* Initialise moving average handle */
    EXPECT_EQ(0, maInit(&test_cb, testBuf, len));

    double tol = 1e-5;
    double variance = 0;

    /* Expected values found using Octave */
    double expectedVariances[len] = {0.2, 0.8, 1.7, 2.5, 2.5};
    for (int i = 0; i < len; i++)
    {
        variance = maVariance(&test_cb, i+1);
        EXPECT_NEAR(variance, expectedVariances[i], tol);
    }

    /* Test that circular computation works - adding another sample wraps around*/
    variance = maVariance(&test_cb, -1);
    EXPECT_NEAR(variance, 5.3, tol);

    /* Test that it can handle a 0 variance */
    EXPECT_EQ(0, maInit(&test_cb, testBuf, len));
    variance = maVariance(&test_cb, 0);
    EXPECT_NEAR(variance, 0, tol);
}

TEST_F(ArrayMathTest, testMvgStdDeviation)
{
    unsigned int len = 5;
    double testBuf[len] = {0};
    moving_avg_cbuf_t test_cb;

    /* Initialise moving average handle */
    EXPECT_EQ(0, maInit(&test_cb, testBuf, len));

    double tol = 1e-5;
    double variance = 0;

    /* Expected values found using Octave */
    double expectedVariances[len] = {0.2, 0.8, 1.7, 2.5, 2.5};
    for (int i = 0; i < len; i++)
    {
        variance = maStdDeviation(&test_cb, i+1);
        EXPECT_NEAR(variance, sqrt(expectedVariances[i]), tol);
    }

    /* Test that circular computation works - adding another sample wraps around*/
    variance = maStdDeviation(&test_cb, -1);
    EXPECT_NEAR(variance, sqrt(5.3), tol);

    /* Test that it can handle a 0 standard deviation */
    EXPECT_EQ(0, maInit(&test_cb, testBuf, len));
    variance = maStdDeviation(&test_cb, 0);
    EXPECT_NEAR(variance, 0, tol);
}


TEST_F(ArrayMathTest, testCbInit)
{
    double testBuf[100] = {0};
    double_cbuf_t test_cb;

    for(int i = 0; i < 100; i++) {
        testBuf[i] = i;
    }

    /* Try calling with the wrong length */
    EXPECT_EQ(-1, cbInit(&test_cb, testBuf, 0));
    for(int i = 1; i < 100; i++) {
        ASSERT_GT(testBuf[i], 0);
    }

    /* Try a good call */
    EXPECT_EQ(0, cbInit(&test_cb, testBuf, 100));
    for(int i = 0; i < 100; i++) {
        ASSERT_EQ(testBuf[i], 0);
    }
}

/* cbPush cannot be blackbox tested alone, only in conjunction with cbMean and cbMax */

TEST_F(ArrayMathTest, testCbMean)
{
    double testBuf[100] = {0};
    double_cbuf_t test_cb;
    EXPECT_EQ(0, cbInit(&test_cb, testBuf, 100));

    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i);
    }

    /* With half the array full, the mean of the last 50 and the last 100 should be the same */
    double result;
    double correct1 = 25 * 49;
    EXPECT_EQ(0, cbMean(&test_cb, 100, &result));
    EXPECT_EQ(correct1 / 100, result);
    EXPECT_EQ(0, cbMean(&test_cb, 50, &result));
    EXPECT_EQ(correct1 / 50, result);

    /* Add some more elements in */
    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i + 100);
    }

    double correct2 = 25 * 249;
    EXPECT_EQ(0, cbMean(&test_cb, 50, &result));
    EXPECT_EQ(correct2 / 50, result);
    EXPECT_EQ(0, cbMean(&test_cb, 100, &result));
    EXPECT_EQ((correct1 + correct2)/100 , result);
    
    /* Add enough to loop round and start filling the buffer again */
    for(int i = 0; i < 26; i++) {
        cbPush(&test_cb, - 1 * i - 100);
    }

    correct1 += - (13 * 225) - (13 * 25);
    EXPECT_EQ(0, cbMean(&test_cb, 100, &result));
    EXPECT_EQ((correct1 + correct2)/100, result);
}

TEST_F(ArrayMathTest, testCbMeanErrors)
{
    double testBuf[100] = {0};
    double_cbuf_t test_cb;
    EXPECT_EQ(0, cbInit(&test_cb, testBuf, 100));

    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i);
    }

    /* With half the array full, the mean of the last 50 and the last 100 should be the same */
    double result = 0;
    double correct1 = 25 * 49;
    EXPECT_EQ(-1, cbMean(&test_cb, 0, &result));
    EXPECT_EQ(-1, cbMean(&test_cb, 500, &result));
    EXPECT_EQ(0, result);

}

TEST_F(ArrayMathTest, testCbMax)
{
    double testBuf[100] = {0};
    double_cbuf_t test_cb;
    EXPECT_EQ(0, cbInit(&test_cb, testBuf, 100));

    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i);
    }

    /* With half the array full, the mean of the last 50 and the last 100 should be the same */
    double result;
    double correct1 = 49;
    EXPECT_EQ(0, cbMax(&test_cb, 100, &result));
    EXPECT_EQ(correct1, result);
    EXPECT_EQ(0, cbMax(&test_cb, 50, &result));
    EXPECT_EQ(correct1, result);

    /* Add some more elements in */
    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i + 100);
    }

    double correct2 = 149;
    EXPECT_EQ(0, cbMax(&test_cb, 50, &result));
    EXPECT_EQ(correct2, result);
    EXPECT_EQ(0, cbMax(&test_cb, 100, &result));
    EXPECT_EQ(correct2, result);
    
    /* Add enough to loop round and start filling the buffer again */
    for(int i = 0; i < 26; i++) {
        cbPush(&test_cb, - 1 * i - 200);
    }

    EXPECT_EQ(0, cbMax(&test_cb, 100, &result));
    EXPECT_EQ(correct2, result);
}

TEST_F(ArrayMathTest, testcbMaxErrors)
{
    double testBuf[100] = {0};
    double_cbuf_t test_cb;
    EXPECT_EQ(0, cbInit(&test_cb, testBuf, 100));

    for(int i = 0; i < 50; i++) {
        cbPush(&test_cb, i);
    }

    /* With half the array full, the mean of the last 50 and the last 100 should be the same */
    double result = 0;
    double correct1 = 49;
    EXPECT_EQ(-1, cbMax(&test_cb, 0, &result));
    EXPECT_EQ(-1, cbMax(&test_cb, 500, &result));
    EXPECT_EQ(0, result);

}

