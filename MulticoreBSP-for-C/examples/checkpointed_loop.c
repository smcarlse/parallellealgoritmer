
#include "bsp.h"
#include "mcbsp-resiliency.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*
 * Example problem statement. We use the repeated trapezoidal rule
 * to integrate the function 4*sqrt(1-x^2) from 0 to 1. The answer
 * should equal pi.
 */
static unsigned int precision = 100000000;
double f( const double );
void inner_loop( double * const, const double );
void sequential( void );
void parallel( void );

double f( const double x ) {
	return 4 * sqrt( 1 - x * x );
}

void inner_loop( double * const I, const double x ) {
	*I += 2 * f( x );
}
/*
 * End example problem statement.
 */

//parallel code. We first compute the start and end
//of the loop for each thread, then execute.
void parallel( void ) {
	bsp_begin( bsp_nprocs() );

	const bsp_pid_t s    = bsp_pid();
	const bsp_nprocs_t P = bsp_nprocs();
	const size_t start   = s == 0 ? s + P : s;
	const size_t end     = precision - 1;
	size_t loop_counter  = 0;
	double partial_work;
	bsp_push_reg( &partial_work, sizeof( double ) );

	//start indefinite loop
	while( true ) {
		//start timer
		const double timer_start = bsp_time();

		//reset local computation
		partial_work = 0.0;

		//handle boundaries
		if( s == 0 ) {
			partial_work += f( 0 );
		}

		if( s == P - 1 ) {
			partial_work += f( 1 );
		}

		//do inner trapezoidal rule
		const double h = 1.0 / ( (double)precision );
		for( size_t i = start; i < end; i += P ) {
			inner_loop( &partial_work, i * h );
		}
		partial_work /= (double)(2*precision);

		//ensure everyone is done computing their local part
		bsp_sync();

		//we gather all results at thread 0
		if( s == 0 ) {
			//include our own partial loop result in the final answer
			double integral = partial_work;
			//for each other processor...
			for( bsp_pid_t k = 1; k < P; ++k ) {
				//get their partial result and store it in place of our own partial result
				bsp_direct_get( k, &partial_work, 0, &partial_work, sizeof(double) );
				//add their partial result to the final answer
				integral += partial_work;
			}
			//done, print result
			printf( "Iteration %zd, integral is %.14lf, time taken for parallel calculation using %d threads: %f\n", loop_counter, integral, bsp_nprocs(), bsp_time() - timer_start );
		}

		//go to next computation
		++loop_counter;
		bsp_sync();
	}

	bsp_end();
}

int main( int argc, char **argv ) {
	bsp_init( &parallel, argc, argv );
	mcbsp_set_checkpoint_frequency( 10 );
	mcbsp_set_safe_checkpoint_frequency( 3 );
	parallel();
	return EXIT_SUCCESS;
}

