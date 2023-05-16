#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

extern "C" {
    #include "goertzel.h"
}

static void generateSine(int32_t* pData, int length, int freq, float sampleTime, float amplitude)
{
    for (int i = 0; i<length; i++)
    {
        pData[i] = amplitude + (amplitude * sin(2*M_PI*freq*(i*sampleTime)));
    }
}

int runGoertzel(int32_t* pData, int noOfSamples, float amplitude)
{
    float mag;
    float tol = 0.000005;
    for (int i = 0; i<40; i++)
        computeSignalPower(&pData[i*5], 1, 5, &mag);

    if ((mag - amplitude) >= tol)
        return __LINE__;
    return 0;
}

int testGoertzel()
{
    // Create an array used for buffer data.
    float fs = 200000.0;
    float Ts = 1.0/fs;
    int noOfSamples = 200;
    int targetFreq = 2000;
    float peakToPeak = 65536.0;
    float amplitude = peakToPeak/2.0;

    GoertzelInit(targetFreq,fs, (float) noOfSamples,peakToPeak,3.3,4.7);

    int32_t pData[noOfSamples];
    generateSine(pData, noOfSamples, targetFreq, Ts, amplitude);

    if (runGoertzel(pData, noOfSamples, amplitude*(1/(peakToPeak*3.3/4.7))))
        return __LINE__;
    return 0;
}

int main(int argc, char *argv[])
{
    int line = 0;
    if (line = testGoertzel()) { printf("TestGoertzel failed at line %d\n", line); }
    if (line == 0) { printf("All tests run successfully\n"); }
    return 0;
}