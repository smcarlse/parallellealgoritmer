/*
 * Copyright (c) 2015
 *
 * File created by A. N. Yzelman, 2015.
 *
 * This file is part of MulticoreBSP in C --
 *        a port of the original Java-based MulticoreBSP.
 *
 * MulticoreBSP for C is distributed as part of the original
 * MulticoreBSP and is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser 
 * General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * MulticoreBSP is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Lesser General Public 
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General 
 * Public License along with MulticoreBSP. If not, see
 * <http://www.gnu.org/licenses/>.
 */


#include <math.h>
#include <stddef.h>

/**
 * @param max Where to record the maximum (pointer to double).
 * @param times Array of times to take the maximum of (pointer to array of doubles).
 * @param n Length of the array times.
 */
void timerlib_recordMax( double * const max, const double * const restrict times, const size_t n );

/**
 * @param max_time Where to store the maximum time taken.
 * @param min_time Where to store the minimum time taken.
 * @param avg_time Where to store the average time taken.
 * @param var_time Where to store the variance of the time taken measurements.
 * @param times Array of time taken measurements.
 * @param n Number of time measurements taken (length of the times array).
 */
void timerlib_deriveStats(
	double * const max_time, double * const min_time, double * const avg_time,
	double * const var_time, const double * const restrict times, const size_t n
);

void timerlib_recordMax( double * const max, const double * const restrict times, const size_t n ) {
	*max = times[ 0 ];
	for( size_t i = 1; i < n; ++i ) {
		if( times[ i ] > *max ) {
			*max = times[ i ];
		}
	}
}

void timerlib_deriveStats(
	double * const max_time, double * const min_time, double * const avg_time,
	double * const var_time, const double * const restrict times, const size_t n
) {
	*avg_time = *var_time = *max_time = 0;
	*min_time = (double)INFINITY;
	for( size_t i = 0; i < n; ++i ) {
		if( *max_time < times[ i ] ) {
			*max_time = times[ i ];
		}
		if( *min_time > times[ i ] ) {
			*min_time = times[ i ];
		}
		*avg_time += times[ i ] / ((double)n);
	}
	for( size_t i = 0; i < n; ++i ) {
		const double diff = times[ i ] - *avg_time;
		*var_time += diff * diff / ((double)(n - 1));
	}
}

