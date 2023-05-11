#include "goertzel.h"
#include "math.h"

static Goertzel hGoertzel;

// Initialize the Goertzel filter. 
// Explanations of the parameters are found in goertzel.h
void GoertzelInit(int targetFrequency, int sampleRate, float samplesPerOutput, float adcres, float vRange, float gain)
{
	hGoertzel.samplesPerOutput = samplesPerOutput;	
	hGoertzel.k = (int) (0.5 + ((hGoertzel.samplesPerOutput * targetFrequency) / sampleRate));
	hGoertzel.omega = (2.0 * M_PI * hGoertzel.k) / hGoertzel.samplesPerOutput;
	hGoertzel.sine = sin(hGoertzel.omega);
	hGoertzel.cosine = cos(hGoertzel.omega);
	hGoertzel.coeff = 2.0 * hGoertzel.cosine;
	hGoertzel.scalingFactor = 1.0/(hGoertzel.samplesPerOutput / 2.0); // Reciprocal scalingFactor for speed.

	hGoertzel.resolution = adcres;
	hGoertzel.Vrange = vRange;
	hGoertzel.gain = gain;
}

// All computation that can be performed once are intialized in GoertzelInit().
// Theory: https://en.wikipedia.org/wiki/Goertzel_algorithm
// Inspired by: https://stackoverflow.com/a/11581251/7683649
// Returns 1 when data magnitude has been updated, otherwise 0.
int computeSignalPower(int32_t *pData, int noOfChannels, int noOfSamples, float * magnitude)
{
    static float q0, q1, q2 = 0;
	static int count = 0;

	// The full length of the Goertzel filter is 200. The interrupt only hold 5 samples
	// i.e the filtering process is run 200/5 = 40 times before returning the magnitude. 
	for (int i = 0; i < noOfSamples; i++)
	{
		q0 = hGoertzel.coeff * q1 - q2 + (pData[i*noOfChannels]/hGoertzel.resolution*hGoertzel.Vrange/hGoertzel.gain);
		q2 = q1;
		q1 = q0;
	}

	// We need to reset the filter outputs q0, q1, q2 after every hGoertzel.samplesPerOutput samples (hGoertzel.samplesPerOutput/noOfSamples iterations)
	// As the IIR filter will otherwise become unstable with high probability.
	// Explanation: https://dsp.stackexchange.com/a/30308
	count++;
	if (count >= hGoertzel.samplesPerOutput/noOfSamples)
	{
		// Compute the real part, imaginary part and magnitude
		float real = (q1 - q2 * hGoertzel.cosine) * hGoertzel.scalingFactor;
		float imag = (q2 * hGoertzel.sine) * hGoertzel.scalingFactor;
		*magnitude = sqrtf(real*real + imag*imag);

		count = 0;
		q0 = q1 = q2 = 0;
		return 1;
	}
	return 0;
}