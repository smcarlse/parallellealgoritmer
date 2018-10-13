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


#ifndef _H_BSP_HOOKS
#define _H_BSP_HOOKS

#undef MCBSP_WITH_ACCELERATOR

#include "mcbsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Accelerator-specific initialisation. This version is to be called by
 * #bsp_begin.
 * 
 * This function should be called on initialisation of the MulticoreBSP run-
 * time and thus is only relevant when running BSP code on the accelerator.
 *
 * The default behaviour is to do nothing. If accelerators need to be
 * initialised for MulticoreBSP to work, then a specific implementation of
 * this function for that accelerator must be provided.
 *
 * @param init Where to store initialisation data.
 * @param P    How many BSP processes to spawn.
 */
static inline void mcbsp_accelerator_implied_init(
	struct mcbsp_init_data * const init,
	const bsp_pid_t P
) {
	(void)init;
	(void)P;
}

/**
 * Accelerator-specific initialisation. This version is compatible with
 * #bsp_init.
 * 
 * This function should be called on initialisation of the MulticoreBSP run-
 * time and thus is only relevant when running BSP code on the accelerator.
 *
 * The default behaviour is to do nothing. If accelerators need to be
 * initialised for MulticoreBSP to work, then a specific implementation of
 * this function for that accelerator must be provided.
 *
 * @param init Where to store initialisation data.
 * @param argc The number of input arguments passed to the executable.
 * @param argv The input arguments passed to the executable.
 */
static inline void mcbsp_accelerator_full_init(
	struct mcbsp_init_data * const init, 
	const int argc,
	char * const * const argv
) {
	(void)init;
	(void)argc;
	(void)argv;
}

/**
 * Returns the maximum number of processes supported by the accelerator.
 *
 * The default behaviour is to return the maximum number of processes.
 * This is likely invalid behaviour. Users adding support for a specific
 * accelerator should provide a more sensible implementation of this
 * function.
 *
 * @return The maximum number of processes to spawn on the accelerator.
 */
static inline bsp_pid_t mcbsp_accelerator_offline_nprocs( void ) {
#ifdef __cplusplus
	return static_cast< bsp_pid_t >( -1 );
#else
	return ((bsp_pid_t)-1);
#endif
}

/**
 * Returns the cache line size in bytes, as active on the accelerator.
 *
 * The default behaviour is to return zero. This is likely invalid.
 * Users adding support for a specific accelerator should provide a more
 * sensible implementation of this function.
 *
 * @return The cache line size (in bytes).
 */
static inline size_t mcbsp_nocc_cache_line_size( void ) { 
	return 0;
}

/**
 * Purges the local cache.
 *
 * A flushes all dirty cache lines in local cache into main memory.
 * It will not necessarily update copies of any corresponding cache
 * lines in remote memory. Additionally, it will invalidate all
 * cache lines in local cache. On completion, the cache will be as
 * though it were empty.
 *
 * The default behaviour is to do nothing. If accelerators are not
 * cache-coherent, then a specific implementation of this function
 * for that accelerator must be provided.
 */
static inline void mcbsp_nocc_purge_all( void ) {}

/**
 * Flushes a specific cache line. The cache line is indicated by
 * address. The address is rounded down to the nearest multiple of
 * the cache line size, and the memory chunk of size L starting at
 * that address, is flushed (where L is the cache line size as
 * returned by #mcbsp_nocc_cache_line_size).
 *
 * Flushing a cache line will check if it is dirty. If so, it will
 * write that data to main memory. This function may exit before
 * any write back has completed. To wait for the write-back to
 * complete, use #mcbsp_nocc_wait_for_flush.
 * 
 * The default behaviour is to do nothing. If accelerators are not
 * cache-coherent, then a specific implementation of this function
 * for that accelerator must be provided.
 */
static inline void mcbsp_nocc_flush_cacheline( const void * const address ) {
	(void)address;
}

/**
 * Invalidate a specific cache line. The first cache line is
 * indicated by \a address. The address is rounded down to the
 * nearest multiple of the cache line size, and the memory chunk
 * of size L starting at that address, is flushed (where L is the
 * cache line size as returned by #mcbsp_nocc_cache_line_size).
 * The last cache line to be invalidated is similarly determined
 * by processing the address \a address + \a length.
 *
 * Invalidation makes it so that any next referral to any memory
 * address corresponding to any of the cache lines that were
 * invalidated, will retrieve up-to-date information from main
 * memory, regardless of whether any lines were in local cache.
 * This function does not guarantee dirty cache lines would be
 * written back.
 * To write back dirty memory to RAM, use flush. To both flush
 * and invalidate, use purge.
 *
 * The default behaviour is to do nothing. If accelerators are not
 * cache-coherent, then a specific implementation of this function
 * for that accelerator must be provided.
 */
static inline void mcbsp_nocc_invalidate(
	const void * const address,
	const size_t length
) {
	(void)address;
	(void)length;
}

/**
 * Waits for any outstanding flushes to complete. This is a blocking
 * function. It is used to make sure any outstanding calls to
 *  - #mcbsp_nocc_invalidate
 *  - #mcbsp_nocc_invalidate_cacheline
 *  - #mcbsp_nocc_purge_all
 * have completed.
 *
 * The default behaviour is to do nothing. If accelerators are not
 * cache-coherent, then a specific implementation of this function
 * for that accelerator must be provided.
 */
static inline void mcbsp_nocc_wait_for_flush( void ) {}

/**
 * As #mcbsp_nocc_invalidate, but invalidates a single cache line
 * only.
 *
 * @param address The cache line to be invalidated contains this
 *                address.
 */
static inline void mcbsp_nocc_invalidate_cacheline( const void * const address ) {
	(void)address;
}

#ifdef __cplusplus
}
#endif

#endif

