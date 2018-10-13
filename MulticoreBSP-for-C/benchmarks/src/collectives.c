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


#ifndef PRIMITIVE
/**
 * Controls which primitive is begin benchmarked.
 * 
 * 0: bsp_put (default)
 * 1: bsp_hpput
 * 2: bsp_get
 * 3: bsp_hpget
 * 10: MPI_Put
 * 11: MPI_Get
 * 12: MPI collectives (MPI_Allgather, MPI_Gather, and MPI_Bcast)
 */
 #define PRIMITIVE 0
#endif

#if PRIMITIVE < 4
 #include <bsp.h>
#elif PRIMITIVE > 9
 #include <mpi.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_REP 50000

double time( void );
void   sync( void );

#if !defined(MPI_VERSION) || MPI_VERSION >= 3
 #define CONST_CORRECT
#endif

#ifdef CONST_CORRECT
double max_time( const double time, const size_t s, const size_t p );
void   kernel(   const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
void   kernel2(  const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
void   kernel3(  const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
#else
double max_time( double time, const size_t s, const size_t p );
void   kernel  ( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
void   kernel2 ( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
void   kernel3 ( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p );
#endif

static const size_t  n1 = 1ul<<13; //8kb
static const size_t  n2 = (1ul<<16) + (1ul<<15); //96kB
static const size_t  n3 = 1ul<<23; //8MB
static const size_t  n4 = 1ul<<28; //256MB

double time( void ) {
#if PRIMITIVE < 4
	return bsp_time();
#elif PRIMITIVE > 9
	return MPI_Wtime();
#endif
}

void sync( void ) {
#if PRIMITIVE < 4
	bsp_sync();
#elif PRIMITIVE > 9
	MPI_Barrier( MPI_COMM_WORLD );
#endif
}

#ifdef CONST_CORRECT
double max_time( const double time, const size_t s, const size_t p ) {
#else
double max_time( double time, const size_t s, const size_t p ) {
#endif
#if PRIMITIVE < 4
	bsp_send( 0, NULL, &time, sizeof(double) );
	double maximum = time;
	bsp_sync();
	if( s == 0 ) {
		unsigned int messages;
		double * curtime;
		void * tagpointer;
		bsp_qsize( &messages, NULL );
		if( messages != p ) {
			bsp_abort( "Not enough BSMP messages!\n" );
		}
		for( size_t k = 0 ; k < p; ++k ) {
			bsp_hpmove( &tagpointer, (void**)(&curtime) );
			if( *curtime > maximum ) {
				maximum = *curtime;
			}
		}
		return maximum;
	}
	return 0;
#elif PRIMITIVE > 9
	double maximum = 0 * s * p;
	MPI_Reduce( &time, &maximum, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD );
	return maximum;
#endif
}

//all-to-all
#ifdef CONST_CORRECT
void kernel( const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#else
void kernel( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#endif
#if PRIMITIVE == 10
	MPI_Win window;
	if( MPI_Win_create( chunk2, (MPI_Aint)(n4), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
		fprintf( stderr, "Error creating MPI Window for all-to-all using MPI_Put!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#elif PRIMITIVE == 11
	MPI_Win window;
	if( MPI_Win_create( chunk1, (MPI_Aint)(n4/p), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
		fprintf( stderr, "Error creating MPI Window for all-to-all using MPI_Get!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#endif
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group group;
	if( MPI_Comm_group( MPI_COMM_WORLD, &group ) != 0 ) {
		fprintf( stderr, "Error in group creation!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#endif
	const double t0 = time();
	for( size_t i = 0; i < rep; ++i ) {
#if PRIMITIVE > 9 && PRIMITIVE < 12
		MPI_Win_post ( group, 0, window );
		MPI_Win_start( group, 0, window );
#endif
#if PRIMITIVE == 12
		MPI_Allgather( chunk1, n, MPI_CHAR, chunk2, n, MPI_CHAR, MPI_COMM_WORLD );
#else
		for( size_t k = 0; k < p; ++k ) {
 #if PRIMITIVE == 0
			bsp_put( (bsp_pid_t)k, chunk1, chunk2, s * n, n );
 #elif PRIMITIVE == 1
			bsp_hpput( (bsp_pid_t)k, chunk1, chunk2, s * n, n );
 #elif PRIMITIVE == 2
			bsp_get( (bsp_pid_t)k, chunk1, 0, chunk2 + s * n, n );
 #elif PRIMITIVE == 3
			bsp_hpget( (bsp_pid_t)k, chunk1, 0, chunk2 + s * n, n );
 #elif PRIMITIVE == 10
			MPI_Put( chunk1, n, MPI_CHAR, (MPI_Aint)k, s * n, n, MPI_CHAR, window );
 #elif PRIMITIVE == 11
			MPI_Get( chunk2 + s*n, n, MPI_CHAR, (MPI_Aint)k, 0, n, MPI_CHAR, window );
 #endif
		}
 #if PRIMITIVE < 4 
		bsp_sync();
 #elif PRIMITIVE > 9 && PRIMITVE < 12
		MPI_Win_complete( window ); //signals completion of local DRMA requests
		MPI_Win_wait( window ); //waits on completion of all DRMA requests
 #endif
#endif
	}
	const double local_elapsed_time = time() - t0;
	const double global_elapsed_time = 1000.0 * max_time( local_elapsed_time, s, p ) / ((double)rep); //avg time in ms
	if( s == 0 ) {
		printf( "Average time taken: %lf ms. Experiments were repeated %ld times.\n\n", global_elapsed_time, rep );
	}
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group_free( &group  );
	MPI_Win_free  ( &window );
#endif
}

//all-to-one
#ifdef CONST_CORRECT
void kernel2( const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#else
void kernel2( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#endif
#if PRIMITIVE == 10
	MPI_Win window;
	if( s == 0 ) {
		if( MPI_Win_create( chunk2, (MPI_Aint)(n4), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
			fprintf( stderr, "Error creating MPI Window for all-to-one using MPI_Put!\n" );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
	} else {
		if( MPI_Win_create( NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
			fprintf( stderr, "Error creating MPI Window for all-to-one using MPI_Put!\n" );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
	}
#elif PRIMITIVE == 11
	MPI_Win window;
	if( MPI_Win_create( chunk1, (MPI_Aint)(n4/p), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
		fprintf( stderr, "Error creating MPI Window for all-to-one using MPI_Get!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#endif
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group group;
	if( MPI_Comm_group( MPI_COMM_WORLD, &group ) != 0 ) {
		fprintf( stderr, "Error in group creation!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#ifdef CONST_CORRECT
	const int root_pid[ 1 ] = {0};
#else
	int root_pid[ 1 ] = {0};
#endif
	MPI_Group root;
	if( MPI_Group_incl( group, 1, root_pid, &root ) != 0 ) {
		fprintf( stderr, "Error in root `group' creation!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#endif
	const double t0 = time();
	for( size_t i = 0; i < rep; ++i ) {
#if PRIMITIVE == 0
		bsp_put( 0, chunk1, chunk2, s * n, n );
		bsp_sync();
#elif PRIMITIVE == 1
		bsp_hpput( 0, chunk1, chunk2, s * n, n );
		bsp_sync();
#elif PRIMITIVE == 2
		for( bsp_pid_t k = 0; s == 0 && k < p; ++k ) {
			bsp_get( k, chunk1, 0, chunk2 + s*n, n );
		}
		bsp_sync();
#elif PRIMITIVE == 3
		for( bsp_pid_t k = 0; s == 0 && k < p; ++k ) {
			bsp_hpget( k, chunk1, 0, chunk2 + s*n, n );
		}
		bsp_sync();
#elif PRIMITIVE == 10
		if( s == 0 ) {
			if( MPI_Win_post( group, 0, window ) != 0 ) {
				fprintf( stderr, "Error in MPI_Win_post!\n" );
				MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
			}
		}
		if( MPI_Win_start( root, 0, window ) != 0 ) {
			fprintf( stderr, "Error in MPI_Win_start!\n" );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
		MPI_Put( chunk1, n, MPI_CHAR, 0, s * n, n, MPI_CHAR, window );
		MPI_Win_complete( window );
		if( s == 0 ) {
			MPI_Win_wait( window );
		}
#elif PRIMITIVE == 11
		MPI_Win_post( root, MPI_MODE_NOPUT, window );
		if( s == 0 ) {
			MPI_Win_start( group, 0, window );
			for( size_t k = 0; k < p; ++k ) {
				MPI_Get( chunk2 + s*n, n, MPI_CHAR, k, 0, n, MPI_CHAR, window );
			}
			MPI_Win_complete( window );
		}
		MPI_Win_wait( window );
#elif PRIMITIVE == 12
		MPI_Gather( chunk1, n, MPI_CHAR, chunk2, n, MPI_CHAR, 0, MPI_COMM_WORLD );
#endif
	}
	const double local_elapsed_time = time() - t0;
	const double global_elapsed_time = 1000.0 * max_time( local_elapsed_time, s, p ) / ((double)rep); //avg time in ms
	if( s == 0 ) {
		printf( "Average time taken: %lf ms. Experiments were repeated %ld times.\n\n", global_elapsed_time, rep );
	}
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group_free( &group  );
	MPI_Group_free( &root   );
	MPI_Win_free  ( &window );
#endif
}

//one-to-all
#ifdef CONST_CORRECT
void kernel3( const char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#else
void kernel3( char * const chunk1, char * const chunk2, const size_t rep, const size_t n, const size_t s, const size_t p ) {
#endif
#if PRIMITIVE == 10
	MPI_Win window;
	if( MPI_Win_create( chunk2, (MPI_Aint)(n4)/p, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
		fprintf( stderr, "Error creating MPI Window for one-to-all using MPI_Put!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#elif PRIMITIVE == 11
	MPI_Win window;
	if( s == 0 ) {
		if( MPI_Win_create( (void * const)chunk1, (MPI_Aint)(n4/p), sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
			fprintf( stderr, "Error creating MPI Window for one-to-all using MPI_Get!\n" );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
	} else {
		if( MPI_Win_create( NULL, 0, sizeof(char), MPI_INFO_NULL, MPI_COMM_WORLD, &window ) != 0 ) {
			fprintf( stderr, "Error creating dummy MPI Window for one-to-all using MPI_Get!\n" );
			MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
		}
	}
#endif
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group group;
	if( MPI_Comm_group( MPI_COMM_WORLD, &group ) != 0 ) {
		fprintf( stderr, "Error in group creation!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
	const int root_pid[ 1 ] = {0};
	MPI_Group root;
	if( MPI_Group_incl( group, 1, (int*)root_pid, &root ) != 0 ) {
		fprintf( stderr, "Error in root `group' creation!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#endif
	const double t0 = time();
	for( size_t i = 0; i < rep; ++i ) {
#if PRIMITIVE == 2
		bsp_get( 0, chunk1, 0, chunk2, n );
#elif PRIMITIVE == 3
		bsp_hpget( 0, chunk1, 0, chunk2, n );
#elif PRIMITIVE == 0
		for( size_t k = 0; s == 0 && k < p; ++k ) {
			bsp_put( (bsp_pid_t)k, chunk1, chunk2, 0, n );
		}
#elif PRIMITIVE == 1
		for( size_t k = 0; s == 0 && k < p; ++k ) {
			bsp_hpput( (bsp_pid_t)k, chunk1, chunk2, 0, n );
		}
#elif PRIMITIVE == 10
		MPI_Win_post( root, 0, window );
		if( s == 0 ) {
			MPI_Win_start( group, 0, window );
			for( size_t k = 0; k < p; ++k ) {
				MPI_Put( chunk1, n, MPI_CHAR, (int)k, 0, n, MPI_CHAR, window );
			}
			MPI_Win_complete( window );
		}
		MPI_Win_wait( window );
#elif PRIMITIVE == 11
		MPI_Win_post( group, MPI_MODE_NOPUT, window );
		MPI_Win_start( group, 0, window );
		MPI_Get( chunk2, n, MPI_CHAR, 0, 0, n, MPI_CHAR, window );
		MPI_Win_complete( window );
		MPI_Win_wait( window );
#elif PRIMITIVE == 12
		//expand some effort to make Bcast broadcast into a different memory region
		//including at the process root.
		if( s == 0 ) {
			memcpy( chunk2, chunk1, n );
		}
		MPI_Bcast( chunk2, n, MPI_CHAR, 0, MPI_COMM_WORLD );
#endif
#if PRIMITIVE < 4
		bsp_sync();
#endif
	}
	const double local_elapsed_time = time() - t0;
	const double global_elapsed_time = 1000.0 * max_time( local_elapsed_time, s, p ) / ((double)rep); //avg time in ms
	if( s == 0 ) {
		printf( "Average time taken: %lf ms. Experiments were repeated %ld times.\n\n", global_elapsed_time, rep );
	}
#if PRIMITIVE > 9 && PRIMITIVE < 12
	MPI_Group_free( &group  );
	MPI_Group_free( &root   );
	MPI_Win_free  ( &window );
#endif
}

#if PRIMITIVE > 9
int main( int argc, char** argv ) {
	MPI_Init( &argc, &argv );
	int p, s;
	MPI_Comm_rank( MPI_COMM_WORLD, &s );
	MPI_Comm_size( MPI_COMM_WORLD, &p );
 #if PRIMITIVE == 10
	const char method[] = "MPI_Put";
 #elif PRIMITIVE == 11
	const char method[] = "MPI_Get";
 #elif PRIMITIVE == 12
	const char method[] = "MPI collectives";
 #endif
#elif PRIMITIVE < 4
int main() {
	bsp_begin( bsp_nprocs() );
	const size_t p = bsp_nprocs();
	const size_t s = bsp_pid();
 #if PRIMITIVE == 0
	const char method[] = "bsp_put";
 #elif PRIMITIVE == 1
	const char method[] = "bsp_hpput";
 #elif PRIMITIVE == 2
	const char method[] = "bsp_get";
 #elif PRIMITIVE == 3
	const char method[] = "bsp_hpget";
 #endif
	size_t old_tagsize = 0;
	bsp_set_tagsize( &old_tagsize );
#endif

	size_t rep = INITIAL_REP;

	if( s == 0 ) {
		printf( "This application will benchmark successive collective calls using %s. Results are averaged over multiple runs. Time taken for synchronised entry and exit of the communication phases are included in the timings. This SPMD section counts %ld BSP processs.\n\n", method, (unsigned long int)p );
	}

#if PRIMITIVE > 9 && PRIMITIVE < 13
	void * chunk1, * chunk2;
	if( MPI_Alloc_mem( (MPI_Aint)(n4/p), MPI_INFO_NULL, &chunk1 ) || MPI_Alloc_mem( (MPI_Aint)n4, MPI_INFO_NULL, &chunk2 ) ) {
		fprintf( stderr, "Could not allocate memory!\n" );
		MPI_Abort( MPI_COMM_WORLD, EXIT_FAILURE );
	}
#elif PRIMITIVE < 4
	//get chunks of memory
	void * const chunk1 = malloc( n4 / p );
	void * const chunk2 = malloc(   n4   );
	if( chunk1 == NULL || chunk2 == NULL ) {
		bsp_abort( "Could not allocate memory!" );
	}
#endif
#if PRIMITIVE < 2
	bsp_push_reg( chunk2, n4 );
#elif PRIMITIVE < 4
	bsp_push_reg( chunk1, n4 / p );
#endif
	//L1 expirement
	if( s == 0 ) {
		printf( "All-to-all %s benchmark, each process sends %ld bytes of data to all other BSP processes (should hit L1).\n", method, n1/p );
		fflush( stdout );
	}
	sync();
	kernel( chunk1, chunk2, rep, n1/p, s, p );

	//L2 expirement
	if( s == 0 ) {
		printf( "All-to-all %s benchmark, each process sends %ld bytes of data to all other BSP processes (should hit L2).\n", method, n2/p );
		fflush( stdout );
	}
	sync();
	kernel( chunk1, chunk2, rep/10, n2/p, s, p );

	//L3 expirement
	if( s == 0 ) {
		printf( "All-to-all %s benchmark, each process sends %ld bytes of data to all other BSP processes (should hit L3).\n", method, n3/p );
		fflush( stdout );
	}
	sync();
	kernel( chunk1, chunk2, rep/100, n3/p, s, p );

	//RAM expirement
	if( s == 0 ) {
		printf( "All-to-all %s benchmark, each process sends %ld bytes of data to all other BSP processes (should hit RAM).\n", method, n4/p );
		fflush( stdout );
	}
	sync();
	kernel( chunk1, chunk2, rep/1000, n4/p, s, p );

	//L1 expirement
	if( s == 0 ) {
		printf( "\nAll-to-one %s benchmark, %ld bytes of data sent per BSP process (should hit L1).\n", method, n1/p );
		fflush( stdout );
	}
	sync();
	kernel2( chunk1, chunk2, rep, n1/p, s, p );

	//L2 expirement
	if( s == 0 ) {
		printf( "All-to-one %s benchmark, %ld bytes of data sent per BSP process (should hit L2).\n", method, n2/p );
		fflush( stdout );
	}
	sync();
	kernel2( chunk1, chunk2, rep/10, n2/p, s, p );

	//L3 expirement
	if( s == 0 ) {
		printf( "All-to-one %s benchmark, %ld bytes of data sent per BSP process (should hit L3).\n", method, n3/p );
		fflush( stdout );
	}
	sync();
	kernel2( chunk1, chunk2, rep/100, n3/p, s, p );

	//RAM expirement
	if( s == 0 ) {
		printf( "All-to-one %s benchmark, %ld bytes of data sent per BSP process (should hit RAM).\n", method, n4/p );
		fflush( stdout );
	}
	sync();
	kernel2( chunk1, chunk2, rep/1000, n4/p, s, p );

	//L1 expirement
	if( s == 0 ) {
		printf( "\nOne-to-all %s benchmark, PID 0 sends %ld bytes of data to each other BSP process (should hit L1).\n", method, n1/p );
		fflush( stdout );
	}
	sync();
	kernel3( chunk1, chunk2, rep, n1/p, s, p );

	//L2 expirement
	if( s == 0 ) {
		printf( "One-to-all %s benchmark, PID 0 sends %ld bytes of data to each other BSP process (should hit L2).\n", method, n2/p );
		fflush( stdout );
	}
	sync();
	kernel3( chunk1, chunk2, rep/10, n2/p, s, p );

	//L3 expirement
	if( s == 0 ) {
		printf( "One-to-all %s benchmark, PID 0 sends %ld bytes of data to each other BSP process (should hit L3).\n", method, n3/p );
		fflush( stdout );
	}
	sync();
	kernel3( chunk1, chunk2, rep/100, n3/p, s, p );

	//RAM expirement
	if( s == 0 ) {
		printf( "One-to-all %s benchmark, PID 0 sends %ld bytes of data to each other BSP process (should hit RAM).\n", method, n4/p );
		fflush( stdout );
	}
	sync();
	kernel3( chunk1, chunk2, rep/1000, n4/p, s, p );

	if( s== 0 ) {
		printf( "Cleaning up...\n" );
		fflush( stdout );
	}

#if PRIMITIVE < 4
	//free chunks
	free( chunk1 );
	free( chunk2 );
	bsp_end();
#elif PRIMITIVE > 9
	MPI_Free_mem( chunk1 );
	MPI_Free_mem( chunk2 );
	MPI_Finalize();
#endif
}

