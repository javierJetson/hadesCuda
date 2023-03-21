#include "IZN.h"
#include "HADES.h"

void IZN_DE_resetter(float* v_in, float* u_in, float* v_out, float* u_out)
{

#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

loop_IZN_eqnReset: 
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		if (v_in[i] >= (float)IZN_peakV) {
			v_out[i] = (float)IZN_c;
			u_out[i] = u_in[i] + (float)IZN_d;
		}
		else {
			v_out[i] = v_in[i];
			u_out[i] = u_in[i];
		}
	}
}

void IZN_DE(float* v_in, float* u_in, float* I_in, float* vd_out, float* ud_out, float* v_in_pass, float* u_in_pass)
{
#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

loop_IZN_Eqns:
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		//vd_out[i] = (0.04f * powf(v_in[i], 2))  + (5.0f * v_in[i]) + 140.0f - u_in[i] + I_in[i];
		vd_out[i] = (0.04f * v_in[i] * v_in[i]) + (5.0f * v_in[i]) + 140.0f - u_in[i] + I_in[i];
		ud_out[i] = (float)IZN_a * (((float)IZN_b * v_in[i]) - u_in[i]);
		v_in_pass[i] = v_in[i];
		u_in_pass[i] = u_in[i];
	}
}

void IZN_DE_v(float* v_in, float* u_in, float* I_in, float* vd_out, float* v_in_pass)
{

#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

loop_IZN_vEqn:
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		//vd_out[i] = (0.04f * powf(v_in[i], 2))  + (5.0f * v_in[i]) + 140.0f - u_in[i] + I_in[i];
		vd_out[i] = (0.04f * v_in[i] * v_in[i]) + (5.0f * v_in[i]) + 140.0f - u_in[i] + I_in[i];
		v_in_pass[i] = v_in[i];
	}
}

void IZN_DE_u(float* v_in, float* u_in, float* ud_out, float* u_in_pass)
{

#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

loop_IZN_uEqn:
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		ud_out[i] = (float)IZN_a * (((float)IZN_b * v_in[i]) - u_in[i]);
		u_in_pass[i] = u_in[i];
	}
}

void IZN_DE_vLimit(float* v_in, float* v_out, spikePulse* rasterOut)
{

#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

loop_IZN_vLimitEqn:
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		if (v_in[i] >= (float)IZN_peakV) {
			v_out[i]		= (float)IZN_peakV;
			rasterOut[i]	= (spikePulse) 1;
		}
		else {
			v_out[i]		= v_in[i];
			rasterOut[i]	= (spikePulse) 0;
		}
	}
}

void IZN_modular(float* delT, float v_in[nNeurons], float u_in[nNeurons], float I_in[nNeurons], float v_out[nNeurons], float u_out[nNeurons], spikePulse rasterOut[nNeurons])
{
	// initial variables
	float v_resetted[nNeurons], u_resetted[nNeurons]; // outputs of state resetter of DE
	float v_temp[nNeurons], u_temp[nNeurons]; // temporary pass-throughs of input states


	float vDelta[nNeurons], uDelta[nNeurons];
	float v_out_unbound[nNeurons]/*, v_out_unbound_temp[nNeurons]*/;

#ifdef __VITIS_HLS__
	#pragma HLS stable variable=I_in
	#pragma HLS DATAFLOW
#endif // __VITIS_HLS__

	// State reset
	IZN_DE_resetter(v_in, u_in, v_resetted, u_resetted);

	// IZN DEs combined evaluation
	IZN_DE(v_in, u_in, I_in, vDelta, uDelta, v_temp, u_temp);

	// IZN v_dot DE evaluation and integration
	//IZN_DE_v(v_resetted, u_resetted, I_in, vDelta, v_temp);
	HADES_euler_vector<float, nNeurons>(delT, v_temp, vDelta, v_out_unbound); // integrate vDelta equation to output new v value
	
	/*
	float delT2 = (*delT) * 0.5;
	IZN_DE_v(v_out_unbound_temp, temp_u, I_in, vDelta);
	HADES_euler<float, nNeurons>(&delT2, v_out_unbound_temp, vDelta, v_out_unbound); // integrate vDelta equation to output new v value
	*/

	IZN_DE_vLimit(v_out_unbound, v_out, rasterOut); // limit output v value to V_peak

	// IZN u_dot DE evaluation and integration	
	//IZN_DE_u(v_resetted, u_resetted, uDelta, u_temp);
	HADES_euler_vector<float, nNeurons>(delT, u_temp, uDelta, u_out); // integrate uDelta equation to output new u value
}

void izhikevich_neuron(const iznParameters &params, iznState& state, float &delT, float &I_in)
{
	float vDot, uDot;
	float v_temp;

#ifdef __VITIS_HLS__
//	#pragma HLS inline
	#pragma HLS dataflow
//	#pragma HLS pipeline
#endif // __VITIS_HLS__
	
	if (state.v >= params.rstV) {
		state.v		= params.c;
		state.u		= state.u + params.d;
		state.spike = 1;
	}
	else {
		vDot		= (0.04f * (state.v) * (state.v)) + (5 * (state.v)) + 140.0f - state.u + I_in; // can run in parallel with line 170
		uDot		= (params.a * ((params.b * state.v) - state.u)); // can run in parallel with 169

#ifdef __VITIS_HLS__
	#pragma HLS pipeline II=1
#endif // __VITIS_HLS__
	
		state.u		= HADES_euler(delT, state.u, uDot); // can run in parallel with 177 and 178 (can be executed as serial step #1)
		v_temp		= HADES_euler(delT, state.v, vDot); // serial step #1
		state.v		= (v_temp >= params.rstV) ? params.rstV : v_temp; // serial step #2
		state.spike = 0;
	}
}

void izhikevich_initStates(iznState *neuronStates)
{
#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS pipeline
#endif // __VITIS_HLS__
	
iznInitLoop: 
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS pipeline
	#pragma HLS unroll
#endif // __VITIS_HLS__

		neuronStates[i].v		= -70.0f;
		neuronStates[i].u		= neuronStates[i].v * (float)IZN_b;
		neuronStates[i].spike	= (spikePulse)0;
	}
}

void izhikevich_population(resetMemory *rst, float *delT, float I_in[nNeurons], iznState outputStates[nNeurons])
{
	// Memory items
	static iznState iznStates[nNeurons];
	static const iznParameters params = iznParameters_default;

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS pipeline
#endif // __VITIS_HLS__
	
	// Neuron state initialization
	if (*rst == (resetMemory)1) {
		izhikevich_initStates(iznStates);
	}
	else {
	iznPopLoop: 
		for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS unroll factor = 50
	#pragma HLS pipeline
#endif // __VITIS_HLS__

			izhikevich_neuron( params, iznStates[i], *delT, I_in[i]); // parallelize this.
		}
	}
	
iznOutputLoop: 
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS unroll factor = 50
	#pragma HLS pipeline
#endif // __VITIS_HLS__

		outputStates[i] = iznStates[i];
	}
}
