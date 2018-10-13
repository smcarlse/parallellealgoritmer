/*
 * Copyright (c) 2012
 *
 * File created by A. N. Yzelman, 2007-2012.
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

//for documentation see the .h file:
#include "mcbsp.h"

void MCBSP_FUNCTION_PREFIX(begin)( const bsp_pid_t P ) {

	//check if keys are allocated
	mcbsp_internal_check_keys_allocated();

	//get necessary data
	struct mcbsp_init_data * init = pthread_getspecific( mcbsp_internal_init_data );

	//if the check did not return an init struct, we are a
	//spawned thread and should just continue the SPMD
	//code, or we come directly from main
	if( init == NULL ) {
		//check if we are in the latter case, and we are PID 0
		const struct mcbsp_thread_data * const thread_data = pthread_getspecific( mcbsp_internal_thread_data );
		if( thread_data == NULL ) {
			//construct an implied init
			init = mcbsp_util_malloc( sizeof( struct mcbsp_init_data ), "bsp_begin (MulticoreBSP for C) implied SPMD init struct" );
			if( init == NULL ) {
				fprintf( stderr, "Could not perform an implicit initialisation!\n" );
				mcbsp_util_fatal();
			}
			init->spmd = NULL; //we want to call main, but (*void)(void) does not match its profile
			init->bsp_program = NULL;
			init->argc = 0;
			init->argv = NULL;
#ifdef MCBSP_WITH_ACCELERATOR
			mcbsp_accelerator_implied_init( init, P );
#endif
			if( pthread_setspecific( mcbsp_internal_init_data, init ) != 0 ) {
				fprintf( stderr, "Error: could not set BSP program key in implied bsp_init!\n" );
				mcbsp_util_fatal();
			}
		} else {
			//in the else case:
			//  we have thread-local data, so we are not the first to enter bsp_begin;
			//  we are sibling SPMD processes

#ifdef MCBSP_ENABLE_HP_DIRECTIVES
			//barrier required, since HP directives assume allocated thread-local comm. queues
			//which need to be allocated per-thread first
			const struct mcbsp_thread_data * const data = mcbsp_internal_const_prefunction();

 #ifdef MCBSP_USE_SPINLOCK

			mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
 #else
			mcbsp_internal_sync( data->init, &(data->init->condition) );
 #endif
#endif
			//as sibling processes, our job ends here
			return;
		}
	}

	//otherwise we need to start the SPMD code 
	init->threads = mcbsp_util_malloc( ((size_t)P) * sizeof( pthread_t ), "bsp_begin (MulticoreBSP for C) pthread_t array" );
	if( init->threads == NULL ) {
		fprintf( stderr, "Could not allocate new threads!\n" );
		mcbsp_util_fatal();
	}

	pthread_attr_t attr;

	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
#if defined _WIN32
	DWORD_PTR mask;
#elif !defined __MACH__
	cpu_set_t mask;
#endif

	//further initialise init object
	init->P                   = ((size_t)P);
	init->abort               = false;
	init->ended               = false;
	init->no_cp               = false;
	init->safe_cp             = false;
	init->cp_f                = machine->cp_f;
	init->safe_cp_f           = machine->safe_cp_f;
	init->skipped_checkpoints = 0;
	init->current_superstep   = 0;
	init->sync_entry_counter  = 0;
	init->sync_exit_counter   = 0;
	pthread_mutex_init( &(init->mutex), NULL );
	init->    sl_condition = mcbsp_util_malloc( ((size_t)P) * sizeof( unsigned char ), "bsp_begin (MulticoreBSP for C) default spinlocking array" );
	init->sl_mid_condition = mcbsp_util_malloc( ((size_t)P) * sizeof( unsigned char ), "bsp_begin (MulticoreBSP for C) middle spinlocking array" );
	init->sl_end_condition = mcbsp_util_malloc( ((size_t)P) * sizeof( unsigned char ), "bsp_begin (MulticoreBSP for C) end spinlocking array" );
	for( size_t s = 0; s < ((size_t)P); ++s ) {
		init->sl_condition[ s ] = init->sl_mid_condition[ s ] = init->sl_end_condition[ s ] = 0;
	}
	pthread_cond_init( &(init->    condition), NULL );
	pthread_cond_init( &(init->mid_condition), NULL );
	pthread_cond_init( &(init->end_condition), NULL );
	mcbsp_util_address_table_initialise( &(init->global2local), ((size_t)P) );
	init->threadData = mcbsp_util_malloc( ((size_t)P) * sizeof( struct mcbsp_thread_data * ), "bsp_begin (MulticoreBSP for C) thread-local data array" );
	init->prev_data  = pthread_getspecific( mcbsp_internal_thread_data );
	if( init->prev_data != NULL ) {
		//we are coming from a top-level BSP run, whose context is saved
		//also store explicitly the parent process ID
		init->top_pid = init->prev_data->bsp_id;
	} else {
		//note we had no parent ID
		init->top_pid = SIZE_MAX;
	}
	init->tagSize = 0;

	//derive pinning
	struct mcbsp_util_pinning_info pinning_info = mcbsp_util_pinning( ((size_t)P), mcbsp_internal_getMachineInfo(),
		init->prev_data == NULL ?
		NULL :
		&(init->prev_data->machine_partition) );
	size_t *pinning = pinning_info.pinning;

	//sanity checks
	if( pinning == NULL ) {
		fprintf( stderr, "Could not get a valid pinning!\n" );
		mcbsp_util_fatal();
	}

#ifdef MCBSP_SHOW_PINNING
	//report pinning:
	fprintf( stdout, "Info: pinning used is" );
	for( size_t s=0; s < ((size_t)P); ++s ) {
		fprintf( stdout, " %lu", ((unsigned long int)pinning[ s ]) );
	}
	fprintf( stdout, "\n" );
#endif

	//spawn P-1 threads.
	for( size_t s = ((size_t)P) - 1; s < ((size_t)P); --s ) {
		//get new thread_data object, with initialised plain old data
		struct mcbsp_thread_data *thread_data = mcbsp_internal_allocate_thread_data( init, s );
		//provide the submachine of this thread (note this is also plain old data in thread_data)
		if( pinning_info.partition.top == 0 ) { //manual partitioning; no submachines available. Derive a simple one.
			thread_data->machine_partition.start = thread_data->machine_partition.end = pinning_info.pinning[ s ];
			thread_data->machine_partition.stride = 1;
		} else { //get submachines from the pinning info
			const struct mcbsp_util_machine_partition * const partitions_array = (struct mcbsp_util_machine_partition*)(pinning_info.partition.array);
			thread_data->machine_partition = partitions_array[ s ];
		}

		//spawn new threads if s>0
		if( s > 0 ) {
			//create POSIX threads attributes
			//(currently for pinning, if supported)
			pthread_attr_init( &attr );

#ifdef _WIN32
			mask = (DWORD_PTR)1;
			mask <<= pinning[ s ];
#elif !defined __MACH__
			CPU_ZERO( &mask );
			CPU_SET ( pinning[ s ], &mask );
			pthread_attr_setaffinity_np( &attr, sizeof( cpu_set_t ), &mask );
#endif

			//spawn the actual thread
			const int pthr_rval = pthread_create( &(init->threads[ s ]), &attr, mcbsp_internal_spmd, thread_data );
			if( pthr_rval != 0 ) {
				fprintf( stderr, "Could not spawn new thread (%s)!\n", strerror( pthr_rval ) );
				mcbsp_util_fatal();
			}

#ifdef _WIN32
			//do after-creation pinning
			SetThreadAffinityMask( pthread_getw32threadhandle_np( init->threads[ s ] ), mask );
#elif defined __MACH__
			//set after-creation affinities
			thread_port_t osx_thread = pthread_mach_thread_np( init->threads[ s ] );
			struct thread_affinity_policy ap;
			switch( machine->affinity ) {
				case SCATTER:
				{
					//Affinity API release notes do not specify whether 0 is a valid tag, or in fact equal to NULL; so 1-based to be sure
					ap.affinity_tag = (int) (s + 1);
					break;
				}
				case COMPACT:
				{
					ap.affinity_tag = 1;
					break;
				}
				case MANUAL:
				{
					ap.affinity_tag = (int) (machine->manual_affinity[ s ]);
					break;
				}
				default:
				{
					fprintf( stderr, "Unhandled affinity type for Mac OS X!\n" );
					mcbsp_util_fatal();
				}
			}
			thread_policy_set( osx_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&ap, THREAD_AFFINITY_POLICY_COUNT );
#endif

			//destroy attributes object
			pthread_attr_destroy( &attr );
		} else {
			//continue ourselves as bsp_id 0.
			//All parts have been handed out; reset partition stack.
			pinning_info.partition.top = 0;

			//Do pinning for master thread
#ifdef __MACH__
			thread_port_t osx_thread = pthread_mach_thread_np( pthread_self() );
			struct thread_affinity_policy ap;
			if( machine->affinity == SCATTER || machine->affinity == COMPACT ) {
				//NOTE: see the above comment on 1-based numbering
				ap.affinity_tag = 1;
			} else if( machine->affinity == MANUAL ) {
				ap.affinity_tag = (int) (machine->manual_affinity[ s ]);
			} else {
				fprintf( stderr, "Unhandled affinity type for Mac OS X!\n" );
				mcbsp_util_fatal();
			}
			thread_policy_set( osx_thread, THREAD_AFFINITY_POLICY, (thread_policy_t)&ap, THREAD_AFFINITY_POLICY_COUNT );
#elif defined _WIN32
			DWORD_PTR mask = 1;
			mask <<= pinning[ s ];
			SetThreadAffinityMask( GetCurrentThread(), mask );
#elif !defined __MACH__
			CPU_ZERO( &mask );
			CPU_SET ( pinning[ s ], &mask );
			if( pthread_setaffinity_np( pthread_self(), sizeof( cpu_set_t ), &mask ) != 0 ) {
				fprintf( stderr, "Could not pin master thread to requested hardware thread (%lu)!\n", (unsigned long int)(pinning[ s ]) );
				mcbsp_util_fatal();
			}
#endif
			//record our own descriptor
			init->threads[ 0 ] = pthread_self();
			//copy part of mcbsp_internal_spmd.
			thread_data = mcbsp_internal_initialise_thread_data( thread_data );
			init->threadData[ 0 ] = thread_data;
			const int rc = pthread_setspecific( mcbsp_internal_thread_data, thread_data );
			if( rc != 0 ) {
				fprintf( stderr, "Could not store thread-local data in continuator thread!\n" );
				fprintf( stderr, "(%s)\n", strerror( rc ) );
				mcbsp_util_fatal();
			}

			//set start time
#ifdef __MACH__
			//get rights for accessing Mach's timers
			const kern_return_t rc1 = host_get_clock_service( mach_host_self(), SYSTEM_CLOCK, &(thread_data->clock) );
			if( rc1 != KERN_SUCCESS ) {
				fprintf( stderr, "Could not access the Mach system timer (%s)\n", mach_error_string( rc1 ) );
				mcbsp_util_fatal();
			}
			const kern_return_t rc2 = clock_get_time( thread_data->clock, &(thread_data->start) );
			if( rc2 != KERN_SUCCESS ) {
				fprintf( stderr, "Could not get starting time (%s)\n", mach_error_string( rc2 ) );
				mcbsp_util_fatal();
			}
#elif defined _WIN32
			QueryPerformanceCounter( &(thread_data->start) );
			QueryPerformanceFrequency( &(thread_data->frequency) );
#else
			clock_gettime( CLOCK_MONOTONIC, &(thread_data->start) );
#endif
			//this enables possible BSP-within-BSP execution.
			if( pthread_setspecific( mcbsp_internal_init_data, NULL ) != 0 ) {
				fprintf( stderr, "Could not reset initialisation data to NULL on SPMD start!\n" );
				mcbsp_util_fatal();
			}

			//record start time of the coming compute phase
			thread_data->superstep_start_time = mcbsp_internal_time( thread_data );
			mcbsp_internal_init_superstep_stats( &(thread_data->superstep_stats), 0 );
		}
	}
	//free pinning_info
	free( pinning );
	mcbsp_util_stack_destroy( &(pinning_info.partition) );
#ifdef MCBSP_ENABLE_HP_DIRECTIVES
	//required, since HP directives assume allocated thread-local comm. queues
	//so do a synchronised *entry* of the SPMD region (other PIDs enter this
	//sync at the top of this function, this here is for PID 0 only!)
#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( init, init->sl_condition, (size_t)0 );
#else
	mcbsp_internal_sync( init, &(init->condition) );
#endif
#endif
}

void MCBSP_FUNCTION_PREFIX(end)( void ) {
	//get thread-local data
	struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );
	if( data == NULL ) {
		fprintf( stderr, "Error: could not get thread-local data in call to bsp_abort( error_message )!\n" );
		mcbsp_util_fatal();
	}

	//record end
	data->init->ended = true;

#if MCBSP_MODE == 3
	//end phase timing
	strncpy( data->superstep_stats.name, "Last computation phase", 255 );
	data->superstep_stats.computation = mcbsp_internal_time( data ) - data->superstep_start_time;
	data->superstep_stats.communication = 0;
	//push current profile data
	mcbsp_util_stack_push( &(data->profile), &(data->superstep_stats) );
#endif

#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
#else
	mcbsp_internal_sync( data->init, &(data->init->condition) );
#endif

	//print profiling data (if any). This is a sequential process.
	if( !mcbsp_util_stack_empty( &(data->profile) ) ) {
		//do profiling
		if( data->bsp_id == 0 ) {
			mcbsp_internal_print_profile( data->init );
		}
	} else {
		//sanity checks
		for( size_t k = 1; k < data->init->P; ++k ) {
			assert( mcbsp_util_stack_empty( &(data->init->threadData[ k ]->profile) ) );
		}
	}

	//wait with destroying local data (in the below) until after profiling has completed,
	//or until debugging (through the above assert) has completed.
#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
#else
	mcbsp_internal_sync( data->init, &(data->init->condition) );
#endif

	//set thread-local data to NULL
	if( pthread_setspecific( mcbsp_internal_thread_data, NULL ) != 0 ) {
		fprintf( stderr, "Could not set thread-local data to NULL on thread exit.\n" );
		mcbsp_util_fatal();
	}

	//free data and exit if not master thread
	if( data->bsp_id != 0 ) {
		//free thread-local data
		mcbsp_internal_destroy_thread_data( data );
		//exit
		pthread_exit( NULL );
	}

	//master thread cleans up init struct
	struct mcbsp_init_data *init = data->init;

	//that's everything we needed from the thread-local data struct
	mcbsp_internal_destroy_thread_data( data );

	//wait for other threads
	for( size_t s = 1; s < init->P; ++s ) {
		pthread_join( init->threads[ s ], NULL );
	}

	//destroy mutex and conditions
	pthread_mutex_destroy( &(init->mutex) );
#ifdef MCBSP_USE_SPINLOCK
	free( init->    sl_condition );
	free( init->sl_mid_condition );
	free( init->sl_end_condition );
#else
	pthread_cond_destroy( &(init->    condition) );
	pthread_cond_destroy( &(init->mid_condition) );
#endif

	//destroy global address table
	mcbsp_util_address_table_destroy( &(init->global2local) );

	//destroy pointers to thread-local data structs
	free( init->threadData );

	//free threads array
	free( init->threads );

	//reset thread-local data, if applicable (hierarchical execution)
	const int rc = pthread_setspecific( mcbsp_internal_thread_data, init->prev_data );

#ifndef MCBSP_NO_CHECKS
	if( rc != 0 ) {
		fprintf( stderr, "Could not restore old thread data, or could not erase local thread data!\n" );
		fprintf( stderr, "(%s)\n", strerror( rc ) );
		mcbsp_util_fatal();
	}
#endif
	
	//exit gracefully, free BSP program init data
	free( init );
}

void MCBSP_FUNCTION_PREFIX(init)( void (*spmd)(void), int argc, char **argv ) {
	//create a BSP-program specific initial data struct
	struct mcbsp_init_data *initialisationData = mcbsp_util_malloc( sizeof( struct mcbsp_init_data ), "bsp_init (MulticoreBSP for C) SPMD init struct" );
	if( initialisationData == NULL ) {
		fprintf( stderr, "Error: could not allocate MulticoreBSP initialisation struct!\n" );
		mcbsp_util_fatal();
	}
	//set values
	initialisationData->spmd 	= spmd;
	initialisationData->bsp_program = NULL;
	initialisationData->argc 	= argc;
	initialisationData->argv 	= argv;
#ifdef MCBSP_WITH_ACCELERATOR
	mcbsp_accelerator_full_init( initialisationData, argc, argv );
#endif
	//continue initialisation
	bsp_init_internal( initialisationData );
}

bsp_pid_t MCBSP_FUNCTION_PREFIX(pid)( void ) {
#if MCBSP_MODE == 3
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();
	++(data->superstep_stats.non_communicating);
#else
	const struct mcbsp_thread_data * const data = mcbsp_internal_const_prefunction();
#endif
	return (bsp_pid_t)(data->bsp_id);
}

bsp_pid_t MCBSP_FUNCTION_PREFIX(nprocs)( void ) {
	//since bsp_nprocs can be called from userspace without even a preceding bsp_init,
	//we must always check whether the BSP run-time is sufficiently-well initialised.
	mcbsp_internal_check_keys_allocated();
	struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );
	//check if we were called from outside an SPMD environment
	if( data == NULL ) {
		//yes; first check for architecture-specific max settings
#ifdef MCBSP_WITH_ACCELERATOR
		//see if we got passed a maximum via bsp_init
		const struct mcbsp_init_data * const global_init = pthread_getspecific( mcbsp_internal_init_data );
		if( global_init == NULL ) {
			//no maximum passed, return max
			return mcbsp_accelerator_offline_nprocs();
		} else {
			//maximum should have been passed, return it
			return global_init->P;
		}
#else
		//return machineInfo data
		const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
		if( machine->num_reserved_cores >= machine->cores ) {
			return 0;
		} else {
			return (bsp_pid_t)(machine->threads);
		}
#endif
	} else {
#if MCBSP_MODE == 3
		++(data->superstep_stats.non_communicating);
#endif
		//check if the BSP execution was aborted
		mcbsp_internal_check_aborted();
		//return nprocs involved in this SPMD run
		return ((bsp_pid_t)data->init->P);
	}
}

void MCBSP_FUNCTION_PREFIX(abort)( const char * const error_message, ... ) {
	//get variable arguments struct
	va_list args;
	va_start( args, error_message );

	//pass to bsp_vabort
	MCBSP_FUNCTION_PREFIX(vabort)( error_message, args );

	//mark end of variable arguments
	va_end( args );
}

void MCBSP_FUNCTION_PREFIX(vabort)( const char * const error_message, va_list args ) {

	//print error message, if we have one
	if( error_message != NULL ) {
		vfprintf( stderr, error_message, args );
	}
	
	//get thread-local data and check for errors
	const struct mcbsp_thread_data * const data = mcbsp_internal_const_prefunction();

	//send signal to all sibling threads
	data->init->abort = true;

	//flush error message
	fflush( stderr );

	//when in debug mode, raise assertion failure for easy backtracking
	assert( false );

	//if there are threads in sync, wake them up
	//first get lock, otherwise threads may sync
	//while checking for synched threads.
	//Only regular condition are checked, as aborts
	//cannot occur within bsp_begin or bsp_sync.
#ifndef MCBSP_USE_SPINLOCK //in spinlock-loop abort-condition is busy-checked
	pthread_mutex_lock( &(data->init->mutex) );
	if( data->init->sync_entry_counter > 0 )
		pthread_cond_broadcast( &(data->init->condition) );
	pthread_mutex_unlock( &( data->init->mutex) );
#else
	//in spinlock-loop abort-condition is busy-checked; no code necessary
#endif
	
	//quit execution
	pthread_exit( NULL );
}

void MCBSP_FUNCTION_PREFIX(sync)( void ) {

	//get local data
	struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );
	
#if MCBSP_MODE == 3
	const double comm_start = mcbsp_internal_time( data );
	data->superstep_stats.computation = comm_start - data->superstep_start_time;
#endif

	//clear local BSMP queue
	data->bsmp.top = 0;

	//update superstep counts
	if( data->bsp_id == 0 ) {
		++(data->init->current_superstep);
#ifdef MCBSP_WITH_DMTCP
		mcbsp_internal_update_checkpointStats( data->init );
#endif
	}

	//sanity checks
	assert( data != NULL );
	assert( data->init != NULL );
	assert( data->init->threadData != NULL );
	assert( data->bsp_id < data->init->P );
	assert( data->init->threadData[ data->bsp_id ] != NULL );

	//if no cache coherency, handle here:
	//
	//we are done with our superstep, start writing back all our modified cache lines,
	//and stop using possibly stale data from remote processes (as for instance gotten
	//from the communication phase of the last superstep).
	mcbsp_nocc_purge_all();

	//get our hp queue
	struct mcbsp_util_stack * const hpdrmaqueue = &(data->init->threadData[ data->bsp_id ]->hpdrma_queue);

	//invalidate local copies of this queue data
	mcbsp_nocc_invalidate( hpdrmaqueue, sizeof(struct mcbsp_util_stack) );

	//handle each message in this queue
	while( !mcbsp_util_stack_empty( hpdrmaqueue ) ) {

		//invalidate any local copies of this pop request
		mcbsp_nocc_invalidate(
			mcbsp_util_stack_peek( hpdrmaqueue ),
			sizeof(struct mcbsp_hp_request)
		);

		//pop hp request
		const struct mcbsp_hp_request request =
			* (struct mcbsp_hp_request *) mcbsp_util_stack_pop( hpdrmaqueue );

		//invalidate any local copies of remote memory
		mcbsp_nocc_invalidate( request.source, request.length );

		//sanity checks
		assert( request.destination != NULL );
		assert( request.source      != NULL );
		assert( request.length      != 0    );
#ifndef NDEBUG
		//execute HP communication, first check for overlap
		if(
			( (      (char *)request.destination) + request.length <= ((const char *)request.source)      ) ||
			( ((const char *)request.source)      + request.length <= (      (char *)request.destination) )
		) {
			//no overlap, do copy
			mcbsp_util_memcpy( request.destination, request.source, request.length );
		} else {
			//there is overlap, warn because this is undefined behaviour
			fprintf( stderr, "Warning: overlapping DRMA communication detected by invalid calls to bsp_hpget or bsp_hpput. This results in undefined behaviour!\n" );
			//ensure `correctness' by doing move instead of copy:
			memmove( request.destination, request.source, request.length );
		}
#else
		//directly execute HP communication;
		//overlap results in undefined behaviour (BSPlib standard), so crashing is OK
		mcbsp_util_memcpy( request.destination, request.source, request.length );
#endif
#if MCBSP_MODE == 3
		const size_t metadata = sizeof(struct mcbsp_hp_request);
		if( request.source_is_remote ) {
			//then this was a bsp_hpget
			data->superstep_stats.bytes_sent         += request.length;
			data->superstep_stats.metabytes_received += metadata;
		} else {
			//otherwise this was a hpput
			data->superstep_stats.bytes_received     += request.length;
			data->superstep_stats.metabytes_received += metadata;
		}
#endif
	} //go to next hp drma request

	//see if synchronisation is complete (after this synch we perform non-hp DRMA communication, as well as hp BSMP communication)
#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
#else
	mcbsp_internal_sync( data->init, &(data->init->condition) );
#endif

	//wait for earlier data cache purge to finish (__builtin_k1_dpurge);
	//after that the code can continue as though we were cache-coherent,
	//since during communication threads read from remote buffers and modify
	//local buffers, and after sync only local memory will be used.
	mcbsp_nocc_wait_for_flush();
	//note that this may not write back some of the data modified by hp_requests,
	//but the standard stipulates any communication reading from areas modified
	//with hp-primitives in the same communication phase results in undefined
	//behaviour.

	//check for mismatched sync/end
#ifndef MCBSP_NO_CHECKS
	if( data->init->ended ) {
		fprintf( stderr, "Mismatched bsp_sync and bsp_end detected!\n" );
		mcbsp_util_fatal();
	}
#endif

	//handle the various BSP requests
	
	//update tagSize, phase 1
	if( data->bsp_id == 0 && data->newTagSize != data->init->tagSize ) {
		data->init->tagSize = data->newTagSize;
	}

	//look for requests with destination us, first cache get-requests
	//since they should not be polluted by incoming put requests
	for( size_t k = 0; k < data->init->P; ++k ) {
#ifdef MCBSP_CA_SYNC
		//do round-robin sync
		const size_t s = (k + data->bsp_id) % data->init->P;
#else
		const size_t s = k;
#endif
		//get remote get request queue
		struct mcbsp_util_stack * const queue = &(data->init->threadData[ s ]->request_queues[ data->bsp_id ]);
		//each request in queue is directed to us. Handle all of them.
		while( !mcbsp_util_stack_empty( queue ) ) {
			//get get-request
			const struct mcbsp_get_request * const request = (struct mcbsp_get_request *) mcbsp_util_stack_pop( queue );
			//check if source address is destination address
			if( request->source == request->destination ) {
				//then communication here is useless, regardless of length;
				//ignore this request
				continue;
			}
			//the top part is actually the BSP message header
			const struct mcbsp_message * const message = (const struct mcbsp_message * const) ((const char * const)request + sizeof( void * ));
			//put data in our local bsp_put communication queue
			struct mcbsp_util_stack * const comm_queue = &(data->queues[ s ]);
			mcbsp_util_varstack_push( comm_queue, request->source, request->length );
			//put BSP message header
			mcbsp_util_varstack_regpush( comm_queue, message );
#if MCBSP_MODE == 3
			const size_t metadata = sizeof(struct mcbsp_message);
			data->superstep_stats.bytes_buffered += request->length;
			data->superstep_stats.bytes_sent     += request->length + metadata;
			data->superstep_stats.metabytes_sent += metadata;
#endif
		}
	}

	//handle pop_regs: loop over all locals, without destroying the stacks
	//FIXME the handling of (de-)registration is probably the next thing on the list of internal improvements for MulticoreBSP
	for( size_t i = 0; i < data->localsToRemove.top; ++i ) {
		void * const * const array = (void **)(data->localsToRemove.array);
		void * const toRemove = array[ i ];
		void * search_address = toRemove;
		const struct mcbsp_util_address_map * search_map = &(data->local2global);
		if( toRemove == NULL ) {
			//use other process' logic to process this pop_reg
			size_t s;
			void * const * array2 = NULL;
			//find processor with non-NULL entry at this point
			for( s = 0; s < data->init->P; ++s ) {
				array2 = (void**)(data->init->threadData[ s ]->localsToRemove.array);
				if( array2[ i ] != NULL )
					break;
			}
			//check if we found an entry
#ifndef MCBSP_NO_CHECKS
			if( s == data->init->P ) {
				fprintf( stderr, "Warning: tried to de-register a NULL address at all processes!\n" );
				continue; //ignore
			}
#endif
			//use this processor's logic
			search_address = array2[ i ];
			search_map = &(data->init->threadData[ s ]->local2global);
		}

		//get global key. Note we may be searching in other processors' map, thus:
		// -this should happen after a barrier to ensure all processors have all pop_regs.
		// -this function cannot change the local local2global map
		const size_t globalIndex = mcbsp_util_address_map_get( search_map, search_address );

#ifdef MCBSP_DEBUG_REGS
		fprintf( stderr, "Debug: %d removes address %p (global table entry %lu).\n", data->bsp_id, search_address, globalIndex );
#endif

		//sanity check
		if( globalIndex == SIZE_MAX ) {
			fprintf( stderr, "Warning: tried to de-register a non-registered variable. The corresponding call to bsp_pop_reg will have no effect.\n" );
		} else {
			//delete entry from table
			if( !mcbsp_util_address_table_delete( &(data->init->global2local), globalIndex, data->bsp_id ) ) {
				//there are still other registrations active. Do not delete from map later on.
				((void **)(data->localsToRemove.array))[ i ] = NULL;
			} //otherwise we should delete, but after the next barrier

			//register globalIndex now is free
			if( data->localC == globalIndex + 1 ) {
				--(data->localC);
			} else {
				mcbsp_util_stack_push( &(data->removedGlobals), (const void *)(&globalIndex) );
			}
		}

#ifdef MCBSP_DEBUG_REGS
		fprintf( stderr, "Debug: %d has local registration count %ld and a removedGlobals stack size of %ld.\n", data->bsp_id, data->localC, data->removedGlobals.top );
#endif
	}

	//coordinate exit using the same mutex (but not same condition!)
#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_mid_condition, (size_t)(data->bsp_id) );
#else
	mcbsp_internal_sync( data->init, &(data->init->mid_condition) );
#endif

	//hard invalidate / barrier, although invalidation targeting the
	//variable registration as well as the communication request
	//queue (get translation into put) should be enough. A rewrite
	//of the global variable registration may make this issue a lot
	//more transparent than it is currently; FIXME.
	mcbsp_nocc_purge_all();
	mcbsp_nocc_wait_for_flush();

	//handle pop_regs: loop over all locals, without destroying the stacks
	while( !mcbsp_util_stack_empty( &(data->localsToRemove) ) ) {
		void * const toRemove = *(void**)mcbsp_util_stack_pop( &(data->localsToRemove) );
		//NOTE: this is safe, since it is guaranteed that this address table entry
		//	will not change during synchronisation.

		//delete from map, if address was not NULL
		if( toRemove != NULL ) {

#ifdef MCBSP_DEBUG_REGS
		fprintf( stderr, "Debug: %d removes address %p from local2global map.\n", data->bsp_id, toRemove );
#endif

			mcbsp_util_address_map_remove( &(data->local2global), toRemove );
		}
	}

	//handle push_reg after sync to avoid races on the internal table when NULL pointers exist
	for( size_t i = 0; i < data->localsToPush.top; ++i ) {
		//get array
		const struct mcbsp_push_request * const array = 
			(struct mcbsp_push_request *)data->localsToPush.array;
		//get address
		const struct mcbsp_push_request request = array[ i ];
		void * const address = request.address;

		//get size
		const size_t size = ((size_t)request.size);

		//set initial search targets
		void * search_address = address;
		const struct mcbsp_util_address_map * search_map = &(data->local2global);

		//check for NULL-pointers; if found, change search targets
		if( address == NULL ) {
			//search for a neighbour with a non-NULL entry
			size_t s;
			const struct mcbsp_push_request * array2 = NULL;
			for( s = 0; s < data->init->P; ++s ) {
				array2 = (struct mcbsp_push_request *)data->init->threadData[ s ]->localsToPush.array;
				if( array2[ i ].address != NULL ) //bingo!
					break;
			}
			//check if we did find a neighbour
#ifndef MCBSP_NO_CHECKS
			if( s == data->init->P ) {
				//this is an all-NULL registration. Ignore
				fprintf( stderr, "Warning: tried to register a NULL-address on all processes!\n" );
				continue;
			}
#endif
			//do logic using data from process s instead of local process
			search_address = array2[ i ].address;
			search_map     = &(data->init->threadData[ s ]->local2global);
		}

		//get global index of this registration. First check map if the key already existed
		const unsigned long int mapSearch = mcbsp_util_address_map_get( search_map, search_address );

		//if the key was not found, create a new global entry
		const unsigned long int global_number = mapSearch != ULONG_MAX ? mapSearch :
								mcbsp_util_stack_empty( &(data->removedGlobals) ) ?
								data->localC++ :
								*(unsigned long int*)mcbsp_util_stack_pop( &(data->removedGlobals) );

		//insert value, global2local map (false sharing is possible here, but effects should be negligable)
		mcbsp_util_address_table_set( &(data->init->global2local), global_number, data->bsp_id, address, size );

#ifdef MCBSP_DEBUG_REGS
		fprintf( stderr, "Debug: %d adds %p to global ID %lu in address table.\n", data->bsp_id, address, global_number );
#endif

		//insert value in local2global map (if this is a new global entry), but not if NULL pointer
		//the actual insertion happens after the next barrier, since the above code may consult
		//other processor's local2global map (see also handling of the bsp_pop_reg requests).
		if( mapSearch == ULONG_MAX && address != NULL ) {
			mcbsp_util_stack_push( &(data->globalsToPush), &global_number );
		} else {
			((struct mcbsp_push_request *)data->localsToPush.array)[ i ].address = NULL;
		}

	}

	//update tagsize, phase 2 (check)
#ifndef MCBSP_NO_CHECKS
	if( data->newTagSize != data->init->tagSize ) {
		fprintf( stderr, "Different tag sizes requested from different processes (%lu requested while process 0 requested %lu)!\n",
			(unsigned long int)(data->newTagSize),
			(unsigned long int)(data->init->tagSize)
		);
		mcbsp_util_fatal();
	}
#endif
	
	//now process requests to local destination
	for( size_t k = 0; k < data->init->P; ++k ) {
#ifdef MCBSP_CA_SYNC
		//do round-robin sync
		const size_t s = (k + data->bsp_id) % data->init->P;
#else
		const size_t s = k;
#endif
		//handle queue for hp-BSMP items
		struct mcbsp_util_stack * const hpqueue =
			&(data->init->threadData[ s ]->hpsend_queues[ data->bsp_id ]);
		//each message in this queue is directed at us; handle them
		while( !mcbsp_util_stack_empty( hpqueue ) ) {
			//pop hpsend request from outgoing stack
			const struct mcbsp_hpsend_request * const request =
				(struct mcbsp_hpsend_request *) mcbsp_util_stack_pop( hpqueue );
			//add payload to bsmp queue
			mcbsp_util_varstack_push( &(data->bsmp), request->payload, request->payload_size );
			//add tag to bsmp queue
			mcbsp_util_varstack_push( &(data->bsmp), request->tag, data->init->tagSize );
			//record length of payload
			mcbsp_util_varstack_regpush( &(data->bsmp), &(request->payload_size) );
#if MCBSP_MODE == 3
			//record profile
			const size_t metadata = sizeof(struct mcbsp_hpsend_request);
			data->superstep_stats.bytes_received     += request->payload_size + metadata;
			data->superstep_stats.metabytes_received += metadata;
#endif
		} //go to next hpsend request

		//put and get requests handled here
		struct mcbsp_util_stack * const queue = &(data->init->threadData[ s ]->queues[ data->bsp_id ]);
		//each request in queue is directed to us. Handle all of them.
		while( !mcbsp_util_stack_empty( queue ) ) {
			struct mcbsp_message * const request = (struct mcbsp_message*) mcbsp_util_varstack_regpop( queue );
			if( request->destination == NULL ) {
				//get tag and payload from queue
				const void * const tag     = mcbsp_util_varstack_pop( queue, data->init->tagSize );
				const void * const payload = mcbsp_util_varstack_pop( queue, request->length );
				//push payload
				mcbsp_util_varstack_push( &(data->bsmp), payload, request->length );
				//push tag
				mcbsp_util_varstack_push( &(data->bsmp), tag, data->init->tagSize );
				//push payload size
				mcbsp_util_varstack_regpush( &(data->bsmp), &(request->length) );
			} else {
				//copy payload to destination
				mcbsp_util_memcpy(
					request->destination,
					mcbsp_util_varstack_pop( queue, request->length ),
					request->length
				);
			}
#if MCBSP_MODE == 3
			//record profile
			const size_t metadata = sizeof(struct mcbsp_message);
			data->superstep_stats.bytes_received     += request->length + data->init->tagSize + metadata;
			data->superstep_stats.metabytes_received += metadata;
#endif
		}
	} //go to next processors' outgoing queues

#ifdef MCBSP_WITH_DMTCP
	//get currently active checkpoint frequency
	const size_t cp_frequency = data->init->safe_cp ? 
		data->init->safe_cp_f : 
		data->init->cp_f;
	//if (automatic) checkpoiting was not disabled
	if( data->init->no_cp == false && cp_frequency > 0 ) {
		//check if we need to do automatic checkpointing
		if( data->init->skipped_checkpoints >= cp_frequency ) {
			//wait until everyone's communication is done
 #ifdef MCBSP_USE_SPINLOCK
			mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
 #else
			mcbsp_internal_sync( data->init, &(data->init->condition) );
 #endif
			//then checkpoint
			if( data->bsp_id == 0 ) {
				data->init->skipped_checkpoints = 0;
				mcbsp_internal_call_checkpoint();
			}
			//leave exit sync to regular bsp_sync code, below
		}
	}
#endif

	//final sync
#ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_end_condition, (size_t)(data->bsp_id) );
#else
	mcbsp_internal_sync( data->init, &(data->init->end_condition) );
#endif

	//hard invalidate / barrier, although invalidation targeting only the
	//variable registration should be enough
	mcbsp_nocc_purge_all();
	mcbsp_nocc_wait_for_flush();

	//final processing of push_regs
	while( !mcbsp_util_stack_empty( &(data->localsToPush) ) ) {

		//get push request
		const struct mcbsp_push_request request =
			*(struct mcbsp_push_request *) mcbsp_util_stack_pop( &(data->localsToPush ) );

		//get address
		void * const address = request.address;

		//ignore NULL addresses
		if( address == NULL ) {
			continue;
		}

		//get global number as derived earlier
		assert( !mcbsp_util_stack_empty( &(data->globalsToPush ) ) );
		const unsigned long int global_number = *(unsigned long int*)mcbsp_util_stack_pop( &(data->globalsToPush) );

#ifdef MCBSP_DEBUG_REGS
		fprintf( stderr, "Debug: %d maps address %p to global ID %lu.\n", data->bsp_id, address, global_number );
#endif

		//add to address and corresponding global index to local2global map
		mcbsp_util_address_map_insert( &(data->local2global), address, global_number );
	} //go to next address to register

#if MCBSP_MODE == 3
	//end communication phase timing
	data->superstep_stats.communication = mcbsp_internal_time( data ) - comm_start;
	//push current profile data
	mcbsp_util_stack_push( &(data->profile), &(data->superstep_stats) );
	//record new time for compute phase start
	data->superstep_start_time = mcbsp_internal_time( data );
	mcbsp_internal_init_superstep_stats( &(data->superstep_stats), data->init->current_superstep );
#endif
} //end synchronisation

double MCBSP_FUNCTION_PREFIX(time)( void ) {
	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#if MCBSP_MODE == 3
	++(data->superstep_stats.non_communicating);
#endif

	return mcbsp_internal_time( data );
}

void MCBSP_FUNCTION_PREFIX(push_reg)( void * const address, const bsp_size_t size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t size = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#if MCBSP_MODE == 3
	++(data->superstep_stats.bookkeeping);
#endif

#ifdef MCBSP_DEBUG_REGS
	fprintf( stderr, "Debug: %d registers address %p with size %lu.\n", data->bsp_id, address, (unsigned long int)(size_in) );
#endif

	//construct the push request
	struct mcbsp_push_request toPush;
	toPush.address = address;
	toPush.size    = size;

	//push the request
	mcbsp_util_stack_push( &(data->localsToPush), (void*)(&toPush) );
}

void MCBSP_FUNCTION_PREFIX(pop_reg)( void * const address ) {
	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifdef MCBSP_DEBUG_REGS
	fprintf( stderr, "Debug: %d de-registers address %p.\n", data->bsp_id, address );
#endif

	//register for removal
	mcbsp_util_stack_push( &(data->localsToRemove), (const void *)(&address) );
}

void MCBSP_FUNCTION_PREFIX(put)(
	const bsp_pid_t pid, const void * const source,
	const void * const destination, const bsp_size_t offset_in,
	const bsp_size_t size_in
) {
	//catch border cases
	if( size_in == 0 ) {
		//simply ignore empty communication requests
		return;
	}

	//library internals work with size_t only; convert if necessary
	const size_t offset = (size_t) offset_in;
	const size_t size   = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_put called with an out-of-range PID argument (%zd, while bsp_nprocs()==%zd)!\n",
			(size_t)pid, data->init->P );
	}
#endif

	//build request
	struct mcbsp_message request;

	//record destination; get global index from local map
	const unsigned long int globalIndex = mcbsp_util_address_map_get( &(data->local2global), destination );

	//sanity checks
	assert( (size_t)pid < data->init->P );
#ifndef MCBSP_NO_CHECKS
	if( globalIndex == ULONG_MAX ) {
		fprintf( stderr, "Error: bsp_put into unregistered memory area (%p) requested!\n", destination );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}	
#endif

	//get entry from address table
	const struct mcbsp_util_address_table_entry * const entry = mcbsp_util_address_table_get( &(data->init->global2local), globalIndex, ((size_t)pid) );

	//sanity checks
#ifndef MCBSP_NO_CHECKS
	if( entry == NULL ) {
		fprintf( stderr, "Error: bsp_put called with an erroneously registered destination variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( offset + size > entry->size ) {
		fprintf( stderr, "Error: bsp_put would go out of bounds at destination processor (offset=%lu, size=%lu, while registered memory area is %lu bytes)!\n", (unsigned long int)offset, (unsigned long int)size, (unsigned long int)(entry->size) );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( entry->address == NULL ) {
		fprintf( stderr, "Error: communication attempted on NULL-registered variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( source == NULL ) {
		fprintf( stderr, "Error: communication of NULL memory address requested.\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
#endif

	//record final destination address
	request.destination = ((char*)(entry->address)) + offset;

	//record length
	request.length = size;

#if MCBSP_MODE == 3
	const size_t metadata_size  = sizeof( struct mcbsp_message );
	++(data->superstep_stats.put);
	data->superstep_stats.bytes_buffered += size;
	data->superstep_stats.bytes_sent     += size + metadata_size;
	data->superstep_stats.metabytes_sent += metadata_size;
	const double buffer_start = mcbsp_internal_time( data );
#endif

	//record payload
	mcbsp_util_varstack_push( &(data->queues[ pid ]), source, size );

#if MCBSP_MODE == 3
	data->superstep_stats.buffering += mcbsp_internal_time( data ) - buffer_start;
#endif

	//record request header
	mcbsp_util_varstack_regpush( &(data->queues[ pid ]), &request );
}

void MCBSP_FUNCTION_PREFIX(get)( const bsp_pid_t pid, const void * const source,
	const bsp_size_t offset_in, void * const destination,
	const bsp_size_t size_in ) {

	//catch border cases
	if( size_in == 0 ) {
		//simply ignore empty communication requests
		return;
	}

	//library internals work with size_t only; convert if necessary
	const size_t offset = (size_t) offset_in;
	const size_t size   = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#if MCBSP_MODE == 3
	++(data->superstep_stats.get);
#endif

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_get called with an out-of-range PID argument (%ld, while bsp_nprocs()==%zd)!\n",
			(long int)pid, data->init->P );
	}
#endif

	//build request
	struct mcbsp_get_request request;

	//record source address, plus sanity checks
	const unsigned long int globalIndex = mcbsp_util_address_map_get( &(data->local2global), source );
#ifndef MCBSP_NO_CHECKS
	if( globalIndex == ULONG_MAX ) {
		fprintf( stderr, "Error: bsp_get into unregistered memory area (%p) requested!\n", destination );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}	
#endif
	const struct mcbsp_util_address_table_entry * entry = mcbsp_util_address_table_get( &(data->init->global2local), globalIndex, ((size_t)pid) );
#ifndef MCBSP_NO_CHECKS
	if( entry == NULL ) {
		fprintf( stderr, "Error: bsp_get called with an erroneously registered source variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
	if( offset + size > entry->size ) {
		fprintf( stderr, "Error: bsp_get would go out of bounds at source processor (offset=%lu, size=%lu, while registered memory area is %lu bytes)!\n", (unsigned long int)offset, (unsigned long int)size, (unsigned long int)(entry->size) );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
#endif
	//record source
	request.source = ((char*)(entry->address)) + offset;

	//record destination
	request.destination = destination;

	//record length
	request.length = size;

	//record request
	mcbsp_util_stack_push( &(data->request_queues[ data->bsp_id ]), &request );
}

void MCBSP_FUNCTION_PREFIX(direct_get)( const bsp_pid_t pid, const void * const source,
        const bsp_size_t offset_in, void * const destination,
	const bsp_size_t size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t offset = (size_t) offset_in;
	const size_t size   = (size_t) size_in;
	
	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_direct_get called with an out-of-range PID argument (%ld, while bsp_nprocs()==%zd)!\n",
			(long int)pid, data->init->P );
	}
#endif

	//get source address
	const unsigned long int globalIndex = mcbsp_util_address_map_get( &(data->local2global), source );
#ifndef MCBSP_NO_CHECKS
	if( globalIndex == ULONG_MAX ) {
		bsp_abort( "Error: bsp_direct_get called with unregistered remote address!\n" );
	}
#endif
	const struct mcbsp_util_address_table_entry * entry = mcbsp_util_address_table_get( &(data->init->global2local), globalIndex, ((size_t)pid) );
#ifndef MCBSP_NO_CHECKS
	if( entry == NULL ) {
		fprintf( stderr, "Error: bsp_direct_get called with an erroneously registered source variable (address %p was never registered remotely)!\n", source );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
	if( offset + size > entry->size ) {
		fprintf( stderr, "Error: bsp_direct_get would go out of bounds at source processor (offset=%lu, size=%lu, while registered memory area is %lu bytes)!\n", (unsigned long int)offset, (unsigned long int)size, (unsigned long int)(entry->size) );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
#endif

	//perform direct get, first derive target memory area
	const char * const from = ((char*)entry->address) + offset;
	//check if we need to do anything, which is when destination equals source
	if( destination != (const void * const)from ) {
		//we need to move data around, check if we can use a memcpy-approach
		bool cpy = true;
		//always calculate offsets such that distances are positive numbers
		if( destination > (const void * const)from ) {
			if( (size_t)((char*)destination - from) < size ) {
				//source and destination areas overlap, precludes memcpy approach
				cpy = false;
			}
		} else {
			if( (size_t)(from - (char*)destination) < size ) {
				//source and destination areas overlap, precludes memcpy approach
				cpy = false;
			}
		}
		//perform data movement
		if( cpy ) {
			mcbsp_util_memcpy( destination, (const void *)from, size );
		} else {
			memmove( destination, (const void *)from, size );
		}
	}
#if MCBSP_MODE == 3
	++(data->superstep_stats.direct_get);
	data->superstep_stats.direct_get_bytes += size;
#endif
}

void MCBSP_FUNCTION_PREFIX(set_tagsize)( bsp_size_t * const size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t size = (size_t) *size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#if MCBSP_MODE == 3
	++(data->superstep_stats.bookkeeping);
#endif

	//record new tagsize
	data->newTagSize = size;

	//return old tag size
	*size_in = (bsp_size_t) (data->init->tagSize);
}

void MCBSP_FUNCTION_PREFIX(send)( const bsp_pid_t pid, const void * const tag,
	const void * const payload, const bsp_size_t size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t size = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_send called with an out-of-range PID argument (%ld, while bsp_nprocs()==%zd)!\n",
			(long int)pid, data->init->P );
	}
#endif

	//build request
	struct mcbsp_message request;

	//record destination
	request.destination = NULL;

	//record payload length
	request.length = size;

#if MCBSP_MODE == 3
	++(data->superstep_stats.send);
	const double buffer_start = mcbsp_internal_time( data );
#endif

	//record payload
	mcbsp_util_varstack_push( &(data->queues[ pid ]), payload, size );

#if MCBSP_MODE == 3
	data->superstep_stats.buffering += mcbsp_internal_time( data ) - buffer_start;
	const size_t metadata = sizeof( struct mcbsp_message );
	data->superstep_stats.bytes_buffered += size;
	data->superstep_stats.metabytes_sent += metadata;
	data->superstep_stats.bytes_sent     += size + metadata;
#endif

	//record tag
	if( data->init->tagSize != 0 ) {
		mcbsp_util_varstack_push( &(data->queues[ pid ]), tag, data->init->tagSize );
	}

	//record message header
	mcbsp_util_varstack_regpush( &(data->queues[ pid ]), &request );	
}

#ifdef MCBSP_ENABLE_HP_DIRECTIVES
void MCBSP_FUNCTION_PREFIX(hpsend)( const bsp_pid_t pid, const void * const tag,
	const void * const payload, const bsp_size_t size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t size = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_hpsend called with an out-of-range PID argument (%ld, while bsp_nprocs()==%zd)!\n",
			(long int)pid, data->init->P );
	}
#endif

	//build request
	struct mcbsp_hpsend_request request;

	//record source address
	request.payload = payload;
	request.tag     = tag; //misuse destination field

	//record length
	request.payload_size = size;

	//record request
	mcbsp_util_stack_push( &(data->hpsend_queues[ pid ]), &request );

#if MCBSP_MODE == 3
	++(data->superstep_stats.hpsend);
	const size_t metadata = sizeof( struct mcbsp_message );
	data->superstep_stats.metabytes_sent += metadata;
	data->superstep_stats.bytes_sent     += size + metadata;
#endif
}

void MCBSP_FUNCTION_PREFIX(hpput)( const bsp_pid_t pid, const void * const source,
	const void * const destination, const bsp_size_t offset_in,
	const bsp_size_t size_in ) {

	//library internals work with size_t only; convert if necessary
	const size_t offset = (size_t) offset_in;
	const size_t size   = (size_t) size_in;

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity check
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_hpput called with an out-of-range PID argument (%zd, while bsp_nprocs()==%zd)!\n",
			(size_t)pid, (data->init->P) );
	}
#endif

	//build request
	struct mcbsp_hp_request request;
	request.source_is_remote = false;

	//record destination; get global index from local map
	const unsigned long int globalIndex = mcbsp_util_address_map_get( &(data->local2global), destination );

	//sanity checks
	assert( (size_t)pid < data->init->P );
#ifndef MCBSP_NO_CHECKS
	if( globalIndex == ULONG_MAX ) {
		fprintf( stderr, "Error: bsp_hpput into unregistered memory area (%p) requested!\n", destination );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}	
#endif

	//get entry from address table
	const struct mcbsp_util_address_table_entry * const entry = mcbsp_util_address_table_get( &(data->init->global2local), globalIndex, ((size_t)pid) );

	//sanity checks
#ifndef MCBSP_NO_CHECKS
	if( entry == NULL ) {
		fprintf( stderr, "Error: bsp_hpput called with an erroneously registered destination variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( offset + size > entry->size ) {
		fprintf( stderr, "Error: bsp_hpput would go out of bounds at destination processor (offset=%lu, size=%lu, while registered memory area is %lu bytes)!\n", (unsigned long int)offset, (unsigned long int)size, (unsigned long int)(entry->size) );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( entry->address == NULL ) {
		fprintf( stderr, "Error: communication attempted on NULL-registered variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
	if( source == NULL ) {
		fprintf( stderr, "Error: communication of NULL memory address requested.\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments.\n" );
	}
#endif

	//record source address
	request.source = source;

	//record final destination address
	request.destination = ((char*)(entry->address)) + offset;

	//record length
	request.length = size;

	//record payload
	mcbsp_util_stack_push( &(data->hpdrma_queue), &request );

#if MCBSP_MODE == 3
	//record profile
	++(data->superstep_stats.hpput);
	const size_t metadata = sizeof( struct mcbsp_hp_request );
	data->superstep_stats.bytes_sent     += request.length + metadata;
	data->superstep_stats.metabytes_sent += request.length;
#endif
}

void MCBSP_FUNCTION_PREFIX(hpget)(
	const bsp_pid_t pid, const void * const source,
	const bsp_size_t offset_in, void * const destination,
	const bsp_size_t size_in
) {

	//library internals work with size_t only; convert if necessary
	const size_t offset = (size_t) offset_in;
	const size_t size   = (size_t) size_in;

	//handle border cases
	if( size_in == 0 ) {
		//ignore empty requests
		return;
	}

	//get init data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#ifndef MCBSP_NO_CHECKS
	//sanity checks
	if( (size_t)pid >= data->init->P ) {
		bsp_abort( "Error: bsp_hpget called with an out-of-range PID argument (%ld, while bsp_nprocs()==%zd)!\n",
			(long int)pid, (unsigned long int)(data->init->P) );
	}
	if( destination == NULL ) {
		fprintf( stderr, "Error: bsp_hpget into NULL pointer requested!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
#endif

	//build request
	struct mcbsp_hp_request request;
	request.source_is_remote = true;

	//record source address, plus sanity checks
	const unsigned long int globalIndex = mcbsp_util_address_map_get( &(data->local2global), source );
#ifndef MCBSP_NO_CHECKS
	if( globalIndex == ULONG_MAX ) {
		fprintf( stderr, "Error: bsp_hpget into unregistered memory area (%p) requested!\n", destination );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}	
#endif
	const struct mcbsp_util_address_table_entry * entry = mcbsp_util_address_table_get( &(data->init->global2local), globalIndex, ((size_t)pid) );
#ifndef MCBSP_NO_CHECKS
	if( entry == NULL ) {
		fprintf( stderr, "Error: bsp_hpget called with an erroneously registered source variable!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
	if( offset + size > entry->size ) {
		fprintf( stderr, "Error: bsp_hpget would go out of bounds at source processor (offset=%lu, size=%lu, while registered memory area is %lu bytes)!\n", (unsigned long int)offset, (unsigned long int)size, (unsigned long int)(entry->size) );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
	if( entry->address == NULL ) {
		fprintf( stderr, "Error: bsp_hpget requested from NULL-registered remote source!\n" );
		bsp_abort( "Aborting due to BSP primitive call with invalid arguments." );
	}
#endif

	//record source
	request.source = ((char*)(entry->address)) + offset;

	//record destination
	request.destination = destination;

	//record length
	request.length = size;

	//sanity checks
	assert( request.source != NULL );
	assert( request.destination != NULL );
	assert( request.length != 0 );

	//record request
	mcbsp_util_stack_push( &(data->hpdrma_queue), &request );

#if MCBSP_MODE == 3
	//record profile
	++(data->superstep_stats.hpget);
	data->superstep_stats.metabytes_sent += sizeof( struct mcbsp_hp_request );
	data->superstep_stats.bytes_received += request.length;
#endif
}
#endif

void MCBSP_FUNCTION_PREFIX(qsize)(
	bsp_nprocs_t * const packets,
	bsp_size_t * const accumulated_size
) {
#if MCBSP_MODE == 3
	//get thread data, record profile
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();
	++(data->superstep_stats.non_communicating);
#else
	//get thread data
	const struct mcbsp_thread_data * const data = mcbsp_internal_const_prefunction();
#endif

	//check no-messages case
	if( mcbsp_util_stack_empty( &(data->bsmp) ) ) {
		*packets = (bsp_nprocs_t) 0;
		if( accumulated_size != NULL )
			*accumulated_size = 0;
		return;
	}

	//set initial values
	*packets = 0;
	if( accumulated_size != NULL )
		*accumulated_size = 0;

	//loop over all messages
	const char * raw = (char*)(data->bsmp.array) + data->bsmp.top - sizeof( size_t );
	do {
		//if requested, count accumulated message sizes
		const size_t curmsg_size = *(const size_t*)raw;
		if( accumulated_size != NULL )
			*accumulated_size += curmsg_size;
		//skip payload size, tag size, and payload size fields
		raw -= curmsg_size + data->init->tagSize + sizeof( size_t );
		//we skipped one packet
		++(*packets);
		//sanity check
		assert( raw + sizeof( size_t ) >= (char*)(data->bsmp.array) );
		//continue loop if we did not go past the start of the bsmp array
	} while( raw + sizeof( size_t ) != (char*)data->bsmp.array );
}

void MCBSP_FUNCTION_PREFIX(move)( void * const payload, const bsp_size_t max_copy_size_in ) {
	//library internals work with size_t only; convert if necessary
	const size_t max_copy_size = (size_t) max_copy_size_in;

	//get thread data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

	//if stack is empty, do not return anything
	if( mcbsp_util_stack_empty( &(data->bsmp) ) ) {
		return;
	}

	//get payload size
	const size_t size = *(size_t*) mcbsp_util_varstack_regpop( &(data->bsmp) );

	//skip tag
	mcbsp_util_varstack_pop( &(data->bsmp), data->init->tagSize );

#if MCBSP_MODE == 3
	const double buffer_start = mcbsp_internal_time( data );
#endif

	//copy message
	const size_t copy_size = size > max_copy_size ? max_copy_size : size;
	memcpy( payload, mcbsp_util_varstack_pop( &(data->bsmp), size ), copy_size );

#if MCBSP_MODE == 3
	//record profile
	data->superstep_stats.buffering += mcbsp_internal_time( data ) - buffer_start;
	data->superstep_stats.bytes_buffered += copy_size;
	++(data->superstep_stats.move);
#endif
}

void MCBSP_FUNCTION_PREFIX(get_tag)( bsp_size_t * const status, void * const tag ) {
#if MCBSP_MODE == 3
	//get thread data, record profile
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();
	++(data->superstep_stats.non_communicating);
#else
	//get thread data
	const struct mcbsp_thread_data * const data = mcbsp_internal_const_prefunction();
#endif

	//if stack is empty, set status to -1
	if( mcbsp_util_stack_empty( &(data->bsmp) ) ) {

		//return failure
		*status = (bsp_size_t)-1;

	} else {

		//get size
		const size_t size = *((size_t*) mcbsp_util_varstack_regpeek( &(data->bsmp) ));

		//set status to payload size
		*status = (bsp_size_t) size;

		//copy tag into target memory area: location is the top of the bsmp stack minus the payload size field
		//(note the last sizeof( size_t ) bytes are not copied; this was the payload length.)
		memcpy( tag, mcbsp_util_varstack_peek( &(data->bsmp), data->init->tagSize + sizeof( size_t ) ), data->init->tagSize );

	}
}

bsp_size_t MCBSP_FUNCTION_PREFIX(hpmove)( void* * const p_tag, void* * const p_payload ) {
	//get thread data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();

#if MCBSP_MODE == 3
	//record profile
	++(data->superstep_stats.hpmove);
#endif

	//if empty, return -1 or SIZE_MAX, as per the function definition
	if( mcbsp_util_stack_empty( &(data->bsmp) ) ) {
		return ((bsp_size_t)-1);
	}

	//the data->bsmp stack contains, in this order, the following fields:
	//-payload size
	//-the BSMP tag
	//-the BSMP payload
	//we now retrieve those:
	const size_t size = *((size_t*) mcbsp_util_varstack_regpop( &(data->bsmp) ));
	*p_tag     = mcbsp_util_varstack_pop( &(data->bsmp), data->init->tagSize );
	*p_payload = mcbsp_util_varstack_pop( &(data->bsmp), size );

	//return the payload length, as per the specification
	return (bsp_size_t) size;
}

#ifdef MCBSP_ENABLE_FAKE_HP_DIRECTIVES
void MCBSP_FUNCTION_PREFIX(hpput)( const bsp_pid_t pid, const void * const source,
        const void * const destination, const bsp_size_t offset,
	const bsp_size_t size ) {
	MCBSP_FUNCTION_PREFIX(put)( pid, source, destination, offset, size );
}

void MCBSP_FUNCTION_PREFIX(hpget)( const bsp_pid_t pid, const void * const source,
        const bsp_size_t offset, void * const destination,
	const bsp_size_t size ) {
	MCBSP_FUNCTION_PREFIX(get)( pid, source, offset, destination, size );
}
#endif

