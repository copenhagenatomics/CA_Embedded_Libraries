/*
 * lowpassFilter.h
 *
 *  Created on: Dec 6, 2023
 *      Author: matias
 */

#ifndef INC_LOWPASSFILTER_H_
#define INC_LOWPASSFILTER_H_

#include "math.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	
	float alpha;    // filter coefficient
	float out;      // filter output (out is used as y[n-1] at next iteration)

} LowpassFilter;

// Initialisation of filter
void InitLowpassFilter(LowpassFilter *filter, float cutOffFrequency, float fs);
void InitLowpassFilterAlpha(LowpassFilter *filter, float alpha);

// Run filter
float UpdateLowpassFilter(LowpassFilter *filter, float x0);

#ifdef __cplusplus
}
#endif

#endif /* INC_LOWPASSFILTER_H_ */
