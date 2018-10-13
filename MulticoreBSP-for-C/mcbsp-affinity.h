/*
 * Copyright (c) 2013
 *
 * File written by A. N. Yzelman, 2013.
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

/*! \file mcbsp-affinity.h
 *
 * The MulticoreBSP for C affinity interface
 * 
 * The functions and data types defined in this file
 * allow a user to take control of the way MulticoreBSP
 * pins BSP processes to hardware threads, at run-time.
 *
 * Changes made to the machine layout using these
 * functions take effect only at the next call to
 * bsp_begin; calling these functions does not 
 * affect running SPMD sections.
 *
 * Values set using this interface supercede values
 * defined in
 *    1. the `machine.info' file (if it exists);
 *    2. the default values.
 *
 * The default values can be changed by directly
 * setting the MCBSP_DEFAULT_AFFINITY,
 * MCBSP_DEFAULT_THREADS_PER_CORE, or
 * MCBSP_DEFAULT_THREAD_NUMBERING globals.
 *
 * All fields are thread-local. Setting values
 * manually should occur on each thread separately.
 * This is only useful if multiple threads call
 * bsp_begin *simultaneously*. This is useful when
 * employing hierarchical BSP computations.
 */


#ifndef _H_MCBSP_AFFINITY
#define _H_MCBSP_AFFINITY

/**
 * \defgroup affinity MulticoreBSP for C extensions for affinity control
 *
 * MulticoreBSP for C extensions for thread affinity, BSP computer, and BSP
 * contracts descriptions.
 *
 * This module enables user-level control of the affinity strategy that
 * MulticoreBSP for C employs. These settings directly influence the behaviour
 * of the BSPlib implementation regarding attaining highly performance,
 * predictability, and fault tolerance.
 *
 * Settings can be influenced in various ways, as detailed in the below.
 *
 * Code-based:
 * ===========
 *   the following six functions influence the pinning strategy of
 *   MulticoreBSP.
 *
 * 	-mcbsp_set_maximum_threads
 * 	-mcbsp_set_available_cores
 * 	-mcbsp_set_threads_per_core
 *	-mcbsp_set_thread_numbering
 *	-mcbsp_set_affinity_mode
 *      -mcbsp_set_pinning
 * 	-mcbsp_set_reserved_cores
 *
 *   See the documentation or the below file-based description for details on
 *   the different settings. To use these functions, the user should include
 *   the `mcbsp-affinity.h' header.
 * 
 *   Options set by using any of these functions can only be overruled by 
 *   later calls to the same functions. Default values are found below.
 * 
 * File-based:
 * ===========
 *   The architecture information is available in a file called `machine.info',
 *   available in the current work directory.
 *
 *   The advantage is that MulticoreBSP users need not know the hardware 
 *   specifics; a system administrator can simply provide a single text file
 *   to the users of his machine, if the MulticoreBSP defaults do not suffice.
 * 
 *   Example machine.info (excluding bracket lines):
 * 	{
 * 		threads 4
 * 		cores 61
 * 		threads_per_core 4
 * 		thread_numbering consecutive
 * 		affinity manual
 * 		pinning 4 8 12 16
 * 		reserved_cores 0
 * 	}
 * 
 *   -threads should be a positive integer, and equals the maximum number of
 *    threads MulticoreBSP can spawn.
 *    
 *   -cores should be a positive integer, and equals the number of cores
 *    available on the machine.
 *
 *   -threads_per_core should be a positive integer. Indicates how many
 *    hardware threads map to the same (hardware) CPU core. Normally this is 1;
 *    but machines with hyper-threading enabled have 2 threads_per_core, while
 *    the Xeon Phi, for example, has 4 hardware threads per core.
 *
 *                                 ***WARNING***
 *             taking threads > cores*threads_per_core is possible,
 *                         but may require manual pinning
 *
 *   -thread_numbering is either `consecutive' or `wrapped', and only has effect
 *    if the number of threads_per_core is larger than 1.
 *    
 *    Let i, k be integers with 0 <= i < cores, and 0 <= k < threads_per_core.
 *
 *    With `wrapped', thread number (i + k * cores) maps to core i. With
 *    `consecutive', thread number (k + i * threads_per_core) maps to core i.
 *
 *    Whether your machine uses wrapped or consecutive numbering depends on the
 *    operating system and on the hardware; e.g., the Linux kernel on hyper-
 *    treading architectures uses a wrapped thread numbering.
 *    On the other hand, the Intel Xeon Phi OS, for example, numbers its threads
 *    consecutively.
 *
 *   -affinity is either scatter, compact, or manual. Scatter will spread
 *    MulticoreBSP threads as much as possible over all available cores, thus
 *    maximising bandwidth use on NUMA systems.
 *    Compact will pin all MulticoreBSP threads as close to each other as
 *    possible. Both schemes rely on the thread numbering being set properly.
 *    Manual will let the i-th BSP thread be pinned to the hardware thread with
 *    OS number pinning[i], where `pinning' is a user-supplied array with length
 *    equal to the number of threads.
 *
 *   -pinning is a list of positive integers of length threads. Pinning is
 *    mandatory when affinity is manual, otherwise it will be ignored.
 *
 *   -reserved_cores An array with elements i in the range 0 <= i < cores. These
 *    cores will not be used by BSP threads. Useful in situations where part of
 *    the machine is reserved for dedicated use, such as for OS-use or for
 *    communication handling.
 *
 * Code-based defaults:
 * ====================
 *   the following fields (as declared in mcbsp-affinity.h) define default
 *   settings that can be changed at run-time:
 *   
 *   -MCBSP_DEFAULT_AFFINITY
 *   -MCBSP_DEFAULT_THREADS_PER_CORE
 *   -MCBSP_DEFAULT_THREAD_NUMBERING
 *
 *   Other defaults are fixed to the below values.
 *
 *                                ***WARNING***
 *   as per the ordering in priority, the `machine.info' file, if it exists,
 *   will always override these defaults, even if these were changed at run-
 *   time. Use the explicit code-based setters when the file-based defaults
 *   are to be overridden.
 *
 * Defaults:
 * =========
 *   If no `machine.info' file is found, or if this file leaves some options
 *   undefined, and if the corresponding option was not set manually, then
 *   the following default values will be used. Some of these default values
 *   may be adapted (see above).
 *
 *   	-threads: <the number of OS hardware threads>
 *   	-cores: <the number of OS hardware threads>
 *   	-threads_per_core: 1
 *   	-thread_numbering: consecutive
 *   	-affinity: scatter
 *   	-reserved_cores: <empty array>
 *
 *    ***THESE DEFAULTS ARE NOT SUITABLE FOR HYPER-THREADING MACHINES***
 *  For machines with hyperthreads enabled, the following two lines should be
 *  made known to MulticoreBSP:
 *  	-threads_per_core 2
 *  	-thread_numbering wrapped
 */

#include <stdlib.h>

/**
 * Pre-defined strategies for pinning threads.
 *
 * @ingroup affinity
 */
enum mcbsp_affinity_mode {

	/**
	 * A scattered affinity will pin P consecutive threads
	 * so that the full range of cores is utilised. This
	 * assumes the cores are numbered consecutively by the
	 * OS. If this is not the case, please use MANUAL.
	 * This is the default strategy.
	 */
	SCATTER = 0,

	/**
	 * A compact affinity will pin P consecutive threads 
	 * to the first P available cores.
	 */
	COMPACT,

	/**
	 * A manual affinity performs pinning as per user-
	 * supplied definitions.
	 * See also MCBSP_DEFAULT_MANUAL_AFFINITY
	 */
	MANUAL
};

/**
 * Enumerates ways of hardware thread numbering.
 *
 * @ingroup affinity
 */
enum mcbsp_thread_numbering {

	/**
	 * If each core supports s threads, and t cores are available
	 * (for a total of p=s*t hardware threads), then CONSECUTIVE
	 * assigns the numbers i*t, i*t+1, ..., i*t+s-1 to hardware
	 * threads living on core i (with 0<=i<t).
	 */
	CONSECUTIVE = 0,

	/**
	 * If each core supports s threads, and t cores are available
	 * (for a total of p=s*t hardware threads), then WRAPPED
	 * assigns the numbers i, i+s, i+2*s, ..., i+(t-1)*s to
	 * hardware threads living on core i (with 0<=i<t).
	 */
	WRAPPED
};

/**
 * Changes the maxmimum amount of threads MulticoreBSP can allocate. Setting
 * this higher than the (auto-detected) maximum of your machine will cause your
 * MulticoreBSP applications to hang, unless combined with a manually set
 * affinity strategy preventing this.
 *
 * @param max The new maximum number of threads supported by this machine.
 *
 * @ingroup affinity
 */
void mcbsp_set_maximum_threads( const size_t max );

/**
 * Changes the currently active affinity strategy.
 *
 * Users should use this function to set the affinity strategy to something other
 * than the default strategy (SCATTER).
 *
 * This will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * Will override defaults, and will override values given in `machine.info'.
 *
 * @param mode The new preferred affinity strategy to be used by MulticoreBSP.
 *
 * @ingroup affinity
 */
void mcbsp_set_affinity_mode( const enum mcbsp_affinity_mode mode );

/**
 * Changes the number of available cores. 
 *
 * Will override defaults, and will override values given in `machine.info'.
 *
 * By default, MulticoreBSP will set this value equal to the number of hardware
 * threads detected, divided by threads_per_core.
 *
 * This will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * @param num_cores The new number of cores supported by this machine. Note
 *                  that this need not be equal to the number of threads,
 *                  given that a core can support multiple hardware threads.
 *
 * @ingroup affinity
 */
void mcbsp_set_available_cores( const size_t num_cores );

/**
 * Changes the number of threads per core. Will override defaults, and will
 * override values given in `machine.info'.
 *
 * The default value is 1. The value affects only the SCATTER and COMPACT
 * strategies.
 *
 * This will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * @param threads_per_core The number of hardware threads that is supported by
 *                         each core on this machine.
 *
 * @ingroup affinity
 */
void mcbsp_set_threads_per_core( const size_t threads_per_core );

/**
 * Prevents the use of a given number of threads at the same core.
 *
 * @param unused_threads_per_core The number of threads per core that
 *                                MulticoreBSP for C is forbidden to run
 *                                BSP processes on.
 *
 * @ingroup affinity
 */
void mcbsp_set_unused_threads_per_core( const size_t unused_threads_per_core );

/**
 * Changes the thread numbering strategy this machines adheres to.
 *
 * Will override defaults, and will override values given in `machine.info'.
 * The default is CONSECUTIVE. This value affects only the SCATTER and COMPACT
 * strategies, and will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * @param numbering The strategy this machine uses for thread numbering.
 *
 * @ingroup affinity
 */
void mcbsp_set_thread_numbering( const enum mcbsp_thread_numbering numbering );

/**
 * Supplies a manually defined pinning strategy for MulticoreBSP to use.
 *
 * Implies
 *     `mcbsp_set_affinity_mode( MANUAL );'
 *
 * Will override values given in `machine.info'.
 *
 * The supplied pinning array must be of size equal to the maximum number of
 * threads supported by the current machine, and is buffered (copied)
 * internally by MulticoreBSP for C.
 *
 * This will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * Note: the user-supplied array must also be freed by the user.
 *
 * @param pinning The new BSP ID to hardware thread ID pinning. The array
 *                remains under control of the user and may be freed or
 *                repurposed after calling this function.
 *
 * @param length The length of the array `pinning'.
 *
 * @ingroup affinity
 */
void mcbsp_set_pinning( const size_t * const pinning, const size_t length );

/**
 * Supplies a list of core IDs that are NOT to be used by MulticoreBSP SPMD
 * runs.
 *
 * The user should NOT update the number of threads of the machine;
 * MulticoreBSP will automatically infer the number of threads available
 * by subtracting from the total number of supported threads the effective
 * number of reserved threads.
 *
 * Please do make sure to set the number of threads_per_core and
 * thread_numbering correctly in case the defaults do not apply.
 *
 * This will only affect new SPMD instances.
 *
 * This function is thread-safe in that upon exit, valid machine info is
 * guaranteed. Concurrent calls with bsp_begin still constitutes a programming
 * error, however (but will not result in crashes).
 *
 * Note: the user-supplied array must also be freed by the user.
 *
 * @param reserved An array containing which CPU core IDs MulticoreBSP is
 *                 forbidden to use.
 * @param length The length of the `reserved' array.
 *
 * @ingroup affinity
 */
void mcbsp_set_reserved_cores( const size_t * const reserved, const size_t length );

/** 
 * @return The maximum number of hardware-supported threads.
 *
 * @see mcbsp_set_maximum_threads
 *
 * @ingroup affinity
 */
size_t mcbsp_get_maximum_threads( void );

/**
 * @return The currently active thread affinity strategy.
 *
 * @see mcbsp_set_affinity_mode
 *
 * @ingroup affinity
 */
enum mcbsp_affinity_mode mcbsp_get_affinity_mode( void );

/**
 * @return The number of hardware cores on this system.
 *
 * @see mcbsp_set_available_cores
 *
 * @ingroup affinity
 */
size_t mcbsp_get_available_cores( void );

/**
 * @return The number of hardware threads supported by each core on this
 *         machine.
 *
 * @see mcbsp_set_threads_per_core
 *
 * @ingroup affinity
 */
size_t mcbsp_get_threads_per_core( void );

/**
 * @return The number of unused threads per core.
 *
 * @see mcbsp_set_unused_threads_per_core
 *
 * @ingroup affinity
 */
size_t mcbsp_get_unused_threads_per_core( void );

/**
 * @return The thread-numbering scheme employed on this machine.
 *
 * @see mcbsp_set_thread_numbering
 *
 * @ingroup affinity
 */
enum mcbsp_thread_numbering mcbsp_get_thread_numbering( void );

/**
 * @return NULL if there is no manually-set pinning strategy, or a copy of the
 *         pinning array otherwise. The array length equals the maximum number
 *         of supported threads.
 *
 * @see mcbsp_set_pinning
 * @see mcbsp_get_maximum_threads
 *
 * @ingroup affinity
 */
size_t * mcbsp_get_pinning( void );

/**
 * @return The number of reserved cores.
 *
 * @see mcbsp_set_reserved_cores
 *
 * @ingroup affinity
 */
size_t mcbsp_get_reserved_cores_number( void );

/**
 * @return A copy of the array of reserved cores, or NULL if no cores are
 *         reserved.
 *
 * @see mcbsp_set_reserved_cores
 * @see mcbsp_get_reserved_cores_number
 *
 * @ingroup affinity
 */
size_t * mcbsp_get_reserved_cores( void );

//Default values

/**
 * Default affinity strategy (SCATTER).
 *
 * @ingroup affinity
 */
extern enum mcbsp_affinity_mode MCBSP_DEFAULT_AFFINITY;

/**
 * Default number of threads per core (1).
 *
 * @ingroup affinity
 */
extern size_t MCBSP_DEFAULT_THREADS_PER_CORE;

/**
 * Default thread numbering (CONSECUTIVE).
 *
 * @ingroup affinity
 */
extern enum mcbsp_thread_numbering MCBSP_DEFAULT_THREAD_NUMBERING;

#endif

