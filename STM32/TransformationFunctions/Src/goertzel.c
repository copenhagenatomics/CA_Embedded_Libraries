#include "goertzel.h"
#include "math.h"

static Goertzel hGoertzel;
static float q0, q1, q2 = 0;
static int sample_no = 0;

/* Input Parameters:

	targetFrequency: 	The frequency [Hz] of which the magnitude should be computed
	samplerate:			The samplerate [Hz] of pData
	samplesPerOutput:	Total number of samples before the magnitude is computed
	adcres:				The maximum ADC value 12-bit: 4096, 16-bit: 65536
	vRange:				Voltage range of the ADC typically 3.3V
	gain:				Any PCB specific gain from schematic. 
	vToUnit:			Conversion factor from Voltage to other unit (bias independent)
*/ 
void GoertzelInit(float adcres, float vRange, float gain, int targetFrequency, int sampleRate, float samplesPerOutput, float vToUnit)
{
    // As the frequency resolution of a "DFT" operation is df = (fs/NDFT) and our target
    // frequency is 2000kHz then we need minimally df = sampleRate/x = 2000Hz -> x = sampleRate/targetFrequency = 100 samples
    // Given the updating frequency on the controller side comes at 1000Hz we choose 200 samples to fit the dynamics.  
    hGoertzel.samplesPerOutput = samplesPerOutput;	
    hGoertzel.k = (int) (0.5 + ((hGoertzel.samplesPerOutput * targetFrequency) / sampleRate));
    hGoertzel.omega = (2.0 * M_PI * hGoertzel.k) / hGoertzel.samplesPerOutput;
    hGoertzel.sine = sin(hGoertzel.omega);
    hGoertzel.cosine = cos(hGoertzel.omega);
    hGoertzel.coeff = 2.0 * hGoertzel.cosine;
    hGoertzel.scalingFactor = 1.0/(hGoertzel.samplesPerOutput / 2.0); // Reciprocal scalingFactor for speed.
    hGoertzel.inputScaling = (1.0/(adcres/vRange*gain))*vToUnit; // Reciprocal of inputScaling for speed.
}

void resetGoertzelParameters()
{
    sample_no = 0;
    q0 = q1 = q2 = 0;
}

// Computation of signal power specifically 2kHz using the Goertzel algorithm.
// All computation that can be performed once are intialized in GoertzelInit(...).
// Theory: https://en.wikipedia.org/wiki/Goertzel_algorithm
// Inspired by: https://stackoverflow.com/a/11581251/7683649
// Returns 1 when data magnitude has been updated, otherwise 0.
int computeSignalPower(int32_t *pData, int noOfChannels, int noOfSamples, int channel, float * magnitude)
{
    // The full length of the Goertzel filter is 200. The interrupt only hold 5 samples
    // i.e the filtering process is run 200/5 = 40 times before returning the magnitude. 
    for (int i = 0; i < noOfSamples; i++)
    {
        q0 = hGoertzel.coeff * q1 - q2 + pData[i*noOfChannels + channel]*hGoertzel.inputScaling;
        q2 = q1;
        q1 = q0;
    }

    // We need to reset the filter outputs q0, q1, q2 after every samplesPerOutput samples (hGoertzel.samplesPerOutput/noOfSamples iterations)
    // As the IIR filter will otherwise become unstable with high probability.
    // Explanation: https://dsp.stackexchange.com/a/30308
    sample_no++;
    if (sample_no >= hGoertzel.samplesPerOutput/noOfSamples)
    {
        // Compute the real part, imaginary part and magnitude
        float real = (q1 - q2 * hGoertzel.cosine) * hGoertzel.scalingFactor;
        float imag = (q2 * hGoertzel.sine) * hGoertzel.scalingFactor;

        *magnitude =  sqrtf(real*real + imag*imag);

        sample_no = 0;
        q0 = q1 = q2 = 0;
        return 1;
    }
    return 0;
}