
#include "bsp.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Example problem statement. We use the repeated trapezoidal rule
 * to integrate the function 4*sqrt(1-x^2) from 0 to 1. The answer
 * should equal pi.
 */
static unsigned int precision = 10000000;
double f( const double );
void inner_loop( double * const, const double );
void sequential( void );
void parallel_half( void );
void parallel_full( void );

double f( const double x ) {
	return 4 * sqrt( 1 - x * x );
}

void inner_loop( double * const I, const double x ) {
	*I += 2 * f( x );
}
/*
 * End example problem statement.
 */

//sequential code. Uses bsp_begin/end to enable use of bsp_time
//this function contains the parallelisable for-loop.
void sequential( void ) {
	bsp_begin( 1 );

	const double h = 1.0 / ( (double)precision );
	double I = f( 0 ) + f( 1 );

	for( unsigned int i = 1; i < precision - 1; ++i ) {
		inner_loop( &I, i * h );
	}

	I /= (double)(2*precision);
	printf( "Integral is %.14lf, time taken for sequential calculation: %f\n", I, bsp_time() );

	bsp_end();
}

//parallel code. We first compute the start and end
//of the loop for each thread, then execute.
static void parallel_kernel( const bsp_pid_t P, const bsp_pid_t s ) {
	const size_t start        = s == 0 ? s + P : s;
	const size_t end          = precision - 1;
	      double partial_work = 0.0;

	//handle boundary cases for trapezoidal rule
	if( s == 0 ) {
		partial_work += f( 0 );
	}

	if( s == P - 1 ) {
		partial_work += f( 1 );
	}

	//handle inner trapezoidal rule
	const double h = 1.0 / ( (double)precision );
	for( size_t i = start; i < end; i += P ) {
		inner_loop( &partial_work, i * h );
	}
	partial_work /= (double)(2*precision);

	//prepare to communicate the partial work
	bsp_push_reg( &partial_work, sizeof(double) );

	//ensure everyone is done computing their local part
	bsp_sync();

	//we gather all results at thread 0
	if( s == 0 ) {
		//include our own partial loop result in the final answer
		double integral = partial_work;
		//for each other processor, get the result
		for( bsp_pid_t k = 1; k < bsp_nprocs(); ++k ) {
			//get their partial result and store it in place of our own partial result
			bsp_direct_get( k, &partial_work, 0, &partial_work, sizeof(double) );
			//add their partial result to the final answer
			integral += partial_work;
		}
		//report
		printf(
			"Integral is %.14lf, time taken for parallel calculation using %d threads: %f\n(time taken as measured from PID 0 after reduction)\n",
			integral, bsp_nprocs(), bsp_time()
		);
	}
}

void parallel_half( void ) {
	bsp_begin( bsp_nprocs() / 2 );

	const bsp_nprocs_t P = bsp_nprocs();
	const bsp_pid_t    s = bsp_pid();
	parallel_kernel( P, s );

	bsp_end();
}

void parallel_full( void ) {
	bsp_begin( bsp_nprocs() );

	const bsp_nprocs_t P = bsp_nprocs();
	const bsp_pid_t    s = bsp_pid();
	parallel_kernel( P, s );

	bsp_end();
}

int main( int argc, char **argv ) {

	bsp_init( &sequential, argc, argv );	
	sequential();

	if( bsp_nprocs() > 1 ) {
		bsp_init( &parallel_half, argc, argv );
		parallel_half();
	} else {
		printf( "Run over half of the available cores skipped since there is only one core available.\n" );
	}

	bsp_init( &parallel_full, argc, argv );
	parallel_full();

	return EXIT_SUCCESS;
}

