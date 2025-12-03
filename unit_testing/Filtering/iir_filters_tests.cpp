/*!
 * @file   motor_control_tests.cpp
 * @brief  Motor control unit tests
 * @author Timothé Dodin
 * @date   03/12/2025
 */

#include <stdint.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

/* UUT */
#include "iir_filters.c"

using namespace std;

/***************************************************************************************************
** TEST FIXTURES
***************************************************************************************************/

class IIRFiltersTest : public ::testing::Test {
   protected:
    /*******************************************************************************************
    ** METHODS
    *******************************************************************************************/

    void generateSine(float *array, uint32_t len, float ts, float freq, float amp, float offset) {
        float phaseStep = M_TWOPI * freq * ts;
        for (uint32_t i = 0; i < len; i++) {
            array[i] = offset + amp * sinf(phaseStep * i);
        }
    }
};

/***************************************************************************************************
** TESTS
***************************************************************************************************/

TEST_F(IIRFiltersTest, band_pass) {
    iir2_t filt;
    float ts                      = 1e-4;
    float fc                      = 1e3;
    float bw                      = 1e2;
    uint32_t noOfSamples          = 100000;
    uint32_t stabilizationSamples = 500;
    float tol                     = 2.0;
    float amp                     = 1e2;
    float input[noOfSamples];

    // Frequency in the bandwidth
    iir2_band_pass_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 1e3, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value isn't changed
        EXPECT_NEAR(iir2_update(&filt, input[i]), input[i], tol);
    }

    // Frequency below the bandwidth
    iir2_band_pass_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 1e2, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value is attenuated
        EXPECT_LT(fabsf(iir2_update(&filt, input[i])), 3.0);
    }

    // Frequency above the bandwidth
    iir2_band_pass_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 4e3, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value is attenuated
        EXPECT_LT(fabsf(iir2_update(&filt, input[i])), 3.0);
    }
}

TEST_F(IIRFiltersTest, band_stop) {
    iir2_t filt;
    float ts                      = 1e-4;
    float fc                      = 1e3;
    float bw                      = 1e2;
    uint32_t noOfSamples          = 100000;
    uint32_t stabilizationSamples = 500;
    float tol                     = 2.0;
    float amp                     = 1e2;
    float input[noOfSamples];

    // Frequency in the stopband
    iir2_band_stop_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 1e3, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value is attenuated
        EXPECT_LT(fabsf(iir2_update(&filt, input[i])), 3.0);
    }

    // Frequency below the stopband
    iir2_band_stop_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 1e2, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value isn't changed
        EXPECT_NEAR(iir2_update(&filt, input[i]), input[i], tol);
    }

    // Frequency above the stopband
    iir2_band_stop_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 4e3, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value isn't changed
        EXPECT_NEAR(iir2_update(&filt, input[i]), input[i], tol);
    }
}

TEST_F(IIRFiltersTest, low_pass) {
    iir2_t filt;
    float ts                      = 1e-4;
    float fc                      = 1e3;
    float bw                      = 1e2;
    uint32_t noOfSamples          = 100000;
    uint32_t stabilizationSamples = 500;
    float tol                     = 2.0;
    float amp                     = 1e2;
    float input[noOfSamples];

    // Frequency in the bandwidth
    iir2_low_pass_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 1e2, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value isn't changed
        EXPECT_NEAR(iir2_update(&filt, input[i]), input[i], tol);
    }

    // Frequency above the bandwidth
    iir2_low_pass_init(&filt, ts, fc, bw);
    generateSine(input, noOfSamples, ts, 4e3, amp, 0.0);
    for (uint32_t i = 0; i < stabilizationSamples; i++) {
        iir2_update(&filt, input[i]);
    }
    for (uint32_t i = stabilizationSamples; i < noOfSamples; i++) {
        // Value is attenuated
        EXPECT_LT(fabsf(iir2_update(&filt, input[i])), 3.0);
    }
}
