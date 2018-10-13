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
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** This is the global size of a single vector. */
static const size_t total_size = 1ul<<30; //2^30=1GB

static const size_t def_repeat = 1;
static const size_t var_repeat = 30;

//for portability across BSPlib implementations
static void mcbsp_util_memcpy_char( char * const restrict destination, const char * const restrict source, const size_t size ) {
	for( size_t i = 0; i < size; ++i ) {
		destination[ i ] = source[ i ];
	}
}

static void mcbsp_util_memcpy_fast( void * const restrict _destination, const void * const restrict _source, const size_t size ) {
	const char   * const restrict source      = (const char   * const restrict) _source;
	      char   * const restrict destination = (      char   * const restrict) _destination;
	const size_t * const restrict         src = (const size_t * const restrict) _source;
	      size_t * const restrict        dest = (      size_t * const restrict) _destination;
	size_t i = 0;
	for( ; i < size / sizeof(size_t); ++i ) {
		dest[ i ] = src[ i ];
	}
	i *= sizeof(size_t);
	if( i < size ) {
		mcbsp_util_memcpy_char( destination + i, source + i, size - i );
	}
}

static void mcbsp_util_memcpy( void * const restrict _destination, const void * const restrict _source, const size_t size ) {
	      char * const restrict destination = (      char * const restrict) _destination;
	const char * const restrict source      = (const char * const restrict) _source;
	if( size < 2 * sizeof(size_t) ) {
		mcbsp_util_memcpy_char( destination, source, size );
		return;
	}
	const size_t d_align = ((uintptr_t)destination) % sizeof(size_t);
	const size_t s_align = ((uintptr_t)source)      % sizeof(size_t);
	if( d_align == s_align && d_align > 0 ) {
		const size_t diff = sizeof(size_t) - d_align;
		mcbsp_util_memcpy_char( destination, source, diff );
		mcbsp_util_memcpy_fast( destination + diff, source + diff, size - diff );
	} else {
		mcbsp_util_memcpy_fast( destination, source, size );
	}
}

static const size_t MCBSP_ALIGNMENT = 64;

static void * mcbsp_util_malloc( const size_t size, const char * const name ) {
	void * ret;
	const int rc = posix_memalign( &ret, MCBSP_ALIGNMENT, size );
	if( rc != 0 ) {
		fprintf( stderr, "%s: memory allocation failed!\n", name );
		exit( EXIT_FAILURE );
	}
	return ret;
}
//end snapshot from mcutil memory utils

void timerlib_recordMax( double * const max, const double * const restrict times, const size_t n );
void timerlib_deriveStats(
	double * const max_time, double * const min_time, double * const avg_time,
	double * const var_time, const double * const restrict times, const size_t n
);

int main( int argc, char **argv ) {
	//start SPMD section over all available threads
	bsp_begin( bsp_nprocs() );
	const unsigned int p = bsp_nprocs();
	const unsigned int s = bsp_pid();
	size_t chunk_size = total_size / p;
	size_t repeat = def_repeat;
	bsp_push_reg( &chunk_size, sizeof(size_t) );
	bsp_push_reg( &repeat,     sizeof(size_t) );

	if( s == 0 ) {
		unsigned long int cli_parsed = ULONG_MAX;
		if( argc >= 2 ) {
			//read chunk size from command line
			cli_parsed = strtoul( argv[ 1 ], NULL, 0 );
			if( cli_parsed == 0 || cli_parsed == ULONG_MAX ) {
				bsp_abort( "Usage: %s <chunk size in bytes> <inner repititions, positive integer> (all arguments are optional).\n", argv[ 0 ] );
			} else {
				chunk_size = (size_t)cli_parsed;
				cli_parsed = ULONG_MAX;
			}
		}
		if( argc >= 3 ) {
			cli_parsed = strtoul( argv[ 2 ], NULL, 0 );
			if( cli_parsed == 0 || cli_parsed == ULONG_MAX ) {
				bsp_abort( "Usage: %s <chunk size in bytes> <inner repititions, positive integer> (all arguments are optional).\n", argv[ 0 ] );
			} else {
				repeat = (size_t)cli_parsed;
				cli_parsed = ULONG_MAX;
			}
		}
		if( argc >= 4 ) {
			printf( "Usage: %s <chunk size in bytes> <inner repititions, positive integer> (all arguments are optional).\n", argv[ 0 ] );
			printf( "\nExtra parameters are ignored.\n\n" );
		}
		printf( "Streaming (memcpy) %lf MB of data movement per process, %zd*%zd repetitions, using %d BSP processes.\n", ((double)chunk_size)/1024.0/1024.0*2.0, var_repeat, repeat, p );
	}
	//get settings
	bsp_sync();
	if( s != 0 ) {
		bsp_hpget( 0, &chunk_size, 0, &chunk_size, sizeof(size_t) );
		bsp_hpget( 0, &repeat,     0, &repeat,     sizeof(size_t) );
	}
	bsp_sync();
	assert( chunk_size > 0 );
	assert( repeat > 0 );
	
	//allocate
	double *        times = (s == 0 ? (double*) mcbsp_util_malloc(          p * sizeof(double), "timing array" )          : NULL);
	double *     vartimes = (s == 0 ? (double*) mcbsp_util_malloc( var_repeat * sizeof(double), "variance timing array" ) : NULL);
	double *  start_times = (s == 0 ? (double*) mcbsp_util_malloc(          p * sizeof(double), "start time array" )      : NULL);
	  char * const chunk1 = (char*) mcbsp_util_malloc( chunk_size,  "input memory chunk" );
	  char * const chunk2 = (char*) mcbsp_util_malloc( chunk_size, "output memory chunk" );
	//warm-up
	for( size_t i = 0; i < chunk_size; ++i ) {
		chunk2[ i ] = 0;
	}
	mcbsp_util_memcpy( chunk1, chunk2, chunk_size );
	//initialise benchmark
	for( size_t i = 0; i < chunk_size; ++i ) {
		chunk1[ i ] = 1;
		chunk2[ i ] = 0;
	}
	for( unsigned int i = 0; s == 0 && i < p; ++i ) {
		start_times[ i ] = 0;
		times[ i ] = (double) INFINITY;
	}
	//prepare communication of timings
	if( s == 0 ) {
		//PID 0 must receive all data elements into this buffer
		bsp_push_reg(       times, p * sizeof(double) );
		bsp_push_reg( start_times, p * sizeof(double) );
	} else {
		//other PIDs register buffer only for remote puts
		bsp_push_reg( &start_times, 0 );
		bsp_push_reg( &times, 0 );
	}

	//outer loop for variance measurement
	for( size_t var = 0; var < var_repeat; ++var ) {
		if( s == 0 ) {
			printf( "Pass %zd/%zd...\r", var, var_repeat ); fflush( stdout );
		}

		//synchronised start
		bsp_sync();

		//start timer
		const double start = bsp_time();

		//repeat a couple of times
		for( size_t i = 0; i < repeat; ++i ) {
			//do memcpy
			mcbsp_util_memcpy( chunk2, chunk1, chunk_size );
		}

		//end timer
		const double end = bsp_time();

		//send time to PID 0
		if( s != 0 ) {
			bsp_hpput( 0, &start, &start_times, s * sizeof(double), sizeof(double) );
			bsp_hpput( 0, &end,         &times, s * sizeof(double), sizeof(double) );
		} else {
			assert( times != NULL );
			*start_times = start;
			      *times = end;
		}

		//communicate time taken
		bsp_sync();

		//PID 0 folds min, max, stores difference
		if( s == 0 ) {
			double global_start = start_times[ 0 ];
			for( size_t k = 1; k < p; ++k ) {
				if( global_start < start_times[ 0 ] ) {
					global_start = start_times[ 0 ];
				}
			}
			timerlib_recordMax( vartimes + var, times, p );
			vartimes[ var ] -= global_start;
		}
	}

	//done, do reporting
	if( s == 0 ) {
		//each process moves chunk_size bytes from A to B;
		//hence chunk_size bytes move to the processor, which
		//then writes back chunk_size bytes to RAM.
		//The whole thing is repeated, giving the following
		//number of megabytes transferred:
		const double tot_GB = 2.0 * ((double)repeat) * ((double)p) * ((double)chunk_size) / 1024.0 / 1024.0 / 1024.0;
		//convert from seconds to megabytes per second
		for( size_t i = 0; i < var_repeat; ++i ) {
			vartimes[ i ] = tot_GB / vartimes[ i ];
		}
		//derive statistics
		double max, min, avg, var;
		timerlib_deriveStats( &max, &min, &avg, &var, vartimes, var_repeat );
		//report
		printf(
			"\nSlowest run: %lf GB transferred in %lf seconds; this corresponds to %lf GB/s of effective bandwidth.\n",
			tot_GB, tot_GB / min, min
		);
		printf( "\nFull statistics: min = %lf, avg = %lf, max = %lf, stddev = %lf GB/s.\n", min, avg, max, sqrt(var) );
		free(       times );
		free(    vartimes );
		free( start_times );
	}

	//clean up
	free( chunk1 );
	free( chunk2 );

	//end SPMD section
	bsp_end();
}

