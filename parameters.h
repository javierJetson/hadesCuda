#pragma once
#ifndef _PARAMETERS_H
#define _PARAMETERS_H

// Sim parameters
#define nNeurons 	1000
#define nMuscles	1
#define deltaT 		1.00f

// Neuron parameters
#define IZN_peakV	30.0f //mV
#define IZN_a		0.02f
#define IZN_b		0.2f
#define IZN_c		-65.0f
#define IZN_d		2.0f
#define IZN_I		15.0f

// Muscle parameters
#define applyForceLengthWeight 1

#endif //!_PARAMETERS_H

