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