/*
 * sisopid.h
 *
 *  Created on: May 10, 2022
 *      Author: matias
 */

#ifndef INC_SISOPID_H_
#define INC_SISOPID_H_


typedef struct PIDHandle {
	float KP; 		// Proportional gain
	float KI; 		// Integral gain
	float KD; 		// Derivative gain
	float I;		// Integral adder
	float dt; 		// Control period time (Time between actuations)

	float u; 		// Current actuation
	float us; 		// Steady state input
	float uprev; 	// Previous actuation (Used for movement constraints)
	float umin; 	// Minimal input value
	float umax; 	// Maximal input value
	float delta_u_max; 	// Maximal movement per control period
} PIDHandle;


void PID(PIDHandle *reg, float ybar, float y, float yprev);

#endif /* INC_REGULATION_H_ */
