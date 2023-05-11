/*
 * notchFilter.h
 *
 *  Created on: Feb 28, 2023
 *      Author: matias
 */

#ifndef INC_NOTCHFILTER_H_
#define INC_NOTCHFILTER_H_

#include "math.h"

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

typedef struct {

	float alpha;	// filter coefficients for current and previous inputs
	float beta;		// filter coefficients for current and previous outputs
	float x[3];		// measured inputs
	float y[3];		// filtered outputs

} NotchFilter;

void InitNotchFilter(NotchFilter *filter, float centerFreqHz, float notchWidthHz, float Ts);
float UpdateNotchFilter(NotchFilter *filter, float x0);

#endif /* INC_NOTCHFILTER_H_ */
