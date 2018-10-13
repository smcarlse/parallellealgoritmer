/*
 * Copyright (c) 2012
 *
 * File created 2012 by A. N. Yzelman by refactoring a much
 * older header file (2007-2009).
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

#ifndef _H_MCINTERNAL
#define _H_MCINTERNAL

//this include has to come first,
//as it defines compilation modes
#include "mcbsp-internal-macros.h"

//public MulticoreBSP includes:
#include "bsp.h"
#include "mcbsp-affinity.h"
#include "mcbsp-profiling.h"
#include "mcbsp-resiliency.h"

//hidden includes (from library user's perspective):

//cross-platform includes:
#ifdef __MACH__
 #include <mach/mach.h>
 #include <mach/clock.h>
 #include <mach/mach_error.h>
 #include <mach/thread_policy.h>
#endif

#ifdef _WIN32
 #include <windows.h>
#endif

#ifdef MCBSP_WITH_DMTCP
 #include <dmtcp.h>
#endif

//common includes:
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>

#include "mcutil.h"


/** To support direct SPMD mode, created threads may call main(). */
extern int main( int argc, char **argv );

/** Struct used to store tracked of superstep statistics. */
struct mcbsp_superstep_stats {

	/** 
	 * Stores a descriptive name for the superstep this is storing
	 * statistics of.
	 */
	char name[ 255 ];

	/** Stores the recorded number of calls to bsp_put */
	size_t put;

	/** Stores the recorded number of calls to bsp_get */
	size_t get;

	/** Stores the recorded number of calls to bsp_hpput */
	size_t hpput;

	/** Stores the recorded number of calls to bsp_hpget */
	size_t hpget;

	/** Stores the recorded number of calls to bsp_send */
	size_t send;

	/** Stores the recorded number of calls to bsp_hpsend */
	size_t hpsend;

	/** Stores the recorded number of calls to bsp_direct_get */
	size_t direct_get;

	/** Stores the recorded number of calls to bsp_move */
	size_t move;

	/** Stores the recorded number of calls to bsp_hpmove */
	size_t hpmove;

	/**
	 * Stores the recorded number of calls to inter-process bookkeeping 
	 * primitives, namely
	 * 	-bsp_push_reg
	 * 	-bsp_pop_reg
	 * 	-bsp_set_tagsize
	 */
	size_t bookkeeping;

	/**
	 * Stores the recorded number of calls to non-communication primitives,
	 * namely
	 *      -bsp_pid
	 * 	-bsp_nprocs
	 * 	-bsp_time
	 * 	-bsp_qsize
	 * 	-bsp_get_tag
	 */
	size_t non_communicating;

	/**
	 * Stores the number of bytes locally buffered.
	 */
	size_t bytes_buffered;

	/**
	 * Stores the number of bytes transferred using bsp_direct_get.
	 */
	size_t direct_get_bytes;

	/**
	 * Stores the number of bytes sent out during communication. This
	 * includes data movement into the same process.
	 */
	size_t bytes_sent;

	/**
	 * Stores the number of bytes received during communication. This
	 * includes data movement into the same process.
	 */
	size_t bytes_received;

	/**
	 * Stores how many bytes of bytes_sent were spent on meta-data,
	 * instead of actual payloads.
	 */
	size_t metabytes_sent;

	/**
	 * Stores how many bytes of bytes_received were spent on meta-data,
	 * instead of actual payloads.
	 */
	size_t metabytes_received;

	/** Stores the time taken in buffering during computation phases. */
	double buffering;

	/** Stores the time taken in the computation phase. */
	double computation;

	/** Stores the time taken in the communication phase. */
	double communication;

};

/**
 * Initialisation struct.
 * 
 * Contains information necessary to start an SPMD
 * section, and contains fields necessary for global
 * actions during the corresponding SPMD section.
 *
 * Can be stored at two locations:
 * 1: at the initialising thread of an SPMD section,
 *    when the SPMD section has not yet been started;
 * 2: as a pointer through thread-local data once the
 *    SPMD section has started.
 *
 * The field ordering of the struct are that alignable
 * fields are placed first (pointers, size_ts), followed
 * by arbitrary structs / typedef (unpredictable size),
 * and finally smaller-sized fields (ints, bools).
 * Within levels, the ordering is semantic (arbitrary).
 */
struct mcbsp_init_data {

	/** Number of processors involved in this run. */
	size_t P;

	/**
	 * Barrier counter for this BSP execution.
	 * Synchronises synchronisation entry.
	 */
	size_t sync_entry_counter;

	/**
	 * Barrier counter for this BSP program.
	 * Synchronises synchronisation exit.
	 */
	size_t sync_exit_counter;

	/** Currently active tag size. */
	size_t tagSize;

	/**
	 * ID of the top-level run, if available.
	 *
	 * This value is SIZE_MAX if this is the top-level run. This value is
	 * kept separate from prev_data as the top-level run need not be a 
	 * MulticoreBSP for C run (we may have been offloaded or called 
	 * recursively from a distributed-memory SPMD setting, both of which
	 * may not even have anything to do with BSP).
	 */
	size_t top_pid;

	/** Currently active checkpointing frequency. */
	size_t cp_f;

	/** Currently active safe checkpointing frequency. */
	size_t safe_cp_f;

	/**
	 * Current number of skipped checkpoints.
	 * I.e., when doing automatic checkpointing, we checkpoint whenever
	 * skipped_checkpoints == cp_f in normal automatic checkpointing mode
	 * (if cp_f > 0, and no checkpointing otherwise), or whenever
	 * skipped_checkpoints == safe_cp_f when in safe checkpointing mode
	 * (i.e., in the case of imminent hardware failures).
	 */
	size_t skipped_checkpoints;

	/** Current superstep of the SPMD program. */
	size_t current_superstep;

	/** Threads corresponding to this BSP program. */
	pthread_t *threads;

	/** Pointers to all thread-local data, as needed for communication. */
	struct mcbsp_thread_data * * threadData;

	/**
	 * Condition used for critical sections.
	 *
	 * Only used if compiled with MCBSP_USE_SPINLOCK, but always
	 * included for binary compatibility!
	 */
	unsigned char * sl_condition;

	/**
	 * Condition used for critical sections.
	 *
	 * Only used if compiled with MCBSP_USE_SPINLOCK, but always
	 * included for binary compatibility!
	 */
	unsigned char * sl_mid_condition;

	/**
	 * Condition used for critical section. See also sl_condition
	 * and sl_mid_condition.
	 */
	unsigned char * sl_end_condition;

	/** Stores any previous thread-local data. Used for nested runs. */
	struct mcbsp_thread_data * prev_data;

	/** User-definied SPMD entry-point. */
	void (*spmd)(void);

	/** 
	 * In case of a call from the C++ wrapper,
	 * pointer to the user-defined BSP_program
	 */
	void *bsp_program;

	/** Passed argv from bsp_init. */
	char **argv;

	/** Address table used for inter-thread communication. */
	struct mcbsp_util_address_table global2local;

	/** Mutex used for critical sections (such as synchronisation). */
	pthread_mutex_t mutex;

	/**
	 * Condition used for critical sections.
	 *
	 * This is the default POSIX threads synchronisation mechanism,
	 * which may not be used in case compilitaion opted for alternative
	 * mechanisms (e.g., spinlocking via MCBSP_USE_SPINLOCK). This
	 * field is always included for binary compatibility, however!
	 */
	pthread_cond_t condition;

	/**
	 * Condition used for critical sections.
	 *
	 * This is the default POSIX threads synchronisation mechanism,
	 * which may not be used in case compilitaion opted for alternative
	 * mechanisms (e.g., spinlocking via MCBSP_USE_SPINLOCK). This
	 * field is always included for binary compatibility, however!
	 */
	pthread_cond_t mid_condition;

	/**
	 * Condition used for critical sections.
	 * See also condition and mid_condition.
	 */
	pthread_cond_t end_condition;

	/** Passed argc from bsp_init. */
	int argc;

	/** Whether the BSP program should be aborted. */
	volatile bool abort;

	/** Whether the BSP program has ended. */
	bool ended;

	/** Whether checkpointing has been explicitly disallowed. */
	bool no_cp;

	/** 
	 * Whether hardware failures were detected, or were detected to be
	 * imminent.
	 */
	bool safe_cp;
	
};

/**
 * Thread-local data.
 *
 * Contains data local to each thread in an SPMD section.
 */
struct mcbsp_thread_data {

	/** Thread-local BSP id. */
	size_t bsp_id;

	/**
	 * Stores the tag size to become active after the
	 * next synchronisation.
	 */
	size_t newTagSize;

	/**
	 * Counts the maximum number of registered variables
	 * at any one time.
	 */
	size_t localC;

	/**
	 * The value of bsp_time at the start of the current computation
	 * phase.
	 */
	double superstep_start_time;
	
	/** Initialisation data. */
	struct mcbsp_init_data * init;

	/**
	 * The communication queues used for bsp_get
	 * requests.
	 */
	struct mcbsp_util_stack * request_queues;

	/**
	 * The communication queues used for all DRMA
	 * and BSMP communication.
	 */
	struct mcbsp_util_stack * queues;

	/** 
	 * The communication queues used for bsp_hpsend
	 * requests. Kept separately from bsp_hpget and
	 * bsp_hpput queues since they are handled quite
	 * differently.
	 */
	struct mcbsp_util_stack * hpsend_queues;

	/** Local address to global variable map. */
	struct mcbsp_util_address_map local2global;

	/**
	 * The communication queue for hpget and hpput
	 * requests. There is only a single queue per
	 * thread, since only the active threads knows
	 * when all requests to-be are made and thus
	 * when the communication can start.
	 * This is what makes hp-primitives unsafe; 
	 * overlapping or neighbouring memory areas can
	 * cause data races and false sharing conditions.
	 * The main advantage of using the hp queue is
	 * that no payload data will be cached.
	 */
	struct mcbsp_util_stack hpdrma_queue;

	/** The BSMP incoming message queue. */
	struct mcbsp_util_stack bsmp;

	/** Where to store any profiling statistics. */
	struct mcbsp_util_stack profile;

	/**
	 * Keeps track of which global variables are removed
	 * (as per bsp_pop_reg).
	 */
	struct mcbsp_util_stack removedGlobals;

	/**
	 * Keeps track which globals will be removed before
	 * the next superstep arrives.
	 */
	struct mcbsp_util_stack localsToRemove;

	/** The push request queue. */
	struct mcbsp_util_stack localsToPush;

	/**
	 * Caches the mapSearch results for use with
	 * bsp_push_reg requests.
	 */
	struct mcbsp_util_stack globalsToPush;

	/** Stores the machine part allocated to this thread. Used for nested runs. */
	struct mcbsp_util_machine_partition machine_partition;

	/** Stores statistics on the current superstep. */
	struct mcbsp_superstep_stats superstep_stats;

#ifdef __MACH__
	/** Mach OS X port for getting timings. */
	clock_serv_t clock;

	/** 
	 * Stores the start-time of this thread.
	 * (Mach-specific OS X timespec.)
	 */
	mach_timespec_t start;
#elif defined _WIN32
	/** 
	 * Stores the start-time of this thread.
	 * (Windows-specific timespec.)
	 */
	LARGE_INTEGER start;

	/**
	 * Frequency of the Windows high-
	 * resultion timer.
	 */
	LARGE_INTEGER frequency;
#else
	/**
	 * Stores the start-time of this thread.
	 * (Default POSIX timer.)
	 */
	struct timespec start;
#endif
};

/**
 *  A DRMA communication request for `get'-requests.
 *  @see bsp_get
 */
struct mcbsp_get_request {

	/** Source */
	void * source;

	/** Destination */
	void * destination;

	/** Length */
	size_t length;

};

/**
 *  A generic BSP communication message.
 *  Handles both `put' and regular BSMP requests.
 *  @see bsp_put
 *  @see bsp_send
 *  @see mcbsp_get_request (used for `get'-requests)
 *  @see mcbsp_hpsend_request (used for high-performance BSMP) 
 */
struct mcbsp_message {
	
	/** Destination */
	void * destination;

	/** Length */
	size_t length;

};

/**
 *  A DRMA communication request for hpput
 *  and hpget requests.
 */
struct mcbsp_hp_request {

	/** Source */
	const void * source;

	/** Destination */
	void * destination;

	/** Length */
	size_t length;

	/**
	 * Whether source is the remote PID.
	 *
	 * (Note that if this is true, then this request must have originated
	 *  from a bsp_hpget.)
	 */
	bool source_is_remote;
};

/**
 *  A high-performance (non-buffering) BSMP message request.
 *  @see bsp_hpsend.
 */
struct mcbsp_hpsend_request {

	/** Payload source */
	const void * payload;

	/** Tag source */
	const void * tag;

	/** Payload length */
	size_t payload_size;

};

/** Struct corresponding to a single push request. */
struct mcbsp_push_request {

	/** The local address to push. */
	void * address;

	/** The memory range to register. */
	size_t size;

};

/** Per-initialising thread initialisation data. */
extern pthread_key_t mcbsp_internal_init_data;

/** Per-thread data. */
extern pthread_key_t mcbsp_internal_thread_data;

/** Per-thread machine info. */
extern pthread_key_t mcbsp_local_machine_info;

/**
 * Per-thread machine info.
 * Necessary for nested MulticoreBSP SPMD calls.
 */
extern pthread_key_t mcbsp_internal_init_data;

/** Whether mcbsp_internal_init_data is initialised. */
extern bool mcbsp_internal_keys_allocated;

/** 
 * Contorls thread-safe singleton access to
 * mcbsp_internal_init_data.
 * Required for safe initialisation.
 */
extern pthread_mutex_t mcbsp_internal_keys_mutex;

//include the currently active hooks for accelerator support
#include "bsp-active-hooks.h"

/**
 * Performs a BSP intialisation using the init struct supplied.
 * The construction of this struct differs when called from C code,
 * or when called from the C++ wrapper.
 */
void bsp_init_internal( struct mcbsp_init_data * const initialisationData );

/**
 * Gets the machine info currently active for this
 * MulticoreBSP session.
 *
 * @return A pointer to the current machine info struct.
 */
struct mcbsp_util_machine_info *mcbsp_internal_getMachineInfo( void );

/**
 * Singleton thread-safe allocator for mcbsp_internal_init_data.
 */
void mcbsp_internal_check_keys_allocated( void );

/**
 * Entry-point of MulticoreBSP threads. Initialises internals
 * and then executes the user-defined SPMD program.
 *
 * @param p The passed POSIX threads init data.
 */
void* mcbsp_internal_spmd( void *p );

/**
 * Checks if a abort has been requested, and if so,
 * exits the current thread.
 */
void mcbsp_internal_check_aborted( void );

/**
 * Alias for mcbsp_internal_syncWithCondition using
 * the standard init.condition
 *
 * @param init   Pointer to the BSP init corresponding
 *               to our current SPMD group.
 * @param cond   The array to spinlock on.
 * @param bsp_id The unique ID number corresponding to
 *               the thread that calls this sync function.
 */
void mcbsp_internal_spinlock( struct mcbsp_init_data * const init, unsigned char * const cond, const size_t bsp_id );

/**
 * Alias for mcbsp_internal_syncWithCondition using
 * the standard init.condition
 *
 * @param init Pointer to the BSP init corresponding
 *             to our current SPMD group.
 * @param cond The condition to use for synchronisation.
 */
void mcbsp_internal_sync( struct mcbsp_init_data * const init, pthread_cond_t *cond );

/**
 * Common part executed by all BSP primitives when in SPMD part.
 * This version assumes local thread data used by BSP remains
 * unchanged.
 */ 
const struct mcbsp_thread_data * mcbsp_internal_const_prefunction( void );

/**
 * Common part executed by all BSP primitives when in SPMD part.
 * This is the non-const version of mcbsp_internal_const_prefunction.
 */
struct mcbsp_thread_data * mcbsp_internal_prefunction( void );

/**
 * Allcates and initialises the `plain old data' parts of the
 * mcbsp_thread_data struct, using the supplied parameters.
 * @param init Pointer to the initialising process' init data.
 * @param s    Process ID of the thread corresponding to this data.
 * @return The newly allocated thread data.
 */
struct mcbsp_thread_data * mcbsp_internal_allocate_thread_data( struct mcbsp_init_data * const init, const size_t s );

/**
 * Finishes initialisation of a mcbsp_thread_data struct. The
 * plain-old-data fields of a given thread_data are copied,
 * and the remaining data fields are properly initialised.
 * The old thread_data struct will be freed.
 *
 * @param data The initial data to copy.
 * @return New process-local thread data.
 */
struct mcbsp_thread_data * mcbsp_internal_initialise_thread_data( struct mcbsp_thread_data * const data );

/**
 * Deallocates a fully initialised mcbsp_thread_data struct.
 *
 * @param data The thread data to destroy.
 */
void mcbsp_internal_destroy_thread_data( struct mcbsp_thread_data * const data );

/**
 * Does the checkpointing from within a critical section.
 * THIS MEANS THIS FUNCTION IS NOT THREADSAFE. Restoring from
 * a checkpoint necessarily means the restored program exits
 * from this function.
 *
 * @return -1 on error, 0 on success, 1 on exit from recovery.
 */
int mcbsp_internal_call_checkpoint( void );

/**
 * On encountering a new superstep, this function takes care
 * of bookkeeping regarding checkpointing frequencies; including
 * checks for the existance of the failure indicating file,
 * `/etc/bsp_failure'.
 *
 * This function is NOT thread-safe and should be called by PID
 * 0 only.
 *
 * @param init The initialisation struct where to update check-
 *             point related information in.
 */
void mcbsp_internal_update_checkpointStats( struct mcbsp_init_data * const init );

/**
 * Prints any collected profiling statistics to stdout.
 *
 * @param init The profiled SPMD init structure.
 */
void mcbsp_internal_print_profile( const struct mcbsp_init_data * const init );

/**
 * (Re-)initialises the superstep stats structure. Will set the descriptor name
 * of the current superstep to `Superstep i', with i-th the current superstep
 * number.
 *
 * @param stats The pre-allocated superstep stats structure to (re-)initialise
 *              the fields of.
 * @param superstep The current superstep number.
 */
void mcbsp_internal_init_superstep_stats( struct mcbsp_superstep_stats * const stats, const size_t superstep );

/**
 * Gets the time relative to the SPMD start section. Works on Linux (via POSIX
 * realtime extensions), Windows, and Mach (OS X).
 *
 * @param data Thread-local data structure that contains the relative start
 *             time, plus any other data required for getting the current time.
 * @return The elapsed time.
 */
double mcbsp_internal_time( struct mcbsp_thread_data * const data );

#endif

