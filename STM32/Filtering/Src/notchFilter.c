/*
 * notchFilter.c
 *
 *  Created on: Feb 28, 2023
 *      Author: matias
 * 
 * The notch filter implementation is based heavily on this guide https://www.youtube.com/watch?v=ysS4bIXFAsU 
 */

#include "notchFilter.h"

void InitNotchFilter(NotchFilter *filter, float centerFreqHz, float notchWidthHz, float Ts)
{
    float w0 = 2.0f * M_PI * centerFreqHz;
    float ww = 2.0f * M_PI * notchWidthHz;

    // Pre-warped for corrected frequency response
    float w0_pw = (2.0f / Ts) * tanf(0.5f * w0 * Ts);

    filter->alpha = 4.0f + w0_pw * w0_pw * Ts * Ts;
    filter->beta  = 2.0f * ww * Ts;
    filter->scaling_factor = 1.0f/(filter->alpha+filter->beta);
    // Initialise all inputs and outputs to 0.
    for (int i = 0; i < 3; i++)
    {
        filter->y[i] = 0.0;
        filter->x[i] = 0.0;
    }
}

float UpdateNotchFilter(NotchFilter *filter, float x0)
{
    // Shift filter inputs and outputs
    filter->x[2] = filter->x[1];
    filter->x[1] = filter->x[0];

    filter->y[2] = filter->y[1];
    filter->y[1] = filter->y[0];

    // New input
    filter->x[0] = x0;

    filter->y[0] = (filter->alpha*filter->x[0] + 2.0f*filter->x[1]*(filter->alpha-8.0f) + filter->alpha*filter->x[2]
                        - 2.0f*filter->y[1]*(filter->alpha-8.0f) - filter->y[2]*(filter->alpha-filter->beta)) * filter->scaling_factor;

    return filter->y[0];
}