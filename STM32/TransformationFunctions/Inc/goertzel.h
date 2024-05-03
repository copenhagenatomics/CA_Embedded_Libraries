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

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Goertzel {
    int k;                  // Index in frequency vector
    float omega;            // Normalized frequency in (rad/s)
    float sine;             // Value of sine evaluated at omega
    float cosine;           // Value of cosine evaluated at omega
    float coeff;            // Goertzel filter coefficient
    float scalingFactor;    // Normalization factor to map to DFT output
    float inputScaling;
    float samplesPerOutput;
} Goertzel;

//void GoertzelInit(float adcres, float vRange, float gain);
void GoertzelInit(float adcres, float vRange, float gain, int targetFrequency, int sampleRate, float numOfSamplesPerOutput, float vToUnit);
void resetGoertzelParameters();
int computeSignalPower(int32_t *pData, int noOfChannels, int noOfSamples, int channel, float * magnitude);

#ifdef __cplusplus
}
#endif

#endif
