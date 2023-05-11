/*
 * regulation.c
 *
 *  Created on: May 6, 2022
 *      Author: matias
 */

#include "sisopid.h"
#include "math.h"

/*
 * 	CONTROL PARAMETERS
 *
 * 		ybar 	- Setpoint of control system
 * 		y		- Current measurement
 * 		yprev	- Measurement of previous control step
 * 		e		- Error estimate: different between setpoint and current state
 *
 * 		P		- Proportional term
 * 		I		- Integral term
 * 		D 		- Derivational term
 *
 *		reg		- PIDHandle defined and described in regulation.h
 */


// Difference Single-Input Single-Output regulator. This regulator
// works well with models where u should be at some value y when arriving
// at the setpoint e.g. flow regulation or pwm regulation.
void PID(PIDHandle *reg, float ybar, float y, float yprev)
{
	reg->uprev = reg->u;

	float e = ybar - y;
	float P = reg->KP*e;
	// NOTE: Computationally light calculation of D compared to standard computation.
	// Yields same result.
	float D = -reg->KD*(y-yprev)/reg->dt;

	reg->u = reg->us + P + reg->I + D;

	if (reg->u >= reg->umax)
	{
		reg->u = reg->umax;
	}
	else if (reg->u <= reg->umin)
	{
		reg->u = reg->umin;
	}
	else
	{
		reg->I += reg->KI*e*reg->dt;
	}
}