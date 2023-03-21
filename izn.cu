#include "IZN.h"
#include "HADES.h"
#include <cuda_runtime.h>

#define CUDA_MAX_NUMBER_OF_THREADS_PER_BLOCK 1024


// This has been CUDAfied - hence why we are not using references & now use pointers.
__global__ void izhikevich_neuronCUDA(const iznParameters *params, iznState *stateArray, float delT, float *I_in_array)
{
	float vDot, uDot;
	float v_temp;
#ifdef __NVCC__
    int neuronThreadIdx = (blockIdx.x * blockDim.x) + threadIdx.x;
    iznState *state = &stateArray[neuronThreadIdx];
    float *I_in = &I_in_array[neuronThreadIdx];
    if(neuronThreadIdx >= nNeurons)
    {
        return; // do nothing if our index is greater than the number of neurons we have in the array.
    }
#endif // __NVCC__
#ifdef __VITIS_HLS__
//	#pragma HLS inline
	#pragma HLS dataflow
//	#pragma HLS pipeline
#endif // __VITIS_HLS__
	
	if (state->v >= params->rstV) {
		state->v		= params->c;
		state->u		= state->u + params->d;
		state->spike = 1;
	}
	else {
		vDot		= (0.04f * (state->v) * (state->v)) + (5 * (state->v)) + 140.0f - state->u + *I_in; // can run in parallel with line 170
		uDot		= (params->a * ((params->b * state->v) - state->u)); // can run in parallel with 169

#ifdef __VITIS_HLS__
	#pragma HLS pipeline II=1
#endif // __VITIS_HLS__
#ifndef __NVCC__
        state->u		= HADES_euler(delT, state->u, uDot); // can run in parallel with 177 and 178 (can be executed as serial step #1)
		v_temp		    = HADES_euler(delT, state->v, vDot); // serial step #1
#else // __NVCC__
        state->u        = (state->u + (delT * uDot));
        v_temp          = (state->v + (delT * vDot));
#endif // __NVCC__
		state->v		= (v_temp >= params->rstV) ? params->rstV : v_temp; // serial step #2
		state->spike = 0;
	}
}

__host__ void izhikevich_initStates(iznState *neuronStates)
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

__host__ void izhikevich_population(resetMemory *rst, float *delT, float I_in[nNeurons], iznState outputStates[nNeurons], iznState outputStatesGPU[nNeurons])
{
	// Memory items
	static iznState iznStates[nNeurons];
	static const iznParameters params = iznParameters_default;

#ifdef __NVCC__
    static iznState *d_iznStates;
    static iznParameters *d_params;
    static float *d_I_in;
    static iznState *d_outputStates;

    int numThreadsPerBlock = CUDA_MAX_NUMBER_OF_THREADS_PER_BLOCK;
    int numBlocks = (nNeurons / CUDA_MAX_NUMBER_OF_THREADS_PER_BLOCK) + 1;
    //delT and params Can be placed into shared memory.

#endif // __NVCC__

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS pipeline
#endif // __VITIS_HLS__
	
	// Neuron state initialization
	if (*rst == (resetMemory)1) {
		izhikevich_initStates(iznStates);
#ifdef __NVCC__
        cudaMalloc((void**) &d_iznStates,    sizeof(iznState) * nNeurons);
        cudaMalloc((void**) &d_params,       sizeof(iznParameters));
        cudaMalloc((void**) &d_I_in,         sizeof(float) * nNeurons);
        cudaMalloc((void**) &d_outputStates, sizeof(iznState) * nNeurons);

        cudaMemcpy(d_iznStates, iznStates,   sizeof(iznState) * nNeurons, cudaMemcpyHostToDevice);
        cudaMemcpy((void *) d_params,        (const void *) &params,      sizeof(iznParameters),       cudaMemcpyHostToDevice);
        cudaMemcpy(d_I_in,      I_in,        sizeof(float) * nNeurons,    cudaMemcpyHostToDevice);
        cudaMemset(d_outputStates, 0x00,     sizeof(iznState) * nNeurons);
#endif // __NVCC__
	}
	else {
	iznPopLoop: 
#ifndef __NVCC__
		for (int i = 0; i < nNeurons; i++) {
			izhikevich_neuron( params, iznStates[i], *delT, I_in[i]); // CPU Run
		}
#endif // __NVCC__
#ifdef __NVCC__
        izhikevich_neuronCUDA<<<numBlocks, numThreadsPerBlock>>>(d_params, d_iznStates, *delT, d_I_in); // GPU run
#endif // __NVCC__	
    }
	
iznOutputLoop: 
#ifndef __NVCC__
	for (int i = 0; i < nNeurons; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS dataflow
	#pragma HLS unroll factor = 50
	#pragma HLS pipeline
#endif // __VITIS_HLS__

		outputStates[i] = iznStates[i];
	}
#endif // __NVCC__

    // now copy the results back from the device, and free memory.
    cudaMemcpy((void *)outputStatesGPU, (void *) d_outputStates, sizeof(iznState) * nNeurons, cudaMemcpyDeviceToHost);
    //cudaFree(d_iznStates);
    //cudaFree(d_params);
    //cudaFree(d_I_in);
    //cudaFree(d_outputStates);
}


