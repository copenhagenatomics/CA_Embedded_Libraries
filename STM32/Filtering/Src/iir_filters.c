/*!
 * @file    iir_filters.h
 * @brief   Implementation of IIR filters
 * @date    03/12/2025
 * @author  Timothé Dodin
 */

#include <math.h>
#include <stdint.h>

#include "iir_filters.h"

/***************************************************************************************************
** DEFINES
***************************************************************************************************/

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif
#ifndef M_TWOPI
#define M_TWOPI (2.0f * M_PI)
#endif

/***************************************************************************************************
** PUBLIC FUNCTION DEFINITIONS
***************************************************************************************************/

/*!
 * @brief Initializes second order iir bandpass filter (normalized by a0)
 * @note https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
 * @param filt Filter structure
 * @param ts Sampling time
 * @param fc Center frequency
 * @param bw Bandwidth
 */
void iir2_band_pass_init(iir2_t *filt, float ts, float fc, float bw) {
    if (filt == NULL) {
        return;
    }

    float w0             = M_TWOPI * fc * ts;
    float bw_octave      = log2f((fc + 0.5 * bw) / (fc - 0.5 * bw));  // From Hz to octaves
    float alpha          = sinf(w0) * sinhf(0.5 * logf(2) * bw_octave * w0 / sinf(w0));
    float scaling_factor = 1.0 / (1 + alpha);

    filt->b0 = alpha * scaling_factor;
    filt->b1 = 0.0;
    filt->b2 = -alpha * scaling_factor;
    filt->a1 = -2.0 * cosf(w0) * scaling_factor;
    filt->a2 = (1 - alpha) * scaling_factor;

    for (uint8_t i = 0; i < 3; i++) {
        filt->x[i] = 0.0;
        filt->y[i] = 0.0;
    }
}

/*!
 * @brief Initializes second order iir bandstop filter (normalized by a0)
 * @note https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
 * @param filt Filter structure
 * @param ts Sampling time
 * @param fc Center frequency
 * @param bw Bandwidth
 */
void iir2_band_stop_init(iir2_t *filt, float ts, float fc, float bw) {
    if (filt == NULL) {
        return;
    }

    float w0             = M_TWOPI * fc * ts;
    float bw_octave      = log2f((fc + 0.5 * bw) / (fc - 0.5 * bw));  // From Hz to octaves
    float alpha          = sinf(w0) * sinhf(0.5 * logf(2) * bw_octave * w0 / sinf(w0));
    float scaling_factor = 1.0 / (1 + alpha);

    filt->b0 = scaling_factor;
    filt->b1 = -2.0 * cosf(w0) * scaling_factor;
    filt->b2 = scaling_factor;
    filt->a1 = -2.0 * cosf(w0) * scaling_factor;
    filt->a2 = (1 - alpha) * scaling_factor;

    for (uint8_t i = 0; i < 3; i++) {
        filt->x[i] = 0.0;
        filt->y[i] = 0.0;
    }
}

/*!
 * @brief Initializes second order iir lowpass filter (normalized by a0)
 * @note https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html
 * @param filt Filter structure
 * @param ts Sampling time
 * @param fc Cutoff frequency
 * @param bw Bandwidth (distance between midpoint (dBgain/2) gain frequencies)
 */
void iir2_low_pass_init(iir2_t *filt, float ts, float fc, float bw) {
    if (filt == NULL) {
        return;
    }

    float w0             = M_TWOPI * fc * ts;
    float bw_octave      = log2f((fc + 0.5 * bw) / (fc - 0.5 * bw));  // From Hz to octaves
    float alpha          = sinf(w0) * sinhf(0.5 * logf(2) * bw_octave * w0 / sinf(w0));
    float scaling_factor = 1.0 / (1 + alpha);

    filt->b0 = 0.5 * (1.0 - cosf(w0)) * scaling_factor;
    filt->b1 = (1.0 - cosf(w0)) * scaling_factor;
    filt->b2 = 0.5 * (1.0 - cosf(w0)) * scaling_factor;
    filt->a1 = -2.0 * cosf(w0) * scaling_factor;
    filt->a2 = (1 - alpha) * scaling_factor;

    for (uint8_t i = 0; i < 3; i++) {
        filt->x[i] = 0.0;
        filt->y[i] = 0.0;
    }
}

/*!
 * @brief Updates second order iir filter
 * @param filt Filter structure
 * @param newValue New sample
 * @return Filtered value
 */
float iir2_update(iir2_t *filt, float newValue) {
    filt->x[2] = filt->x[1];
    filt->x[1] = filt->x[0];
    filt->y[2] = filt->y[1];
    filt->y[1] = filt->y[0];

    filt->x[0] = newValue;
    filt->y[0] = filt->b0 * filt->x[0] + filt->b1 * filt->x[1] + filt->b2 * filt->x[2] -
                 filt->a1 * filt->y[1] - filt->a2 * filt->y[2];

    return filt->y[0];
}
