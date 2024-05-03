/*
 * lowpassFilter.c
 *
 *  Created on: Dec 6, 2023
 *      Author: matias
 * 
 *  Description:
 *      First order exponential moving average (low pass IIR filter).
 *      Low cutoff frequencies relative to the sampling frequency
 *      will produce alpha values close to 0 i.e. smooth the signal
 *      to a high degree. 
 *      Higher cutoff frequencies relative to the sampling frequency 
 *      will produce alpha values close to 1 i.e. retain much of the 
 *      input signal.
 * 
 *      Alternatively, the alpha value can be directly chosen. 
 */

#include "lowpassFilter.h"

/*
 *   Inputs: 
 *      filter: Lowpass filter struct
 *      cutOffFrequency: Cut-off frequency with 3 dB attenuation              
 *      fs: sampling frequency
 * 
 *   NOTE: The nyquist theorem states that the fs/cutOffFrequency MUST be
 *         at least 2. Any ratio lower than that gives undefined behaviour.
 *         In addition, the quality of the filter degrades when lowering the
 *         fs/cutOffFrequency ratio. A ratio of 10 or higher is advised. 
 */
void InitLowpassFilter(LowpassFilter *filter, float cutOffFrequency, float fs)
{
    // The following computation to find the filter coefficient alpha is based
    // on this answer: https://dsp.stackexchange.com/a/40465
    float omega3dB = cutOffFrequency * M_PI/(fs/2.0f);
    float cosOmega3dB = cos(omega3dB);
    float alpha = cosOmega3dB - 1 + sqrt(cosOmega3dB*cosOmega3dB - 4.0*cosOmega3dB + 3.0);

    // Ensure alpha is within legal region
    if (alpha > 1)
    {
        alpha = 1;
    }
    else if (alpha < 0)
    {
        alpha = 0;
    }

    // Initialise filter
    filter->alpha = alpha;
    filter->out = 0;
}

void InitLowpassFilterAlpha(LowpassFilter *filter, float alpha)
{
    // Ensure alpha is within legal region
    if (alpha > 1)
    {
        alpha = 1;
    }
    else if (alpha < 0)
    {
        alpha = 0;
    }

    // Initialise filter
    filter->alpha = alpha;
    filter->out = 0;
}

float UpdateLowpassFilter(LowpassFilter *filter, float x0)
{
    // Exponential moving average filter 
    return filter->out = filter->alpha*x0 + (1-filter->alpha)*filter->out;
}


