
#include "bsp.h"
#include "mcbsp-resiliency.h"

#include <stdio.h>
#include <stdlib.h>

//parallel code. We first compute the start and end
//of the loop for each thread, then execute.
void parallel( void ) {
	bsp_begin( bsp_nprocs() );

	//retrieve SPMD info
	const unsigned int s = bsp_pid();
	const unsigned int P = bsp_nprocs();

	//allocate buffer
	unsigned char * const buffer = (unsigned char*) malloc( P * sizeof(unsigned char) );

	//register buffer and sync
	bsp_push_reg( buffer, P * sizeof(unsigned char) );
	bsp_sync();

	//construct random message in 0..3P-1
	const unsigned char msg = rand() % (3*P);

	//call checkpoint
	mcbsp_checkpoint();

	//report my message
	for( unsigned int k = 0; k < P; ++k ) {
		if( k == s ) {
			printf( "PID %u selected message %u\n", s, ((unsigned int)msg) );
		}
		bsp_sync();
	}

	//request all-to-all communication
	for( unsigned int i = 0; i < P; ++i ) {
		bsp_put( i, &msg, buffer, s * sizeof(unsigned char), sizeof(unsigned char) );
	}

	//call checkpoint
	mcbsp_checkpoint();

	//report communication was buffered and is now to be executed
	if( s == 0 ) {
		printf( "SPMD is about to execute buffered communications...\n" );
	}

	//execute
	bsp_sync();

	//report
	for( unsigned int k = 0; k < P; ++k ) {
		if( k == s ) {
			printf( "PID %u: buffer contents are %u", s, ((unsigned int)buffer[0]) );
			for( unsigned int i = 1; i < P; ++i ) {
				printf( ", %u", ((unsigned int)buffer[i]) );
			}
			printf( "\n" );
		}
		bsp_sync();
	}

	//done
	bsp_end();
}

int main( int argc, char **argv ) {
	bsp_init( &parallel, argc, argv );
	parallel();
	return EXIT_SUCCESS;
}

