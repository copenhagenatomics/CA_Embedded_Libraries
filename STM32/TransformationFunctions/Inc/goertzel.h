/*
 * goertzel.h
 *
 *  Created on: April 24, 2023
 *      Author: matias
 */

#ifndef INC_GOERTZEL_H_
#define INC_GOERTZEL_H_

#include <stdint.h>

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

typedef struct Goertzel {
    int k;                  // Index in frequency vector
    float omega;            // Normalized frequency in (rad/s)
    float sine;             // Value of sine evaluated at omega
    float cosine;           // Value of cosine evaluated at omega
    float coeff;            // Goertzel filter coefficient
    float scalingFactor;    // Normalization factor to map to DFT output
    int samplesPerOutput;   // Accumulated number of samples until magnitude is computed.

    // The following 3 parameters are used to transform the input from ADC to
    // the volts - the magnitudes will be smaller and the computations lighter.
    // The parameters are PCB/MCU dependent.
    float inputScaling;
} Goertzel;

void GoertzelInit(int targetFrequency, int sampleRate, float samplesPerOutput, float adcres, float vRange, float gain);
int computeSignalPower(int32_t *pData, int noOfChannels, int noOfSamples, float * magnitude);

#endif
