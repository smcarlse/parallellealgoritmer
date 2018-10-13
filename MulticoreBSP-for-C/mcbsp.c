/*
 * Copyright (c) 2012
 *
 * File created by A. N. Yzelman, 2012.
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

//global fields

size_t MCBSP_DEFAULT_THREADS_PER_CORE = 1;

size_t MCBSP_DEFAULT_CHECKPOINT_FREQUENCY = 0;

enum mcbsp_affinity_mode MCBSP_DEFAULT_AFFINITY = SCATTER;

enum mcbsp_thread_numbering MCBSP_DEFAULT_THREAD_NUMBERING = CONSECUTIVE;

pthread_key_t mcbsp_internal_init_data;

pthread_key_t mcbsp_internal_thread_data;

pthread_key_t mcbsp_local_machine_info;

pthread_mutex_t mcbsp_internal_keys_mutex = PTHREAD_MUTEX_INITIALIZER;

bool mcbsp_internal_keys_allocated = false;

//MulticoreBSP extension functions

void mcbsp_set_maximum_threads( const size_t max ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->threads = max;
	machine->Tset = true;
}

void mcbsp_set_affinity_mode( const enum mcbsp_affinity_mode mode ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->affinity = mode;
	machine->Aset = true;
}

void mcbsp_set_available_cores( const size_t num_cores ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->cores = num_cores;
	machine->Cset = true;
}

void mcbsp_set_threads_per_core( const size_t TpC ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->threads_per_core = TpC;
	machine->TpCset = true;
}

void mcbsp_set_unused_threads_per_core( const size_t UTpC ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->unused_threads_per_core = UTpC;
	machine->UTset = true;
}

void mcbsp_set_thread_numbering( const enum mcbsp_thread_numbering numbering ) {
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	machine->thread_numbering = numbering;
	machine->TNset = true;
}

void mcbsp_set_pinning( const size_t * const pinning, const size_t length ) {
	//get machine info
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	//set number of threads
	machine->threads = length;
	//check for NULL
	if( pinning == NULL ) {
		machine->manual_affinity = NULL;
		if( machine->affinity != MANUAL ) {
			machine->affinity = MCBSP_DEFAULT_AFFINITY;
			fprintf( stderr, "Warning: mcbsp_set_pinning called with NULL pinning. Updated supported threads as normal, but reset affinity strategy to defaults.\n" );
		}
	}
	//allocate pinning array
	machine->manual_affinity = mcbsp_util_malloc( length * sizeof( size_t ), "mcbsp_set_pinning MulticoreBSP manual affinity array (copy of user input)" );
	//copy array
	for( size_t s = 0; s < machine->threads; ++s ) {
		machine->manual_affinity[ s ] = pinning[ s ];
	}
	//also set affinity
	machine->affinity = MANUAL;
	//set manual flags
	machine->Tset = true;
	machine->Pset = true;
	machine->Aset = true;
}

void mcbsp_set_reserved_cores( const size_t * const reserved, const size_t length ) {
	//get machine info
	struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	//check for NULL
	if( reserved == NULL ) {
		machine->num_reserved_cores = 0;
		machine->reserved_cores = NULL;
		//check for sane user input
		if( length != 0 ) {
			fprintf( stderr, "Error: mcbsp_set_reserved_cores called with NULL array while non-zero number of reserved cores was given!\n" );
			mcbsp_util_fatal();
		}
	}
	//set number of cores
	machine->num_reserved_cores = length;
	//allocate reserved array
	machine->reserved_cores = mcbsp_util_malloc( length * sizeof( size_t ), "mcbsp_set_reserved_cores reserved cores array (copy of user input)" );
	//copy
	for( size_t s = 0; s < length; ++s )
		machine->reserved_cores[ s ] = reserved[ s ];
	//set manual flag
	machine->Rset = true;
}

size_t mcbsp_get_maximum_threads( void ) {
        const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
        return machine->threads;
}

enum mcbsp_affinity_mode mcbsp_get_affinity_mode( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->affinity;
}

size_t mcbsp_get_available_cores( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->cores;
}
	
size_t mcbsp_get_threads_per_core( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->threads_per_core;
}

size_t mcbsp_get_unused_threads_per_core( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->unused_threads_per_core;
}

enum mcbsp_thread_numbering mcbsp_get_thread_numbering( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->thread_numbering;
}

size_t * mcbsp_get_pinning( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	//check boundary condition
	if( machine->manual_affinity == NULL ) {
		return NULL;
	}
	//not null, so allocate array copy
	const size_t size = machine->threads;
	size_t * ret = mcbsp_util_malloc( size * sizeof( size_t ), "mcbsp_get_pinning return array" );
	if( ret == NULL ) {
		fprintf( stderr, "Error: could not allocate copy of manual affinity array (size %lu)!\n", (unsigned long int)size );
		mcbsp_util_fatal();
	}
	//do copy
	for( size_t s = 0; s < size; ++s ) {
		ret[ s ] = machine->manual_affinity[ s ];
	}
	//do return
	return ret;
}

size_t mcbsp_get_reserved_cores_number( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	return machine->num_reserved_cores;
}

size_t * mcbsp_get_reserved_cores( void ) {
	const struct mcbsp_util_machine_info * const machine = mcbsp_internal_getMachineInfo();
	//check for NULL
	if( machine->reserved_cores == NULL ) {
		return NULL;
	}
	//otherwise create copy
	const size_t size = machine->num_reserved_cores;
	size_t * ret = mcbsp_util_malloc( size * sizeof( size_t ), "mcbsp_get_reserved_cores return array" );
	if( ret == NULL ) {
		fprintf( stderr, "Error: could not allocate copy of reserved cores array (size %lu)!\n", (unsigned long int)size );
		mcbsp_util_fatal();
	}
	//do copy
	for( size_t s = 0; s < size; ++s ) {
		ret[ s ] = machine->reserved_cores[ s ];
	}
	//do return
	return ret;
}

void mcbsp_checkpoint( void ) {
#if defined __MACH__ || defined _WIN32
	//unsupported OS!
	bsp_abort( "mcbsp_checkpoint(): current OS has no checkpointing support, aborting...\n" );
#else
 #ifdef MCBSP_WITH_DMTCP
	//get local data
	struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );
	
	//wait until everyone wants to checkpoint
  #ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_condition, (size_t)(data->bsp_id) );
  #else
	mcbsp_internal_sync( data->init, &(data->init->condition) );
  #endif

	//only a single process signals the DMTCP coordinator
	if( bsp_pid() == 0 ) {
		//I call DMTCP
		mcbsp_internal_call_checkpoint();
	}

	//wait until checkpoint/restore has completed
  #ifdef MCBSP_USE_SPINLOCK
	mcbsp_internal_spinlock( data->init, data->init->sl_end_condition, (size_t)(data->bsp_id) );
  #else
	mcbsp_internal_sync( data->init, &(data->init->end_condition) );
  #endif
	//done
 #else
	//compiled without DMTCP support!
	bsp_abort( "mcbsp_checkpoint(): MulticoreBSP was compiled without checkpointing support, aborting...\n" );
 #endif
#endif
} //end checkpoint

void mcbsp_enable_checkpoints( void ) {
#ifndef MCBSP_WITH_DMTCP
	fprintf( stderr, "Warning: mcbsp_enable_checkpoints() called but MulticoreBSP for C was not compiled with checkpointing support; ignoring call!\n" );
	return;
#endif
	//get data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();
	//set no checkpointing flag
	data->init->no_cp = false;
	//note this starts a data race, but this is actually what we want; the winner of the
	//race is the one who will call a checkpoint first, which he only correctly does do
	//if he was the one who set the no_cp flag. All other solutions require synching,
	//which we definitely do not want here.
}

void mcbsp_disable_checkpoints( void ) {
	//get data
	struct mcbsp_thread_data * const data = mcbsp_internal_prefunction();
	//set no checkpointing flag
	data->init->no_cp = true;
	//note this starts a data race, but this is actually what we want; the winner of the
	//race is the one who will call a checkpoint first, which he only correctly does not
	//do if he was the one who set the no_cp flag. All other solutions require synching,
	//which we definitely do not want here.
}

void mcbsp_set_checkpoint_frequency( const size_t _cp_f ) {
#ifndef MCBSP_WITH_DMTCP
	fprintf( stderr, "Warning: mcbsp_set_checkpoint_frequency(...) called but MulticoreBSP for C was not compiled with checkpointing support; ignoring call!\n" );
	return;
#endif
	//check if we are in SPMD mode
	mcbsp_internal_check_keys_allocated();
	struct mcbsp_thread_data * const tdata = pthread_getspecific( mcbsp_internal_thread_data );
	//first adapt the machine info
	struct mcbsp_util_machine_info * const mdata = mcbsp_internal_getMachineInfo();
	mdata->cp_f   = _cp_f;
	mdata->CPFset = true;
	//check if we also need to set the safe checkpointing frequency
	bool set_scp = false;
	if( mdata->SCFset == false ) {
		//then yes, we do need to do that
		mdata->safe_cp_f = _cp_f;
		set_scp = true;
	}
	//then check if we also need to adapt some run-time settings
	if( tdata != NULL ) {
		//yes, set run-time change
		tdata->init->cp_f = _cp_f;
		if( set_scp ) tdata->init->safe_cp_f = _cp_f;
	} else {
		//no, check if bsp_init was called
		struct mcbsp_init_data *sdata = pthread_getspecific( mcbsp_internal_init_data );
		if( sdata != NULL ) {
			//yes, set pre-bsp_begin change
			sdata->cp_f = _cp_f;
			if( set_scp ) sdata->safe_cp_f = _cp_f;
		}
	}
}

void mcbsp_set_safe_checkpoint_frequency( const size_t _safe_cp_f ) {
#ifndef MCBSP_WITH_DMTCP
	fprintf( stderr, "Warning: mcbsp_set_safe_checkpoint_frequency(...) called but MulticoreBSP for C was not compiled with checkpointing support; ignoring call!\n" );
	return;
#endif
	//check if we are in SPMD mode
	mcbsp_internal_check_keys_allocated();
	struct mcbsp_thread_data * const tdata = pthread_getspecific( mcbsp_internal_thread_data );
	//first adapt the machine info
	struct mcbsp_util_machine_info * const mdata = mcbsp_internal_getMachineInfo();
	mdata->safe_cp_f = _safe_cp_f;
	mdata->SCFset    = true;
	//then check if we also need to adapt some run-time settings
	if( tdata != NULL ) {
		//yes, set run-time change
		tdata->init->safe_cp_f = _safe_cp_f;
	} else {
		//no, check if bsp_init was called
		struct mcbsp_init_data *sdata = pthread_getspecific( mcbsp_internal_init_data );
		if( sdata != NULL ) {
			//yes, set pre-bsp_begin change
			sdata->safe_cp_f = _safe_cp_f;
		}
	}
}

size_t mcbsp_get_checkpoint_frequency( void ) {
#ifndef MCBSP_WITH_DMTCP
	fprintf( stderr, "Warning: mcbsp_get_checkpoint_frequency() called but MulticoreBSP for C was not compiled with checkpointing support; ignoring call!\n" );
	return 0;
#endif
	//necessary because this function may be called before bsp_init
	mcbsp_internal_check_keys_allocated();
	//first check thread-local data, as it may be cached
	const struct mcbsp_thread_data * const tdata = pthread_getspecific( mcbsp_internal_thread_data );
	if( tdata != NULL ) {
		//it's there, we're done
		return tdata->init->cp_f;
	} else {
		//if not, directly go back to machine info
		const struct mcbsp_util_machine_info * const mdata = mcbsp_internal_getMachineInfo();
		return mdata->cp_f;
	}
}

size_t mcbsp_get_safe_checkpoint_frequency( void ) {
#ifndef MCBSP_WITH_DMTCP
	fprintf( stderr, "Warning: mcbsp_get_safe_checkpoint_frequency() called but MulticoreBSP for C was not compiled with checkpointing support; ignoring call!\n" );
	return 0;
#endif
	//necessary because this function may be called before bsp_init
	mcbsp_internal_check_keys_allocated();
	//first check thread-local data, as it may be cached
	const struct mcbsp_thread_data * const tdata = pthread_getspecific( mcbsp_internal_thread_data );
	if( tdata != NULL ) {
		//it's there, we're done
		return tdata->init->safe_cp_f;
	} else {
		//if not, directly go back to machine info
		const struct mcbsp_util_machine_info * const mdata = mcbsp_internal_getMachineInfo();
		return mdata->safe_cp_f;
	}
}

void mcbsp_profiling_set_description( const char * const name ) {
	//check if we are in SPMD mode
	mcbsp_internal_check_keys_allocated();
	struct mcbsp_thread_data * const tdata = pthread_getspecific( mcbsp_internal_thread_data );
	//set name
	strncpy( tdata->superstep_stats.name, name, 255 );
}

//internally-used functions:

struct mcbsp_util_machine_info *mcbsp_internal_getMachineInfo( void ) {
	//check if keys are allocated
	mcbsp_internal_check_keys_allocated();
	//get from pthreads
	struct mcbsp_util_machine_info * ret = pthread_getspecific( mcbsp_local_machine_info );
	//if unsuccessful create new and set
	if( ret == NULL ) {
		ret = mcbsp_util_createMachineInfo();
		pthread_setspecific( mcbsp_local_machine_info, ret );
	}
	//return current machine info
	return ret;
}	

void bsp_init_internal( struct mcbsp_init_data * const initialisationData ) {
	//store using pthreads setspecific. Note this is per BSP program, not per thread
	//active within this BSP program!
	mcbsp_internal_check_keys_allocated();
	if( pthread_getspecific( mcbsp_internal_init_data ) != NULL ) {
		const struct mcbsp_init_data * const oldData = pthread_getspecific( mcbsp_internal_init_data );
		if( !oldData->ended ) {
			fprintf( stderr, "Warning: initialisation data corresponding to another BSP run found;\n" );
			fprintf( stderr, "         and this other run did not terminate (gracefully).\n" );
		}
	}
	if( pthread_setspecific( mcbsp_internal_init_data, initialisationData ) != 0 ) {
		fprintf( stderr, "Error: could not set BSP program key!\n" );
		mcbsp_util_fatal();
	}
}

void mcbsp_internal_check_keys_allocated( void ) {
	//if already allocated, we are done
	if( mcbsp_internal_keys_allocated ) return;

	//lock mutex against data race
	pthread_mutex_lock( &mcbsp_internal_keys_mutex );

	//if still not allocated, allocate
	if( !mcbsp_internal_keys_allocated ) {
		if( pthread_key_create( &mcbsp_internal_init_data, free ) != 0 ) {
			fprintf( stderr, "Could not allocate mcbsp_internal_init_data key!\n" );
			mcbsp_util_fatal();
		}
		if( pthread_key_create( &mcbsp_internal_thread_data, free ) != 0 ) {
			fprintf( stderr, "Could not allocate mcbsp_internal_thread_data key!\n" );
			mcbsp_util_fatal();
		}
		if( pthread_key_create( &mcbsp_local_machine_info, mcbsp_util_destroyMachineInfo ) != 0 ) {
			fprintf( stderr, "Could not allocate mcbsp_local_machine_info key!\n" );
			mcbsp_util_fatal();
		}
		if( pthread_setspecific( mcbsp_internal_init_data, NULL ) != 0 ) {
			fprintf( stderr, "Could not initialise mcbsp_internal_init_data to NULL!\n" );
			mcbsp_util_fatal();
		}
		if( pthread_setspecific( mcbsp_internal_thread_data, NULL ) != 0 ) {
			fprintf( stderr, "Could not initialise mcbsp_internal_thread_data to NULL!\n" );
			mcbsp_util_fatal();
		}
		if( pthread_setspecific( mcbsp_local_machine_info, NULL ) != 0 ) {
			fprintf( stderr, "Could not initialise mcbsp_local_machine_info to NULL!\n" );
			mcbsp_util_fatal();
		}
		mcbsp_internal_keys_allocated = true;
	}

	//unlock mutex and exit
	pthread_mutex_unlock( &mcbsp_internal_keys_mutex );
}

void* mcbsp_internal_spmd( void *p ) {
	//get thread-local data
	struct mcbsp_thread_data *data = (struct mcbsp_thread_data *) p;

	//reallocate thread-local data and initialise communication queues
	data = mcbsp_internal_initialise_thread_data( data );

	//provide a link back from the initialising process' data
	data->init->threadData[ data->bsp_id ] = data;

	//store thread-local data
	const int rc = pthread_setspecific( mcbsp_internal_thread_data, data );
	if( rc != 0 ) {
		fprintf( stderr, "Could not store thread local data!\n" );
		fprintf( stderr, "(%s)\n", strerror( rc ) );
		mcbsp_util_fatal();
	}

#ifdef __MACH__
	//get rights for accessing Mach's timers
	const kern_return_t rc1 = host_get_clock_service( mach_host_self(), SYSTEM_CLOCK, &(data->clock) );
	if( rc1 != KERN_SUCCESS ) {
		fprintf( stderr, "Could not access the Mach system timer (%s)\n", mach_error_string( rc1 ) );
		mcbsp_util_fatal();
	}

	//record start time
	const kern_return_t rc2 = clock_get_time( data->clock, &(data->start) );
	if( rc2 != KERN_SUCCESS ) {
		fprintf( stderr, "Could not get start time (%s)\n", mach_error_string( rc2 ) );
		mcbsp_util_fatal();
	}
#elif defined _WIN32
	//record start time
	QueryPerformanceCounter( &(data->start) );
	//record frequency
	QueryPerformanceFrequency( &(data->frequency) );
#else
	//record start time
	clock_gettime( CLOCK_MONOTONIC, &(data->start) );
#endif

	//set default superstep profiling name, initialise other
	//profiling fields
	mcbsp_internal_init_superstep_stats( &(data->superstep_stats), 0 );
	data->superstep_stats.buffering = 0;
	data->superstep_stats.direct_get_bytes = 0;

	//record start time of the coming compute phase
	data->superstep_start_time = bsp_time();

	//continue with SPMD part
	if( data->init->spmd == NULL ) {
		main( 0, NULL ); //we had an implicit bsp_init
	} else {
		data->init->spmd(); //call user-defined SPMD program
	}

	//exit cleanly
	return NULL;
}

void mcbsp_internal_check_aborted( void ) {
	const struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );
	//note this function can implicitly be called by bsp_abort;
	//bsp_abort can be called from non-SPMD sections. Hence if no local thread data,
	//just exit neatly.
	if( data != NULL ) {
		if( data->init != NULL ) {
			if( data->init->abort ) {
				pthread_exit( NULL );
			}
		} else {
			fprintf( stderr, "Local thread data found, but no init data encountered!\n" );
			mcbsp_util_fatal();
		}
	}
	//in all other cases, not sure whether or not to fail; simply return
	return;
}

void mcbsp_internal_spinlock( struct mcbsp_init_data * const init, unsigned char * const condition, const size_t bsp_id ) {
	//set our condition go
	const unsigned char sync_number = ++(condition[ bsp_id ]);

#ifdef MCBSP_NO_CC
	mcbsp_nocc_flush_cacheline( condition + bsp_id );
#endif

	//check if all are go
	bool go;
	do {
#ifdef MCBSP_NO_CC
		//brute force dealing with cache coherency, disabled in favour 
		//of the current implementation that invalidates only the parts
		//of the memory the spinlock relies on
		//mcbsp_nocc_purge_all();
#endif
		//assume we can leave the spin-lock, unless...
		go = true;
		for( size_t s = 0; s < init->P; ++s ) {
#ifdef MCBSP_NO_CC
			//warning: hardcoded for 64-byte cache lines!
			if( s % mcbsp_nocc_cache_line_size() == 0 ) {
				//invalidate this new cache line
				mcbsp_nocc_invalidate_cacheline( condition + s );
			}
#endif
			//if the condition is sync_number, then s is at my superstep
			//if the condition is sync_number+1, then s is at the next superstep
			//in both cases, s is OK with us proceeding into the next superstep
			//therefore, if neither is true, we should stay in the spinlock.
			if( !(init->abort) && condition[ s ] != sync_number && condition[ s ] != (unsigned char)(sync_number + 1) ) {
				go = false;
				break;
			}
		}
	} while( !go );

	//sync complete, check for abort condition on exit
	mcbsp_internal_check_aborted();

	//all is OK, continue execution
}

void mcbsp_internal_sync( struct mcbsp_init_data * const init, pthread_cond_t *condition ) {
	//get lock
	pthread_mutex_lock( &(init->mutex) );

	//increment counter & check if we are the last one to sync
	if( init->sync_entry_counter++ == init->P - 1 ) {
		//if so, reset counter
		init->sync_entry_counter = 0;
		//wake up other threads
		pthread_cond_broadcast( condition );
	} else {
		//check if execution was aborted just before we got the lock
		if( !init->abort ) { //if not, enter wait
			pthread_cond_wait( condition, &(init->mutex) );
		}
	}

	//synchronisation is done, unlock mutex
	pthread_mutex_unlock( &(init->mutex) );

	//before continuing execution, check if we woke up due to an abort
	//and now exit if so (we could not exit earlier as not unlocking the
	//sync mutex will cause a deadlock).
	mcbsp_internal_check_aborted();

	//all is OK, return
}

const struct mcbsp_thread_data * mcbsp_internal_const_prefunction( void ) {
	return mcbsp_internal_prefunction();
}

struct mcbsp_thread_data * mcbsp_internal_prefunction( void ) {
	//check if the BSP execution was aborted
	mcbsp_internal_check_aborted();

	//get thread-local data
	struct mcbsp_thread_data * const data = pthread_getspecific( mcbsp_internal_thread_data );

	//check for errors if not in high-performance mode
#ifndef MCBSP_NO_CHECKS
	if( data == NULL ) {
		fprintf( stderr, "Error: could not get thread-local data in call to mcbsp_internal_prefunction!\n" );
		mcbsp_util_fatal();
	}
#endif

	//return data
	return data;
}

struct mcbsp_thread_data * mcbsp_internal_allocate_thread_data( struct mcbsp_init_data * const init, const size_t s ) {
	//allocate new thread-local data
	struct mcbsp_thread_data *thread_data = mcbsp_util_malloc( sizeof( struct mcbsp_thread_data ), "mcbsp_internal_allocate_thread_data thread data struct" );
	//provide a link to the SPMD program init struct
	thread_data->init   = init;
	//set local ID
	thread_data->bsp_id = s;
	//set the maximum number of registered globals at any time (0, since SPMD not started yet)
	thread_data->localC = 0;
	//initialise default tag size
	thread_data->newTagSize = 0;
	//return newly constructed and partially initialised thread_data struct
	return thread_data;
}

struct mcbsp_thread_data * mcbsp_internal_initialise_thread_data( struct mcbsp_thread_data * const data ) {
	//allocate new thread-local data
	struct mcbsp_thread_data *thread_data = mcbsp_util_malloc( sizeof( struct mcbsp_thread_data ), "mcbsp_internal_initialise_thread_data thread data struct" );

	//copy initial thread data, for better locality on NUMA systems
	*thread_data = *data;

	//initialise non-copyable/plain-old-data fields
	const size_t P = (size_t)(thread_data->init->P);

	//allocate stack arrays used for communication
	thread_data->request_queues = mcbsp_util_malloc( P * sizeof( struct mcbsp_util_stack ), "mcbsp_internal_initialise_thread_data communication request stack array" );
	thread_data->queues         = mcbsp_util_malloc( P * sizeof( struct mcbsp_util_stack ), "mcbsp_internal_initialise_thread_data communication stack array" );
	thread_data->hpsend_queues  = mcbsp_util_malloc( P * sizeof( struct mcbsp_util_stack ), "mcbsp_internal_initialise_thread_data hp-communication stack array" );
	//initialise stacks
	for( size_t i = 0; i < P; ++i ) {
		mcbsp_util_stack_initialise( &(thread_data->request_queues[ i ]), sizeof( struct mcbsp_get_request ) );
		mcbsp_util_stack_initialise( &(thread_data->queues[ i ]),         sizeof( struct mcbsp_message) );
		mcbsp_util_stack_initialise( &(thread_data->hpsend_queues[ i ]),  sizeof( struct mcbsp_hpsend_request) );
	}

	//initialise local to global map
	mcbsp_util_address_map_initialise( &(thread_data->local2global ) );

	//initialise stack used for hp communication
	mcbsp_util_stack_initialise( &(thread_data->hpdrma_queue), sizeof( struct mcbsp_hp_request ) );

	//initialise BSMP queue
	mcbsp_util_stack_initialise( &(thread_data->bsmp), sizeof( size_t ) );

	//initialise profile vector
	mcbsp_util_stack_initialise( &(thread_data->profile), sizeof( struct mcbsp_superstep_stats ) );

	//initialise stack used for efficient registration of globals after de-registrations
	mcbsp_util_stack_initialise( &(thread_data->removedGlobals), sizeof( unsigned long int ) );

	//initialise stack used for de-registration of globals
	mcbsp_util_stack_initialise( &(thread_data->localsToRemove), sizeof( void * ) );

	//initialise push request queue
	mcbsp_util_stack_initialise( &(thread_data->localsToPush), sizeof( struct mcbsp_push_request ) );

	//initialise the map-search stack
	mcbsp_util_stack_initialise( &(thread_data->globalsToPush), sizeof( unsigned long int ) );

	//free original thread_data
	free( data );

	//return new completely local thread data
	return thread_data;
}

void mcbsp_internal_destroy_thread_data( struct mcbsp_thread_data * const data ) {
#ifdef __MACH__
	mach_port_deallocate( mach_task_self(), data->clock );
#endif
	for( size_t s = 0; s < data->init->P; ++s ) {
		mcbsp_util_stack_destroy( &(data->request_queues[ s ]) );
		mcbsp_util_stack_destroy( &(data->queues[ s ]) );
		mcbsp_util_stack_destroy( &(data->hpsend_queues[ s ]) );
	}
	free( data->request_queues );
	free( data->queues );
	free( data->hpsend_queues );
	mcbsp_util_address_map_destroy( &(data->local2global) );
	mcbsp_util_stack_destroy( &(data->hpdrma_queue) );
	mcbsp_util_stack_destroy( &(data->bsmp) );
	mcbsp_util_stack_destroy( &(data->profile) );
	mcbsp_util_stack_destroy( &(data->removedGlobals) );
	mcbsp_util_stack_destroy( &(data->localsToRemove) );
	mcbsp_util_stack_destroy( &(data->localsToPush) );
	mcbsp_util_stack_destroy( &(data->globalsToPush) );
	free( data );
}

#ifdef MCBSP_WITH_DMTCP
int mcbsp_internal_call_checkpoint( void ) {
	const int rc = dmtcp_checkpoint();
	int ret = 0; //assume everything is all right

	//check sanity
	if( rc == DMTCP_AFTER_RESTART ) {
		ret =  1; //we are recovering, not checkpointing
	} else if( rc == DMTCP_NOT_PRESENT ) {
		ret = -1; //everything is not all right
		bsp_abort( "mcbsp_internal_call_checkpoint(): not connected to a DMTCP coordinator, aborting...\n" );
	} else if( rc != DMTCP_AFTER_CHECKPOINT ) {
		ret = -1; //everything is not all right
		bsp_abort(
			"mcbsp_internal_call_checkpoint(): unexpected DMTCP return code (%d); %d (DMTCP_AFTER_CHECKPOINT) was expected. Checkpointing likely failed, aborting...\n",
			rc,
			DMTCP_AFTER_CHECKPOINT
		);
	}
	return ret;
}
#else
int mcbsp_internal_call_checkpoint( void ) {
	bsp_abort( "mcbsp_internal_call_checkpoint called but MulticoreBSP for C was compiled without checkpointing support! Aborting...\n" );
	return -1; //return error code
}
#endif

void mcbsp_internal_update_checkpointStats( struct mcbsp_init_data * const init ) {
	//only skip a checkpoint when checkpointing is enabled
	if( !(init->no_cp) ) {
		++(init->skipped_checkpoints);
	}
	//when we reach safe checkpointing frequency, check if we need to switch to safe checkpointing
	//..or switch back again!
	if(
		init->safe_cp_f > 0 &&
		(init->skipped_checkpoints % init->safe_cp_f) == 0
	) {
		//see if /etc/bsp_failure exists
		const int rc = access( "/etc/bsp_failure", F_OK );
		if( rc == 0 ) {
			//then yes, failure is imminent! Check if that is new
			if( !(init->safe_cp) ) {
				//yes, so switch states
				init->safe_cp = true;
				//print warning
				fprintf(
					stderr,
					"Warning: MulticoreBSP for C enters safe checkpointing mode (with frequency %zd). Reasons follow:\n",
					init->safe_cp_f
				);
				//open bsp_failure file
				FILE * const fd = fopen( "/etc/bsp_failure", "r" );
				//check if opened OK
				if( fd != NULL ) {
					//seek end of file
					if( fseek( fd, 0, SEEK_END ) == 0 ) {
						//seek successful, get and print out last two lines
						//get current file position
						long int pos = ftell( fd ) - 1;
						//print out the last two lines
						for( unsigned char line = 0; line < 2; ++line ) {
							//print header
							fprintf( stderr, "    /etc/bsp_failure: " );
							//walk until previous line
							long int walk = pos - 1;
							fseek( fd, walk, SEEK_SET );
							int current = fgetc( fd );
							while( walk >= 0 && current != EOF && current != '\n' ) {
								--walk;
								fseek( fd, walk, SEEK_SET );
								current = fgetc( fd );
							}
							//print out everything between walk and pos
							long int i = walk + 1;
							while( i < pos ) {
								fseek( fd, i, SEEK_SET );
								current = fgetc( fd );
								if( current != EOF ) {
									fputc( current, stderr );
								}
								++i;
							}
							//rewind back to start of this line
							pos = walk;
							//print newline
							fputc( '\n', stderr );
						}
					}
					//close file
					fclose( fd );
				} else {
					//could not read /etc/bsp_failure
					fprintf( stderr, "\t(reason unknown: could not read /etc/bsp_failure)\n" );
				}
			}
		} else {
			//otherwise we are in regular checkpointing mode
			init->safe_cp = false;
		}
	}
	//done
}

void mcbsp_internal_print_profile( const struct mcbsp_init_data * const init ) {
	struct mcbsp_thread_data * const * const threads = init->threadData;
	const size_t P = init->P;

	//record start time
	const double start = bsp_time();

	//first check if there was any profile
	bool profiled = false;
	for( size_t k = 0; k < P; ++k ) {
		if( !mcbsp_util_stack_empty( &(threads[ k ]->profile) ) ) {
			profiled = true;
			break;
		}
	}
	if( !profiled ) {
		return;
	}

	//yes, so give output. First do sanity checking
	const size_t N = threads[ 0 ]->profile.top;
	for( size_t k = 1; k < P; ++k ) {
		if( threads[ k ]->profile.top != N ) {
			fprintf(
				stderr,
				"Profiling mode did not record equal number of supersteps on each SPMD process (PID 0 has %zd, while PID %zd has %zd! Aborting profiling analysis...\n",
				N, k, threads[ k ]->profile.top
			);
			return;
		}
	}

	//print header
	printf( "\n=====================\n" );
	printf( "SPMD profiling report\n" );
	printf( "=====================\n\n" );
	printf( "Number of supersteps: %zd.\n", N );

	//go per superstep
	for( size_t i = 0; i < N; ++i ) {
		//copy struct from BSP PID 0
		struct mcbsp_superstep_stats p0 = 
			*( ((const struct mcbsp_superstep_stats *)(threads[ 0 ]->profile.array)) + i );
		//sanity checks
		assert( p0.bytes_sent > p0.metabytes_sent );
		assert( p0.bytes_received > p0.metabytes_received );
		//this superstep's h-relation
		size_t h =
			p0.bytes_sent > p0.bytes_received ?
			p0.bytes_sent :
			p0.bytes_received;
		size_t user_h = 
			(p0.bytes_sent - p0.metabytes_sent >
			 p0.bytes_received - p0.metabytes_received) ?
			 p0.bytes_sent - p0.metabytes_sent :
			 p0.bytes_received - p0.metabytes_received;
		double true_compute_time = p0.computation - p0.buffering;
		//reduce all other SPMD processes into p0
		for( size_t k = 1; k < P; ++k ) {
			const struct mcbsp_superstep_stats pk =
				*( ((const struct mcbsp_superstep_stats*)(threads[k]->profile.array)) + i );
			//sanity checks
			assert( pk.bytes_sent     > pk.metabytes_sent );
			assert( pk.bytes_received > pk.metabytes_received );
			if( strncmp( p0.name, pk.name, 255 ) != 0 ) {
				fprintf( stderr, "Warning: profile names at superstep %zd are not equal (%s vs %s); I will take the former and continue profiling.\n", i, p0.name, pk.name );
			}
			//get local h
			const size_t max_h_is =
				pk.bytes_sent > pk.bytes_received ?
				pk.bytes_sent :
				pk.bytes_received;
			const size_t user_max_h_is = 
				(pk.bytes_sent     - pk.metabytes_sent > 
				 pk.bytes_received - pk.metabytes_received) ?
				 pk.bytes_sent     - pk.metabytes_sent :
				 pk.bytes_received - pk.metabytes_received;
			//update global h
			if( h < max_h_is ) {
				h = max_h_is;
			}
			if( user_h < user_max_h_is ) {
				user_h = user_max_h_is;
			}
			//reduce by addition
			p0.put                += pk.put;
			p0.get                += pk.get;
			p0.hpput              += pk.hpput;
			p0.hpget              += pk.hpget;
			p0.send               += pk.send;
			p0.hpsend             += pk.hpsend;
			p0.direct_get         += pk.direct_get;
			p0.move               += pk.move;
			p0.hpmove             += pk.hpmove;
			p0.bookkeeping       += pk.bookkeeping;
			p0.non_communicating  += pk.non_communicating;
			p0.bytes_buffered     += pk.bytes_buffered;
			p0.bytes_sent         += pk.bytes_sent;
			p0.bytes_received     += pk.bytes_received;
			p0.metabytes_sent     += pk.metabytes_sent;
			p0.metabytes_received += pk.metabytes_received;
			//reduce by maximum
			if( p0.buffering < pk.buffering ) {
				p0.buffering = pk.buffering;
			}
			if( p0.computation < pk.computation ) {
				p0.computation = pk.computation;
			}
			if( p0.communication < pk.communication ) {
				p0.communication = pk.communication;
			}
			if( true_compute_time < pk.computation - pk.buffering ) {
				true_compute_time = pk.computation - pk.buffering;
			}
		}
		//derive imbalance (if any)
		size_t max_imbalance = 0;
		for( size_t k = 0; k < P; ++k ) {
			const struct mcbsp_superstep_stats pk =
				*( ((const struct mcbsp_superstep_stats*)(threads[k]->profile.array)) + i );
			const size_t max_h_is =
				pk.bytes_sent > pk.bytes_received ?
				pk.bytes_sent :
				pk.bytes_received;
			assert( h >= max_h_is );
			const size_t cur_imb = h - max_h_is;
			if( max_imbalance < cur_imb ) {
				max_imbalance = cur_imb;
			}
		}
		//give output
		printf( "\n*** %s ***\n", p0.name );
		printf( "maximum time spent in computation phase   = %lf seconds\n", p0.computation );
		printf( "maximum time spent in buffering           = %lf seconds\n", p0.buffering );
		printf( "maximum time in non-buffering computation = %lf seconds\n", true_compute_time );
		printf( "maximum time spent in communication phase = %lf seconds\n", p0.communication );
		printf( "\nh-relation (in bytes, with metadata) = %zd\n", h );
		printf(   "maximum imbalance (in bytes)         = %zd\n", max_imbalance );
		printf(   "true h-relation (in bytes)           = %zd\n", user_h );
		printf( "\ntotal number of bytes sent          = %zd\n", p0.bytes_sent );
		printf(   "total number of bytes received      = %zd\n", p0.bytes_received );
		printf(   "total number of meta-data sent      = %zd\n", p0.metabytes_sent );
		printf(   "total number of meta-data received  = %zd\n", p0.metabytes_received );
		printf(   "total number of bytes buffered      = %zd\n", p0.bytes_buffered );
		printf(   "total number of bytes in direct_get = %zd\n", p0.direct_get_bytes );
		printf( "\ntotal number of DRMA calls:\n" );
		printf( "    -       bsp_put: %zd\n", p0.put );
		printf( "    -       bsp_get: %zd\n", p0.get );
		printf( "    -     bsp_hpput: %zd\n", p0.hpput );
		printf( "    -     bsp_hpget: %zd\n", p0.hpget );
		printf( "    -bsp_direct_get: %zd\n", p0.direct_get );
		printf( "\ntotal number of BSMP calls:\n" );
		printf( "    -      bsp_send: %zd\n", p0.send );
		printf( "    -    bsp_hpsend: %zd\n", p0.hpsend );
		printf( "    -      bsp_move: %zd\n", p0.move );
		printf( "    -    bsp_hpmove: %zd\n\n", p0.hpmove );
		printf( "total number of memory registration calls (bsp_push_reg, bsp_pop_reg, bsp_set_tagsize): %zd\n", p0.bookkeeping );
		printf( "total number of calls to non-communicating BSP primitives (bsp_pid, bsp_nprocs, bsp_time, bsp_qsize, bsp_get_tag): %zd\n", p0.non_communicating );
	}

	//print footer
	const double time_taken = bsp_time() - start;
	printf( "\nEnd profile. This report was generated in %lf seconds.\n", time_taken );
}

void mcbsp_internal_init_superstep_stats( struct mcbsp_superstep_stats * const stats, const size_t superstep ) {
	snprintf( stats->name, 255, "Superstep %zd", superstep );
	stats->put = stats->get = stats->hpput = stats->hpget = stats->send =
	stats->hpsend = stats->direct_get = stats->move = stats->hpmove = 
	stats->bookkeeping = stats->non_communicating = stats->bytes_buffered = 
	stats->direct_get_bytes = stats->bytes_sent = stats->bytes_received = 
	stats->metabytes_sent = stats->metabytes_received = 0;
	stats->buffering = stats->computation = stats->communication = 0;
}

double mcbsp_internal_time( struct mcbsp_thread_data * const data ) {
#if !defined __MACH__ && !defined _WIN32
	//get stop time
	struct timespec stop;
	clock_gettime( CLOCK_MONOTONIC, &stop);
#endif

#ifdef __MACH__
	//get rights for accessing Mach's timers
	const kern_return_t rc1 = host_get_clock_service( mach_host_self(), SYSTEM_CLOCK, &(data->clock) );
 #ifndef MCBSP_NO_CHECKS
	if( rc1 != KERN_SUCCESS ) {
		fprintf( stderr, "Could not access the Mach system timer (%s)\n", mach_error_string( rc1 ) );
		mcbsp_util_fatal();
	}
 #endif
	//get stop time
	mach_timespec_t stop;
	const kern_return_t rc2 = clock_get_time( data->clock, &stop );
 #ifndef MCBSP_NO_CHECKS
	if( rc2 != KERN_SUCCESS ) {
		fprintf( stderr, "Could not get time at call to bsp_time (%s)\n", mach_error_string( rc2 ) );
		mcbsp_util_fatal();
	}
 #endif
#elif defined _WIN32
	LARGE_INTEGER stop;
 #ifndef MCBSP_NO_CHECKS
	if( !QueryPerformanceCounter( &stop ) ) {
		fprintf( stderr, "Could not get time at call to bsp_time!\n" );
		mcbsp_util_fatal();
	}
 #endif
#endif

	//return elapsed time
#ifdef _WIN32
	stop.QuadPart -= data->start.QuadPart;
	return stop.QuadPart / ((double)(data->frequency.QuadPart));
#else
	double time = (stop.tv_sec - data->start.tv_sec);
	time += (stop.tv_nsec-data->start.tv_nsec)/1000000000.0;
	return time;
#endif
}

