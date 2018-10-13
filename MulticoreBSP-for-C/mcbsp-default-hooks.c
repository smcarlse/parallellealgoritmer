/*
 * Copyright (c) 2015-2017
 *
 * File created Albert-Jan N. Yzelman, 2015.
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


#include "mcbsp-default-hooks.h"

extern void mcbsp_accelerator_implied_init(
	struct mcbsp_init_data * const init,
	const bsp_pid_t P
);

extern void mcbsp_accelerator_full_init(
	struct mcbsp_init_data * const init, 
	const int argc,
	char * const * const argv
);

extern bsp_pid_t mcbsp_accelerator_offline_nprocs( void );

extern size_t mcbsp_nocc_cache_line_size( void );

extern void mcbsp_nocc_purge_all( void );

extern void mcbsp_nocc_flush_cacheline( const void * const address );

extern void mcbsp_nocc_invalidate(
	const void * const address,
	const size_t length
);

extern void mcbsp_nocc_wait_for_flush( void );

extern void mcbsp_nocc_invalidate_cacheline( const void * const address );

