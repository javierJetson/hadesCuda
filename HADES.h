#pragma once
#ifndef _HADES_H
#define _HADES_H

#include <iterator>
#include "parameters.h"

template <typename T>
	T HADES_euler(T delT, T x, T xDelta)
{

#ifdef __VITIS_HLS__
	#pragma HLS inline
#endif // __VITIS_HLS__
	
	return (x + (delT * xDelta));
}

template <typename T, unsigned nStates>
	void HADES_euler_vector(T* delT, T* xCurrent, T* xDelta, T* xNew)
{

#ifdef __VITIS_HLS__
	#pragma HLS INLINE off
#endif // __VITIS_HLS__

	for (unsigned i = 0; i < nStates; i++) {

#ifdef __VITIS_HLS__
	#pragma HLS PIPELINE
#endif // __VITIS_HLS__

		xNew[i] = xCurrent[i] + ((*delT) * xDelta[i]);
	}
}

#endif // !_HADES_H

