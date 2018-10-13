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


#include "src/stream-standard-kernel.h"

#include <bsp.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/** This is the global size of a single vector. */
static const size_t total_size = 1ul<<30; //=2^30=1GB, same as stream-memcpy

static const size_t def_repeat = 1;       //same as stream-memcpy
static const size_t var_repeat = 30;      //same as stream-memcpy
static const size_t num_kernels = 8;

//for portability across BSPlib implementations
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

static float scalar;

void timerlib_recordMax( double * const max, const double * const restrict times, const size_t n );
void timerlib_deriveStats(
	double * const max_time, double * const min_time, double * const avg_time,
	double * const var_time, const double * const restrict times, const size_t n
);

static inline void call_kernel( const size_t mode, float * const restrict a, const float * const restrict b, const float * const restrict c, const float * const restrict d, const size_t n, const size_t m ) {
	switch( mode ) {
		case 0:
			kernel2( a, b, n, m );
			break;
		case 1:
			kernel3a( a, b, c, n, m );
			break;
		case 2:
			kernel3b( a, b, c, scalar, n, m );
			break;
		case 3:
			kernel4( a, b, c, d, n, m );
			break;
		case 4:
			kernel2i( a, scalar, n, m );
			break;
		case 5:
			kernel3ai( a, b, n, m );
			break;
		case 6:
			kernel3bi( a, b, scalar, n, m );
			break;
		case 7:
			kernel4i( a, b, c, n, m );
			break;
		default:
			printf( "Unrecognised kernel mode %zd! Ignoring call to call_kernel...\n", mode );
	}
	//done
	return;
}

static inline const char * kernel_str( const size_t mode ) {
	const char * ret;
	switch( mode ) {
		case 0:
			ret = "a[ i ] = b[ i ]";
			break;
		case 1:
			ret = "a[ i ] = b[ i ] + c[ i ]";
			break;
		case 2:
			ret = "a[ i ] = b[ i ] + c[ i ] * d";
			break;
		case 3:
			ret = "a[ i ] = b[ i ] + c[ i ] * d[ i ] ";
			break;
		case 4:
			ret = "a[ i ] = a[ i ] * b";
			break;
		case 5:
			ret = "a[ i ] = a[ i ] + b[ i ]";
			break;
		case 6:
			ret = "a[ i ] = a[ i ] + b[ i ] * c";
			break;
		case 7:
			ret = "a[ i ] = a[ i ] + b[ i ] * c[ i ]";
			break;
		default:
			ret = NULL;
			printf( "Unrecognised kernel mode %zd! Ignoring call to kernel_str...\n", mode );
	}
	//done
	return ret;
}

static inline size_t kernel_bytes( const size_t mode, const size_t chunk_size ) {
	size_t ret;
	switch( mode ) {
		case  0:
			ret = 2 * chunk_size;
			break;
		case  1:
			ret = 3 * chunk_size;
			break;
		case  2:
			ret = 3 * chunk_size;
			break;
		case  3:
			ret = 4 * chunk_size;
			break;
		case  4:
			ret = 2 * chunk_size;
			break;
		case  5:
			ret = 3 * chunk_size;
			break;
		case  6:
			ret = 3 * chunk_size;
			break;
		case  7:
			ret = 4 * chunk_size;
			break;
		default:
			ret = 0;
			printf( "Unrecognised kernel mode %zd! Ignoring call to kernel_str...\n", mode );
	}
	//got number of elements, now return number of bytes
	return ret * sizeof(float);
}

int main( int argc, char **argv ) {
	//start SPMD section over all available threads
	bsp_begin( bsp_nprocs() );
	const unsigned int p = bsp_nprocs();
	const unsigned int s = bsp_pid();
	size_t chunk_size = total_size / p / sizeof(float);
	size_t     repeat = def_repeat;
	bsp_push_reg( &chunk_size, sizeof(size_t) );
	bsp_push_reg( &repeat,     sizeof(size_t) );

	if( s == 0 ) {
		unsigned long int cli_parsed = ULONG_MAX;
		if( argc >= 2 ) {
			//read chunk size from command line
			cli_parsed = strtoul( argv[ 1 ], NULL, 0 );
			if( cli_parsed == 0 || cli_parsed == ULONG_MAX ) {
				bsp_abort( "Usage: %s <thread-local chunk size in bytes> <inner repititions, positive integer> (all arguments are optional).\n", argv[ 0 ] );
			} else {
				chunk_size  = (size_t)cli_parsed;
				chunk_size /= sizeof(float); //chunk size was given in bytes
				cli_parsed  = ULONG_MAX;
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
			printf( "Usage: %s <chunk size in bytes> <inner repititions, "
				"positive integer> (all arguments are optional).\n",
				argv[ 0 ] );
			printf( "\nExtra parameters are ignored.\n\n" );
		}
#ifdef SW_TRIAD
		printf( "This is the software STREAM benchmark. Timings include barriers at "
                        "the end of each kernel call.\n\n" );
#else
		printf( "This is the hardware STREAM benchmark. If successful, total timings "
			"will be exact up to the time taken for a single barrier. If this "
			"cannot be guaranteed, the program will exit with an error.\n\n" );
#endif
		printf(
			"Parallel STREAM benchmark (multiple kernels) computes for local "
			"vectors of length %zd, single-precision floating point (%zd bytes); "
			"%zd*%zd repetitions, using %d BSP processes.\n",
			chunk_size, sizeof(float), var_repeat, repeat, p
		);
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
	double *       times = (s == 0 ? (double*) mcbsp_util_malloc(          p * sizeof(double), "timing array" )          : NULL);
	double * start_times = (s == 0 ? (double*) mcbsp_util_malloc(          p * sizeof(double), "start time array" )      : NULL);
	double *    vartimes = (s == 0 ? (double*) mcbsp_util_malloc( var_repeat * sizeof(double), "variance timing array" ) : NULL);
	 float * const     a = (float*) mcbsp_util_malloc( chunk_size * sizeof(float), "a-vector" );
	 float * const     b = (float*) mcbsp_util_malloc( chunk_size * sizeof(float), "b-vector" );
	 float * const     c = (float*) mcbsp_util_malloc( chunk_size * sizeof(float), "c-vector" );
	 float * const     d = (float*) mcbsp_util_malloc( chunk_size * sizeof(float), "d-vector" );
	for( size_t i = 0; i < chunk_size; ++i ) {
		a[ i ] = 1;
		b[ i ] = 2;
		c[ i ] = 3;
		d[ i ] = 4;
	}
	//get vectors hot, and calculate checksum
	kernel4( a, b, c, d, chunk_size, 1 );
	float checksum = 0.0;
	for( size_t i = 0; i < chunk_size; ++i ) {
		//kernel 4 computed a = b + c * d,
		//hence every value should be 14
		checksum += 14.0f - a[ i ];
	}
	if( checksum != 0.0f ) { 
		bsp_abort( "BSP PID %d has nonzero checksum: %f.\n", bsp_pid(), checksum );
	}
	//initialise benchmark
	for( unsigned int i = 0; s == 0 && i < p; ++i ) {
		start_times[ i ] = 0;
		      times[ i ] = (double) INFINITY;
	}
	//prepare communication of timings
	if( s == 0 ) {
		//PID 0 must receive all data elements into this buffer
		bsp_push_reg( start_times, p * sizeof(double) );
		bsp_push_reg(       times, p * sizeof(double) );
	} else {
		//other PIDs register buffer only for remote puts
		bsp_push_reg( &start_times, 0 );
		bsp_push_reg(       &times, 0 );
	}

	//for each kernel type, do experiment
	for( size_t mode = 0; mode < num_kernels; ++mode ) {
		for( size_t var = 0; var < var_repeat; ++var ) {
			if( s == 0 ) {
				printf( "Pass %zd/%zd...\r", var, var_repeat ); fflush( stdout );
			}

			//synchronised entry
			bsp_sync();

			//start timer
			const double start = bsp_time();

			//call appropriate kernel
#ifndef SW_TRIAD
			call_kernel( mode, a, b, c, d, chunk_size, repeat );
#else
			for( size_t k = 0; k < repeat; ++k ) {
				call_kernel( mode, a, b, c, d, chunk_size, 1 );
				bsp_sync();
			}
#endif

			//end timers
			const double end = bsp_time();
#ifndef SW_TRIAD
			bsp_sync();
			const double postsync = bsp_time();
			if( end - start > postsync - start ) {
				fprintf( stderr, "Local elapsed time (%lf) is smaller than the local time elapsed with exit barrier (%lf).\n", end - start, postsync - start );
				bsp_abort( "Part of the computation has executed sequentially. Timing has become invalid. Please run the software stream benchmark.\n" );
			}
#endif
			const double elapsed_time = end - start;

			//send time to PID 0
			if( s != 0 ) {
				bsp_hpput( 0, &elapsed_time, &times, s * sizeof(double), sizeof(double) );
			} else {
				assert( times != NULL );
				      *times = elapsed_time;
			}

			//communicate time taken
			bsp_sync();

			if( s == 0 ) {
			        //PID 0 folds max end time
				timerlib_recordMax( vartimes + var, times, p );
			}
		}

		if( s == 0 ) {
			//we repeated the operation `repeat' times. Using the function kernel_bytes,
			//we can compute the total amount of moved bytes:
			const double tot_GB = ((double)(kernel_bytes( mode, chunk_size ))) * ((double)repeat) * ((double)p) / 1024.0 / 1024.0 / 1024.0;
			//convert from seconds to megabytes per second
			for( size_t i = 0; i < var_repeat; ++i ) {
				vartimes[ i ] = tot_GB / vartimes[ i ];
			}
			//derive statistics
			double max, min, avg, var;
			timerlib_deriveStats( &max, &min, &avg, &var, vartimes, var_repeat );
			printf(
				"Triad kernel: %s, local vector length %zd (%zd MB), total memory movement %zd bytes; results are as follows.\n",
				kernel_str( mode ), chunk_size, chunk_size * sizeof(float) / 1024 / 1024, kernel_bytes( mode, chunk_size )
			);
			printf(
				"Slowest run: %lf GB transferred in %lf seconds; this corresponds to %lf GB/s of effective bandwidth.\n", tot_GB, tot_GB / min, min
			);
			printf( "Full statistics: min = %lf, avg = %lf, max = %lf, stddev = %lf GB/s.\n\n", min, avg, max, sqrt(var) );
		}
	}

	//clean up
	if( s == 0 ) {
		free(       times );
		free(    vartimes );
		free( start_times );
	}
	free( a );
	free( b );
	free( c );
	free( d );

	//end SPMD section
	bsp_end();
}

