/*
 * Copyright (c) 2015
 *
 * Created 2nd of April, 2015 by Albert-Jan N. Yzelman
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

#ifndef _H_MCBSP_INTERNAL_MACROS
#define _H_MCBSP_INTERNAL_MACROS

#ifdef __GNUC__
 #ifndef _GNU_SOURCE
  #define _GNU_SOURCE
 #endif
#endif

#ifndef MCBSP_USE_MUTEXES
 /**
  * Uses spin-locks instead of standard PThreads mutexes
  * and conditions. Less energy-efficient, but faster.
  */
 #define MCBSP_USE_SPINLOCK
#endif

#ifdef MCBSP_USE_SYSTEM_MEMCPY
 #ifdef MCBSP_USE_CUSTOM_MEMCPY
  #pragma message "Warning: MCBSP_USE_SYSTEM_MEMCPY takes precedence over MCBSP_USE_CUSTOM_MEMCPY"
 #endif
#endif

#ifndef MCBSP_USE_SYSTEM_MEMCPY
/**
 * Enables a custom implementation of memcpy that can
 * lead to faster codes than standard implementations
 * because the custom implementation can be compiled to
 * run on your target architecture specifically, and may
 * benefit from, for example, architecture-specific
 * vectorisation capabilities that can speed up memory
 * copying.
 * If disabled, MulticoreBSP for C will rely on the
 * system-wide memcpy function instead.
 */
 #define MCBSP_USE_CUSTOM_MEMCPY
#endif

/**
 * Allows multiple registrations of the same memory address.
 * 
 * The asymptotic running times of bsp_push_reg, bsp_pop_reg,
 * all communication primitives, and the synchronisation
 * methods, are unaffected by this flag.
 * However, it does introduce run-time overhead as well as
 * memory overhead. If your application(s) do not use
 * multiple registrations of the same variable, you may
 * consider compiling MulticoreBSP for C with this flag
 * turned off for slightly enhanced performance.
 *
 * Note the original BSPlib standard does demand multiple
 * registrations be possible. Thus if 
 * MCBSP_COMPATIBILITY_MODE is set, this flag is
 * automatically set as well.
 */
#define MCBSP_ALLOW_MULTIPLE_REGS

#ifdef MCBSP_NO_CHECKS
/**
 * Define MCBSP_NO_CHECKS to disable all run-time sanity
 * checks, except those regarding memory allocation.
 */
 #define MCBSP_NO_CHECKS
#endif

/**
 * When MulticoreBSP for C allocates memory, it aligns it to the following
 * boundary (in bytes).
 */
#define MCBSP_ALIGNMENT 128

/**
 * If enabled, then during the handling of communication buffers during a
 * bsp_sync, a use congestion-avoiding scheduling is used.
 */
#define MCBSP_CA_SYNC

#endif

