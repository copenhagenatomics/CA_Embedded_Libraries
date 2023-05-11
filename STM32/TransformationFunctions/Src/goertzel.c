#include "goertzel.h"
#include "math.h"

static Goertzel hGoertzel;

// Initialize the Goertzel filter. 
// Explanations of the parameters are found in goertzel.h
/* Input Parameters:

	targetFrequency: 	The frequency [Hz] of which the magnitude should be computed
	samplerate:			The samplerate [Hz] of pData
	samplesPerOutput:	Total number of samples before the magnitude is computed
	adcres:				The maximum ADC value 12-bit: 4095, 16-bit: 65535
	vRange:				Voltage range of the ADC typically 3.3V
	gain:				Any PCB specific gain from schematic. 
*/ 
void GoertzelInit(int targetFrequency, int sampleRate, float samplesPerOutput, float adcres, float vRange, float gain)
{
	hGoertzel.samplesPerOutput = samplesPerOutput;	
	hGoertzel.k = (int) (0.5 + ((hGoertzel.samplesPerOutput * targetFrequency) / sampleRate));
	hGoertzel.omega = (2.0 * M_PI * hGoertzel.k) / hGoertzel.samplesPerOutput;
	hGoertzel.sine = sin(hGoertzel.omega);
	hGoertzel.cosine = cos(hGoertzel.omega);
	hGoertzel.coeff = 2.0 * hGoertzel.cosine;
	hGoertzel.scalingFactor = 1.0/(hGoertzel.samplesPerOutput / 2.0); // Reciprocal scalingFactor for speed.
	hGoertzel.inputScaling = 1.0/(adcres*vRange/gain); // Reciprocal of inputScaling for speed.
}

// All computation that can be performed once are intialized in GoertzelInit().
// Theory: https://en.wikipedia.org/wiki/Goertzel_algorithm
// Inspired by: https://stackoverflow.com/a/11581251/7683649
// Returns 1 when data magnitude has been updated, otherwise 0.
int computeSignalPower(int32_t *pData, int noOfChannels, int noOfSamples, float * magnitude)
{
    static float q0, q1, q2 = 0;
	static int count = 0;

	for (int i = 0; i < noOfSamples; i++)
	{
		q0 = hGoertzel.coeff * q1 - q2 + (pData[i*noOfChannels]*hGoertzel.inputScaling);
		q2 = q1;
		q1 = q0;
	}

	// The full length of the Goertzel filter is hGoertzel.samplesPerOutput. The interrupt hold noOfSamples samples
	// where noOfSamples <= hGoertzel.samplesPerOutput i.e the filtering process is 
	// run hGoertzel.samplesPerOutput/noOfSamples iterations before returning the magnitude. 
	// We need to reset the filter outputs q0, q1, q2 after every hGoertzel.samplesPerOutput samples 
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