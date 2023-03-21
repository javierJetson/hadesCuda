#pragma once
#ifndef _IZN_H
#define _IZN_H
#include <stdint.h>
typedef uint8_t	    spikePulse;
typedef uint8_t	    resetMemory;

#include "parameters.h"


typedef struct _iznParameters {
	float a;	/*! Recovery variable time scale.*/
	float b;	/*! Recovery variable sensitivity to sub-threshold membrane potential.*/
	float c;	/*! Membrane potential post-spike reset value.*/
	float d;	/*! Recovery variable post-spike reset value.*/
	float rstV; /*! Membrane potential reset voltage (in mV).*/
} iznParameters;
#define iznParameters_default {IZN_a, IZN_b, IZN_c, IZN_d, IZN_peakV}

typedef struct _neuronState {
	float		v;		/*! Membrane potential.*/
	float		u;		/*! Recovery variable.*/
	spikePulse	spike;	/*! Spike pulse.*/
} iznState;

void IZN_DE_resetter(float* v_in, float* u_in, float* v_out, float* u_out);
void IZN_DE(float* v_in, float* u_in, float* I_in, float* vd_out, float* ud_out, float* v_in_pass, float* u_in_pass);
void IZN_DE_v(float* v_in, float* u_in, float* I_in, float* vd_out, float* v_in_pass);
void IZN_DE_u(float* v_in, float* u_in, float* ud_out, float* u_in_pass);
void IZN_DE_vLimit(float* v_in, float* v_out, spikePulse* rasterOut);
void IZN_modular(float* delT, float v_in[nNeurons], float u_in[nNeurons], float I_in[nNeurons], float v_out[nNeurons], float u_out[nNeurons], spikePulse rasterOut[nNeurons]);
void IZN_monolithic(float* delT, float v_in[nNeurons], float u_in[nNeurons], float I_in[nNeurons], float v_out[nNeurons], float u_out[nNeurons], spikePulse rasterOut[nNeurons]);
// New models

/**
 * @brief Models one izhikevich neuron
 * @param params Reference to the constant structure of parameter of type \ref iznParameters that defines the firing pattern of the neuron.
 * @param state Reference to the updatable state of the neuron including membrane potential (v, in mV), recovery variable (u) and a binary spike output.
 * @param delT Reference to the time period over which numerical integration shoudld be performed.
 * @param I_in Reference to the input synaptic injection current into the neuron.
*/
void izhikevich_initStates(iznState* neuronStates);
#ifdef __NVCC__
void izhikevich_population(resetMemory *rst, float *delT, float I_in[nNeurons], iznState outputStates[nNeurons], iznState outputStatesGPU[nNeurons]);
#else // __NVCC__
void izhikevich_population(resetMemory* rst, float* delT, float I_in[nNeurons], iznState outputStates[nNeurons]);
void izhikevich_neuron(const iznParameters& params, iznState& state, float& delT, float& I_in);
#endif // __NVCC__
#endif // !_IZN_H
