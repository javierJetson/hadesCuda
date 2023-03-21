#define _CRT_SECURE_NO_WARNINGS // Visual Studio-specific directive to disable deprecation SCANF warnings
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>

#ifndef _MSC_VER
	#include <unistd.h>
#endif // _MSC_VER
#include "IZN.h"
#include "HADES.h"
#include "parameters.h"
#include <sys/time.h>

#define numIters	1000
#define maxTime 	10 // seconds

#define useInternalStates 1

#define minCurrent	  0.0f
#define maxCurrent	 15.0f
#define CurrentStep   5.0f
#define useGoldenInp 1
#ifdef _MSC_VER
	#define goldenInFile "../../../testbenches/iznTest_inputs_90pcTime_golden.csv"
#elif defined(__VITIS_HLS__)
	#define goldenInFile "../../../../../testbenches/iznTest_inputs_90pcTime_golden.csv"
#else
	#define goldenInFile "iznTest_inputs_90pcTime_golden.csv"
#endif // _MSC_VER

#define clampMode	 0
#define stopRandTime 0.9

float prngf(float minVal, float maxVal)
{
	float temp = 1.0f * rand() / RAND_MAX;
	return minVal + ( (maxVal-minVal) * (temp) );
}

using namespace std;
int main()
{
	// Static storage variables
	float delT = deltaT;
	float t;
	float I[nNeurons];

#if useInternalStates == 1
	iznState outputStates[nNeurons];
	iznState outputStatesGPU[nNeurons];
	resetMemory isResetStates = (resetMemory) 1;
#else
	float v[nNeurons], vo[nNeurons];
	float u[nNeurons], uo[nNeurons];
	spikePulse raster[nNeurons];
#endif

	FILE *goldInpPtr, *resFPtr, *inpFPtr;
// Used for the fopen, fprintf and fscanf functions since they have different syntax from their Windows counterparts.
#ifdef __aarch64__
	const char *goldInpFileName = goldenInFile; 
	const char *resFPName = "results.csv";
	const char *inpFPName = "inputs.csv";
	struct timeval start, end;
	double time_taken = 0.0;
#endif // __aarch64__
	char* tempHeader;
	float temp_t;
#ifndef _MSC_VER
	char currPath[512];
	getcwd(currPath, 512);
	printf("Current program path is '%s'\n", currPath);
#endif // !_MSC_VER

	//----------------------
	// Initialize simulator
	//----------------------
	printf("Initializing simulator...\n");
	srand((unsigned) time(NULL));
	t = 0;
		
	// optional golden inputs for testing
#if useGoldenInp == 1
	printf("Sim will use pre-generated inputs from file '%s'.\n", goldenInFile);

#ifndef __aarch64__
	fopen_s(&goldInpPtr, goldenInFile, "r");
#else // __aarch64__
	goldInpPtr = fopen64(goldInpFileName, "r");
#endif // __aarch64__
	tempHeader = (char*) malloc(sizeof(char) * 100000);
	fscanf(goldInpPtr, "%s", tempHeader);
	free(tempHeader);
	tempHeader = nullptr;
#else
	printf("Sim will add psuedo-random generated noise to inputs (initially 0.0).\n");
#endif

	// per neuron state initialization
	fscanf(goldInpPtr, "%f", &temp_t);
	for (int i = 0; i < nNeurons; i++) {
#if useInternalStates == 1
		if (i == 0) { // Reset handled for all neurons internally with one call
 #ifdef __NVCC__
			izhikevich_population(&isResetStates, &delT, I, outputStates, outputStatesGPU);
 #else // __NVCC__
			izhikevich_population(&isResetStates, &delT, I, outputStates);
 #endif // __NVCC__
			// here we can initialize the population of neurons on the CPU, then copy them over to the GPU once done.
			isResetStates = (resetMemory)0;
		}
#else
		v[i] = -70;
		u[i] = ((float)IZN_b) * v[i];
		raster[i] = (spikePulse) 0;
#endif
#if useGoldenInp == 1
		fscanf(goldInpPtr, ",%f", &(I[i]));
#else
		I[i] = 0;
#endif
	}
	fscanf(goldInpPtr, "\n");

	// Initialize log file
#ifndef __aarch64__
	fopen_s(&resFPtr, "results.csv", "w");
#else // __aarch64__
	resFPtr = fopen64(resFPName, "w");
#endif // __aarch64__
	fprintf(resFPtr, "t");
	
#ifndef __aarch64__
	fopen_s(&inpFPtr, "inputs.csv", "w");
#else // __aarch64__
	inpFPtr = fopen64(inpFPName, "w");
#endif // __aarch64__

	fprintf(inpFPtr, "t");
	for (int i = 0; i < nNeurons; i++) {
		fprintf(resFPtr, ",v_%d,u_%d,I_%d,r_%d", i, i, i, i);
		fprintf(inpFPtr, ",I_%d", i);
	}
	fprintf(resFPtr, "\n%f", t);
	fprintf(inpFPtr, "\n%f", t);
	for (int i = 0; i < nNeurons; i++) {
#if useInternalStates == 1
		if(outputStates[i].spike == (spikePulse)0) {
			fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", outputStates[i].v, outputStates[i].u, I[i], (unsigned)0);
		}
		else {
			fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", outputStates[i].v, outputStates[i].u, I[i], (unsigned)1);
		}
#else
		if (raster[i] == (spikePulse)0) {
			fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", v[i], u[i], I[i], (unsigned)0);
		}
		else {
			fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", v[i], u[i], I[i], (unsigned)1);
		}
#endif // useInternalStateMode
		fprintf(inpFPtr, ",%.9f", I[i]);
	}
	fprintf(resFPtr, "\n");
	fprintf(inpFPtr, "\n");

	//-----------------
	// Simulate Neuron
	//-----------------
	printf("Simulation starting...\n");
	printf("Sim step: \n");

	// Simulation loop
	for (int i = 1; i < numIters; i++) {
		//Execute DUT
#if useInternalStates == 1
#ifdef __aarch64__
		gettimeofday(&start, NULL);
		izhikevich_population(&isResetStates, &delT, I, outputStates, outputStatesGPU);
//		izhikevich_population(&isResetStates, &delT, I, outputStates);
		gettimeofday(&end, NULL);
		time_taken += end.tv_sec + end.tv_usec / 1e6 -
                        start.tv_sec - start.tv_usec / 1e6; // in seconds
		
#else // __NVCC__
		izhikevich_population(&isResetStates, &delT, I, outputStates);
#endif // __NVCC__
#else
		IZN_modular(&delT, v, u, I, vo, uo, raster);
#endif
		// Update conditions (and states as necessary)
		t = t + deltaT;
		// per neuron 'j'
		fscanf(goldInpPtr, "%f", &temp_t);
		for (int j = 0; j < nNeurons; j++) {
#if  useInternalStates == 1
			// do nothing - states stored internally
#else
			// update input states
			v[j] = vo[j];
			u[j] = uo[j];
#endif //  useInternalStateMode == 1
#if useGoldenInp == 1
			fscanf(goldInpPtr, ",%f\n", &(I[j]));
#else
#if clampMode == 1
			if (i > (int)(numIters / 10)) {
				I[j] = IZN_I;
			}
			else {
				I[j] = 0;
			}
#else
			if (i <= stopRandTime * numIters) {
				I[j] = I[j] + prngf(-CurrentStep, CurrentStep);
				I[j] = (I[j] < minCurrent) ? minCurrent : ((I[j] > maxCurrent) ? maxCurrent : I[j]);
			}
			else {
				I[j] = 0;
			}
#endif // end clampMode if-else
#endif // end useGoldenInp if-else
		}
		fscanf(goldInpPtr, "\n");

		// Print and log results
		fprintf(resFPtr, "%f", t);
		fprintf(inpFPtr, "%f", t);
		for (int j = 0; j < nNeurons; j++) {
#if useInternalStates == 1
			if(outputStates[j].spike == (spikePulse)0) {
				fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", outputStates[j].v, outputStates[j].u, I[j], (unsigned)0);
			}
			else {
				fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", outputStates[j].v, outputStates[j].u, I[j], (unsigned)1);
			}
#else
			if (raster[j] == (spikePulse)0) {
				fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", v[j], u[j], I[j], (unsigned)0);
			} 
			else {
				fprintf(resFPtr, ",%.9f,%.9f,%.9f,%u", v[j], u[j], I[j], (unsigned)1);
				 //printf(      ",%f,%f,%f,%u", v[j], u[j], I[j], (unsigned)1);
			}
#endif
			fprintf(inpFPtr, ",%f", I[j]);
		}
		fprintf(resFPtr, "\n");
		fprintf(inpFPtr, "\n");
		printf("\t%d\r", i);
	}
	
	printf("time program took %f seconds to execute\n", time_taken);

	//std::cout<<"\nSimulation completed in "<<elapsed_WallClockTime.count()<<" seconds!\n";
	
	return 0;
}
