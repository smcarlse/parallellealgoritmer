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


#include <bsp.h>
#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

static const size_t     repeat = 1000;
static const size_t var_repeat = 30;

void timerlib_deriveStats(
	double * const max_time, double * const min_time, double * const avg_time,
	double * const var_time, const double * const restrict times, const size_t n
);

void timerlib_recordMax( double * const max, const double * const restrict times, const size_t n );

int main() {
	bsp_begin( bsp_nprocs() );
	const unsigned int p = bsp_nprocs();
	const unsigned int s = bsp_pid();
	double * time = NULL, * times = NULL;
	if( s == 0 ) {
		time  = (double*) malloc(          p * sizeof(double) );
		times = (double*) malloc( var_repeat * sizeof(double) );
		bsp_push_reg( time, p * sizeof(double) );
	} else {
		bsp_push_reg( &time, 0 );
	}
	bsp_sync();

	for( size_t r = 0; r < var_repeat; ++r ) {
		bsp_sync();
		const double start = bsp_time();
		for( size_t i = 0; i < repeat; ++i ) {
			bsp_sync();
		}
		const double end = bsp_time();
		const double time_taken = (end - start) * 1000.0;
		if( s == 0 ) {
			time[ 0 ] = time_taken;
		} else {
			bsp_put( 0, &time_taken, &time, s * sizeof(double), sizeof(double) );
		}
		bsp_sync();
		if( s == 0 ) {
			timerlib_recordMax( times + r, time, p );
			printf( "Pass %zd/%zd: %zd bsp_sync()s took %e ms.\n", r, var_repeat, repeat, times[ r ] );
			times[ r ] /= (double) repeat;
		}
		sleep( 1 );
	}

	if( s == 0 ) {
		double min, avg, max, var;
		timerlib_deriveStats( &max, &min, &avg, &var, times, var_repeat );
		printf( "Time taken for a barrier (bsp_sync): min = %e, avg = %e, max = %e, stddev = %e ms.\n", min, avg, max, sqrt( var ) );
	}

	bsp_end();
}

