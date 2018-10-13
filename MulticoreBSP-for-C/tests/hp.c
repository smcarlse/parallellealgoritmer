/*
 * Copyright (c) 2015
 * 
 * File created April 7th 2015 by Albert-Jan N. Yzelman
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


#include "mcbsp.h"

#include <stdio.h>
#include <stdlib.h>

int main( int argc, char **argv ) {
	(void)argc;
	(void)argv;

	bsp_begin( bsp_nprocs() );

	const unsigned int P = (unsigned int)bsp_nprocs();
	const unsigned int s = (unsigned int)bsp_pid();

	unsigned char * chunk1 = (unsigned char*)malloc( P );
	bsp_push_reg( chunk1, (bsp_size_t)P );

	unsigned char random = (unsigned char)( (s * (unsigned int)rand()) % ( 3 * P ) );
	bsp_push_reg( &random, 1 );

	if( s == 0 ) {
		printf( "Testing bsp_hpget...\n" );
	}
	bsp_sync();

	if( s == 0 ) {
		for( unsigned int k = 0; k < P; ++k ) {
			bsp_hpget( (bsp_pid_t)k, &random, 0, chunk1 + k, 1 );
		}
		bsp_sync();
		printf( "PID 0 received:" );
		for( unsigned int k = 0; k < P; ++k ) {
			printf( " %d", chunk1[ k ] );
		}
		printf( ", while:\n" );
	} else {
		bsp_sync();
	}
	for( unsigned int k = 0; k < P; ++k ) {
		if( s == k ) {
			if( k == P - 1 ) {
				printf( " -PID %d locally has %d.\n", s, random );
				printf( "\nTesting hpput...\n" );
			} else {
				printf( " -PID %d locally has %d,\n", s, random );
			}
		}
		bsp_sync();
	}

	for( unsigned int k = 0; k < P; ++k ) {
		bsp_hpput( (bsp_pid_t)k, &random, chunk1, (bsp_size_t)s, 1 );
	}
	bsp_sync();
	for( unsigned int k = 0; k < P; ++k ) {
		if( s == k ) {
			printf( "PID %d received:", s );
			for( unsigned int t = 0; t < P; ++t ) {
				printf( " %d", chunk1[ t ] );
			}
			printf( ".\n" );
			if( k == P - 1 ) {
				printf( "\nPlease check manually if the above values make sense.\n" );
			}
		}
		bsp_sync();
	}

	free( chunk1 );

	bsp_end();
}

