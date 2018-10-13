
#include "bsp.h"

#include <stdio.h>
#include <stdlib.h>

void spmd1( void );
void spmd2( void );

void spmd1() {
	bsp_begin( 2 );
	printf( "Hello world from thread %d!\n", bsp_pid() );
	bsp_end();
}

void spmd2() {
	bsp_begin( bsp_nprocs() );
	printf( "Hello world from thread %d!\n", bsp_pid() );
	bsp_end();
}

int main( int argc, char **argv ) {
	printf( "Sequential part 1\n" );

	bsp_init( &spmd1, argc, argv );
	spmd1();

	printf( "Sequential part 2\n" );

	bsp_init( &spmd2, argc, argv );
	spmd2();

	printf( "Sequential part 3\n" );

	return EXIT_SUCCESS;	
}

