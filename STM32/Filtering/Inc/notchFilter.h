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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	
	float alpha;
	float beta;
	float scaling_factor; 
	float x[3];
	float y[3];

} NotchFilter;

void InitNotchFilter(NotchFilter *filter, float centerFreqHz, float notchWidthHz, float Ts);
float UpdateNotchFilter(NotchFilter *filter, float x0);

#ifdef __cplusplus
}
#endif

#endif /* INC_NOTCHFILTER_H_ */
