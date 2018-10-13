/*
 * Copyright (c) 2007-2017
 *
 * File created Albert-Jan N. Yzelman, 2007-2009.
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

/*! \mainpage MulticoreBSP for C
 *
 * This is the doxygen code of the MulticoreBSP for C library.
 * The latest version of the library, and of this documentation,
 * can always be found at:
 *
 *     http://www.multicorebsp.com
 *
 * For users of the library: the documentation of bsp.h is all
 * you require when coding in C. A C++-wrapper is also available;
 * see bsp.hpp.
 * For advanced run-time thread affinity control, see
 * mcbsp-affinity.h.
 *
 * The BSPlib extension header files mcbsp-resiliency.h,
 * mcbsp-profiling.h, and mcbsp-templates.hpp furthermore make
 * available functions for controlling BSP checkpointing (if
 * enabled during compilation), for controlling BSP profiling,
 * and for automatic element-to-byte translations for BSP
 * primtives in C++ programming, respectively.
 *
 * The remainder of the documentation is meant to help those
 * who want to understand or adapt the MulticoreBSP library.
 *
 * MulticoreBSP, which includes<ol>
 *     <li>MulticoreBSP for C,</li>
 *     <li>MulticoreBSP for Java,</li>
 *     <li>Java MulticoreBSP utilities, and</li>
 *     <li>BSPedupack for Java</li>
 * </ol>
 *
 * are copyright by
 *  1. Albert-Jan Nicholas Yzelman,
 *     Student of Scientific Computing, Utrecht University, 2007.
 *  2. Dept. of Mathematics, Utrecht University, 2007-2011,
 *     with author A. N. Yzelman, PhD candidate.
 *  3. Dept. of Computer Science, KU Leuven, 2011-2015,
 *     with author A. N. Yzelman, Postdoctoral researcher,
 *     affiliated to the Flanders ExaScience Lab (Intel Labs Europe),
 *     and the ExaLife Lab, Leuven, Belgium.
 *  4. Huawei Technologies France, 2015,
 *     with authors A. N. Yzelman and W. Suijlen.
 *
 * <p>
 * MulticoreBSP for C is distributed as part of the original
 * MulticoreBSP and is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser 
 * General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or 
 * (at your option) any later version.<br><br>
 * MulticoreBSP is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the 
 * implied warranty of MERCHANTABILITY or FITNESS FOR A 
 * PARTICULAR PURPOSE. See the GNU Lesser General Public 
 * License for more details.
 * </p><p>
 * You should have received a copy of the GNU Lesser General 
 * Public License along with MulticoreBSP. If not, see
 * <http://www.gnu.org/licenses/>.</p>
 */

/*! \file bsp.h 
 * Defines the MulticoreBSP primitives and includes the necessary standard
 * headers. The linkage is defined by the various macros that can be set
 * before including this header:
 * <ul>
 *   <li>MCBSP_ENABLE_HP_DIRECTIVES      (enables hpsend, hpget, and hpput)</li>
 *   <li>MCBSP_ENABLE_FAKE_HP_DIRECTIVES (aliases hpsend to send, hpget to get, and hpput to put)</li>
 *   <li>MCBSP_COMPATIBILITY_MODE        (reverts to the original BSPlib specification by Hill et al.)</li>
 *   <li>MCBSP_ALLOW_MULTIPLE_REGS       (allows multiple push_regs of the same registration key)</li>
 * </ul>
 *
 * Enabled by default are MCBSP_ENABLE_HP_DIRECTIVES and MCBSP_ALLOW_MULTIPLE_REGS. Note that:
 *     * MCBSP_ENABLE_HP_DIRECTIVES and MCBSP_ENABLE_FAKE_HP_DIRECTIVES are mutually exclusive.
 *     * MCBSP_COMPATIBILITY_MODE implies MCBSP_ALLOW_MULTIPLE_REGS.
 */

/**
 * \defgroup SPMD Single Program, Multiple Data Framework
 *
 * Collection of BSPlib primitives that control BSP processes.
 *
 * Each BSP process executes the same program-- and to do useful computation,
 * this program should be run on different data; hence the BSP paradigm falls
 * under the Single Program, Multiple Data (SPMD) family. Additionally, BSP
 * imposes a strict structure on a BSP program's algorithmic structure; a
 * program consists out of a series of <em>supersteps</em>, and each superstep
 * consists of (1) a computation phase, and (2) a communication phase.
 * From an algorithmic viewpoint, communication is not allowed during a
 * computation phase, while computation is not be allowed during a communication
 * phase. In practice, however, an implementation may perform asynchronous
 * communication and may not even require global barriers.
 *
 * To start SPMD sections, BSPlib defines the following primitives:
 * <ul>
 *  <li>bsp_init   Initialises the BSP runtime and registers the SPMD section.</li>
 *  <li>bsp_begin  Starts an SPMD program.</li>
 * </ul>
 *
 * To end SPMD sections, the following primitives are available:
 * <ul>
 *  <li>bsp_end    Cleanly exits an SPMD run.</li>
 *  <li>bsp_abort  Aborts an SPMD run, as soon as possible.</li>
 * </ul>
 *
 * To provide BSP processes introspection about the SPMD run they take part in:
 * <ul>
 *  <li>bsp_pid    Returns a process ID that is unique in the current SPMD run.</li>
 *  <li>bsp_nprocs Returns the number of processes in the current SPMD run.</li>
 * </ul>
 *
 * To signal the end of the current computation phase and start the following
 * communication phase:
 * <ul>
 *  <li>bsp_sync</li>
 * </ul>
 *
 * In BSPlib, an SPMD program is given by a function wherein the first
 * executable statement is bsp_begin, and the last executable statement is
 * bsp_end. If the SPMD function is not `int main()', the SPMD section must
 * first be registered using bsp_init.
 * The functions bsp_end, bsp_abort, and bsp_pid will raise errors when called
 * outside an SPMD section.
 */

/**
 * \defgroup DRMA Direct Remote Memory Access
 *
 * Direct Remote Memory Access (DRMA) provides BSP processes the means to
 * accesses memory regions of other BSP processes.
 *
 * Remote memory regions must first be registered before DRMA primitives are
 * allowed to operate on them:
 * <ul>
 *  <li>bsp_push_reg Registers a memory area of a given length.</li>
 *  <li>bsp_pop_reg  Invalidates a previously registered memory area.</li>
 * </ul>
 *
 * To copy local data directly to remote memory areas:
 * <ul>
 *  <li>bsp_put   Buffers local data and does not touch remote data during the
 *                current computation phase.</li>
 *  <li>bsp_hpput Unbuffered communication that immediately invalidates remote
 *                memory as well.</li>
 * </ul>
 * 
 * To copy data from remote memory areas to local memory:
 * <ul>
 *  <li>bsp_get Buffers remote data at the end of the current computation
 *              phase, and does not touch local data until then.</li>
 *  <li>bsp_hpget Unbuffered communication that immediately invalidates the
 *                local memory area.</li>
 * </ul>
 *
 * The bsp_put and bsp_get are considered safe primitives, while the
 * bsp_hpget and bsp_hpput should be used with care.
 */

/**
 * \defgroup BSMP Bulk Synchronous Message Passing
 *
 * Bulk Synchronous Message Passing (BSMP) provides message passing semantics
 * to BSPlib programs. A message consists of a tag and a payload. The tag size
 * is fixed during a superstep, while the payload size can vary per message.
 *
 * To control the tag size (in bytes):
 * <ul>
 *  <li>bsp_set_tagsize</li>
 * </ul>
 *
 * To send a message:
 * <ul>
 *  <li>bsp_send   Buffered send; first copies the tag and payload into a local
 *                 outgoing messages buffer.</li>
 *  <li>bsp_hpsend Unbuffered version that requires the source memory area to
 *                 remain untouched until after the current computation phase
 *                 (that is, untouched until the next bsp_sync).</li>
 * </ul>
 *
 * To inspect the queue of received messages:
 * <ul>
 *  <li>bsp_qsize</li>
 * </ul>
 *
 * To receive a message:
 * <ul>
 *  <li>bsp_get_tag Returns the size of the payload as well as a pointer to the
 *                  memory area where the message tag is stored. The message is
 *                  not removed from the queue, unlike with the bsp_move and the
 *                  bsp_hpmove.</li>
 *  <li>bsp_move    Buffered receive that copies the payload of the first message
 *                  in the queue into user-defined and user-managed memory areas.
 *                  The message is then removed from the incoming message queue.</li>
 *  <li>bsp_hpmove  Unbuffered receive that returns pointers into memory areas
 *                  managed by the BSPlib implementation. The user should not
 *                  attempt to free any memory areas retrieved by use of this
 *                  primitive, and should be aware that invalid memory accesses,
 *                  as always, lead to undefined behaviour. Like with bsp_move,
 *                  after returning the pointers, the next message may be
 *                  retrieved. The returned pointers remain valid until the end
 *                  of the current computation phase (i.e., until the next
 *                  bsp_sync).</li>
 * </ul>
 */

#ifndef _H_MCBSP
#define _H_MCBSP

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

//macros that control the interface follow first:

/**
 * Enables high-performance variants of bsp_put, bsp_get,
 * and bsp_send. The high-performance variant of bsp_move
 * is always enabled. If this pragma is not defined,
 * the MCBSP_ENABLE_FAKE_HP_DIRECTIVES pragma will be set
 * automatically.
 */
#define MCBSP_ENABLE_HP_DIRECTIVES

#ifndef MCBSP_ENABLE_HP_DIRECTIVES
/** 
 * Sets bsp_put and bsp_get to substitute as implementations
 * for bsp_hpput and bsp_hpget, respectively.
 * If this flag is not set, bsp_hpput and bsp_hpget will not
 * be defined. Note that according to the standard, the
 * regular bsp_put and bsp_get are indeed valid replacements
 * for bsp_hpput and bsp_hpget-- they just are not more
 * performant.
 */
#define MCBSP_ENABLE_FAKE_HP_DIRECTIVES
#endif

/** Data type used for thread IDs. */
typedef unsigned int bsp_pid_t;
/** Data type used to count the number of BSMP messages waiting in queue. */
typedef unsigned int bsp_nprocs_t;
/** Data type used to refer to memory region sizes. */
typedef size_t bsp_size_t;

//set forced defines
#ifdef MCBSP_COMPATIBILITY_MODE
 //The BSPlib standard requires multiple registrations.
 #define MCBSP_ALLOW_MULTIPLE_REGS
#endif

//The public API is actually overriden by the use of macros in the below.
//This allows for per-unit mode switching, between, for instance,
//performance, debug, or profiling mode.
//Special modes are defined using the macro MCBSP_MODE, whose value
//contains the prefix to be used. For example:
//#define MCBSP_MODE bsp_profile
//will transform all BSP calls to bsp_begin to bsp_profile_begin, and
//similarly for all other BSP primitives. A special case is given by the
//MCBSP_COMPATIBILITY_MODE, which uses ANSI C99 style inlining in
//combination with macro-based aliases.
//A list of currently supported values for MCBSP_MODE follows:
//   -1: the default performance mode.
//   -2: a debugging mode that performs more sanity checks and
//       is friendly for use with debuggers such as gdb and
//       valgrind.
//   -3: a profile mode that collects statistics during the
//       SPMD run and report them after SPMD exit. 

#ifndef MCBSP_MODE
 /**
  * Defines the run-time mode of MulticoreBSP for C.
  *
  * Not compiling in the default performance mode (MCBSP_MODE == 1) will print
  * warning messages at compile time.
  * The debug mode (2) makes MulticoreBSP perform more internal checking and
  * more user argument sanity checking. It will also make MulticoreBSP use
  * POSIX Threads barriers for synchronisation (instead of the default
  * spinlocks) so to induce better behaviour on debuggers such as Valgrind,
  * which rely on a serialised execution of SPMD programs.
  * The profile mode collects various statistics at run-time and reports them
  * when the SPMD program collectively enters the bsp_end.
  *
  * 1: performance mode (default).
  * 2: debug mode.
  * 3: profile mode.
  */
 #define MCBSP_MODE 1
#endif

#if MCBSP_MODE == 3
 #pragma message "Warning: compiling in profile mode"
 #define MCBSP_FUNCTION_PREFIX(n) mcbsp_profile_##n
#elif MCBSP_MODE == 2
 #pragma message "Warning: compiling in debug mode"
 #define MCBSP_FUNCTION_PREFIX(n) mcbsp_debug_##n
#else
 #if MCBSP_MODE != 1
  #pragma message "Warning: unknown MCBSP_MODE `" #MCBSP_MODE "'; defaulting to perf mode..."
 #endif
 #define MCBSP_FUNCTION_PREFIX(n) mcbsp_perf_##n
#endif

//the real library functions are prototyped here,
//and depend on which mode we are compiling for:
void MCBSP_FUNCTION_PREFIX(begin)( const bsp_pid_t P );
void MCBSP_FUNCTION_PREFIX(end)( void );
void MCBSP_FUNCTION_PREFIX(init)( void (*spmd)(void), int argc, char **argv );
void MCBSP_FUNCTION_PREFIX(abort)( const char * const error_message, ... );
void MCBSP_FUNCTION_PREFIX(vabort)( const char * const error_message, va_list args );
bsp_pid_t MCBSP_FUNCTION_PREFIX(nprocs)( void );
bsp_pid_t MCBSP_FUNCTION_PREFIX(pid)( void );
double MCBSP_FUNCTION_PREFIX(time)( void );
void MCBSP_FUNCTION_PREFIX(sync)( void );
void MCBSP_FUNCTION_PREFIX(push_reg)( void * const address, const bsp_size_t size );
void MCBSP_FUNCTION_PREFIX(pop_reg)( void * const address );
void MCBSP_FUNCTION_PREFIX(put)(
	const bsp_pid_t pid,
	const void * const source,
	const void * const destination,
	const bsp_size_t offset,
	const bsp_size_t size
);
void MCBSP_FUNCTION_PREFIX(get)(
	const bsp_pid_t pid,
	const void * const source,
	const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
);
void MCBSP_FUNCTION_PREFIX(direct_get)(
	const bsp_pid_t pid,
	const void * const source,
        const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
);
void MCBSP_FUNCTION_PREFIX(set_tagsize)( bsp_size_t * const size );
void MCBSP_FUNCTION_PREFIX(send)(
	const bsp_pid_t pid,
	const void * const tag,
	const void * const payload,
	const bsp_size_t size
);
void MCBSP_FUNCTION_PREFIX(qsize)(
	bsp_nprocs_t * const packets,
	bsp_size_t * const accumulated_size
);
void MCBSP_FUNCTION_PREFIX(get_tag)(
	bsp_size_t * const status,
	void * const tag
);
void MCBSP_FUNCTION_PREFIX(move)(
	void * const payload,
	const bsp_size_t max_copy_size
);
bsp_size_t MCBSP_FUNCTION_PREFIX(hpmove)(
	void* * const p_tag,
	void* * const p_payload
);
void MCBSP_FUNCTION_PREFIX(hpput)(
	const bsp_pid_t pid,
	const void * const source,
	const void * const destination,
	const bsp_size_t offset,
	const bsp_size_t size
);
void MCBSP_FUNCTION_PREFIX(hpget)(
	const bsp_pid_t pid,
	const void * const source,
	const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
);
#ifdef MCBSP_ENABLE_HP_DIRECTIVES
void MCBSP_FUNCTION_PREFIX(hpsend)(
	const bsp_pid_t pid,
	const void * const tag,
	const void * const payload,
	const bsp_size_t size
);
#endif

#if MCBSP_MODE == 2
 #define mcbsp_begin       mcbsp_debug_begin
 #define mcbsp_end         mcbsp_debug_end
 #define mcbsp_init        mcbsp_debug_init
 #define mcbsp_abort       mcbsp_debug_abort
 #define mcbsp_vabort      mcbsp_debug_vabort
 #define mcbsp_nprocs      mcbsp_debug_nprocs
 #define mcbsp_pid         mcbsp_debug_pid
 #define mcbsp_time        mcbsp_debug_time
 #define mcbsp_sync        mcbsp_debug_sync
 #define mcbsp_push_reg    mcbsp_debug_push_reg
 #define mcbsp_pop_reg     mcbsp_debug_pop_reg
 #define mcbsp_put         mcbsp_debug_put
 #define mcbsp_get         mcbsp_debug_get
 #define mcbsp_send        mcbsp_debug_send
 #define mcbsp_qsize       mcbsp_debug_qsize
 #define mcbsp_get_tag     mcbsp_debug_get_tag
 #define mcbsp_move        mcbsp_debug_move
 #define mcbsp_hpmove      mcbsp_debug_hpmove
 #define mcbsp_hpput       mcbsp_debug_hpput
 #define mcbsp_hpget       mcbsp_debug_hpget
 #define mcbsp_set_tagsize mcbsp_debug_set_tagsize

 #define mcbsp_hpsend      mcbsp_debug_hpsend
 #define mcbsp_direct_get  mcbsp_debug_direct_get
#elif MCBSP_MODE == 3
 #define mcbsp_begin       mcbsp_profile_begin
 #define mcbsp_end         mcbsp_profile_end
 #define mcbsp_init        mcbsp_profile_init
 #define mcbsp_abort       mcbsp_profile_abort
 #define mcbsp_vabort      mcbsp_profile_vabort
 #define mcbsp_nprocs      mcbsp_profile_nprocs
 #define mcbsp_pid         mcbsp_profile_pid
 #define mcbsp_time        mcbsp_profile_time
 #define mcbsp_sync        mcbsp_profile_sync
 #define mcbsp_push_reg    mcbsp_profile_push_reg
 #define mcbsp_pop_reg     mcbsp_profile_pop_reg
 #define mcbsp_put         mcbsp_profile_put
 #define mcbsp_get         mcbsp_profile_get
 #define mcbsp_send        mcbsp_profile_send
 #define mcbsp_qsize       mcbsp_profile_qsize
 #define mcbsp_get_tag     mcbsp_profile_get_tag
 #define mcbsp_move        mcbsp_profile_move
 #define mcbsp_hpmove      mcbsp_profile_hpmove
 #define mcbsp_hpput       mcbsp_profile_hpput
 #define mcbsp_hpget       mcbsp_profile_hpget
 #define mcbsp_set_tagsize mcbsp_profile_set_tagsize

 #define mcbsp_hpsend      mcbsp_profile_hpsend
 #define mcbsp_direct_get  mcbsp_profile_direct_get
#else
 #define mcbsp_begin       mcbsp_perf_begin
 #define mcbsp_end         mcbsp_perf_end
 #define mcbsp_init        mcbsp_perf_init
 #define mcbsp_abort       mcbsp_perf_abort
 #define mcbsp_vabort      mcbsp_perf_vabort
 #define mcbsp_nprocs      mcbsp_perf_nprocs
 #define mcbsp_pid         mcbsp_perf_pid
 #define mcbsp_time        mcbsp_perf_time
 #define mcbsp_sync        mcbsp_perf_sync
 #define mcbsp_push_reg    mcbsp_perf_push_reg
 #define mcbsp_pop_reg     mcbsp_perf_pop_reg
 #define mcbsp_put         mcbsp_perf_put
 #define mcbsp_get         mcbsp_perf_get
 #define mcbsp_send        mcbsp_perf_send
 #define mcbsp_qsize       mcbsp_perf_qsize
 #define mcbsp_get_tag     mcbsp_perf_get_tag
 #define mcbsp_move        mcbsp_perf_move
 #define mcbsp_hpmove      mcbsp_perf_hpmove
 #define mcbsp_hpput       mcbsp_perf_hpput
 #define mcbsp_hpget       mcbsp_perf_hpget
 #define mcbsp_set_tagsize mcbsp_perf_set_tagsize

 #define mcbsp_hpsend      mcbsp_perf_hpsend
 #define mcbsp_direct_get  mcbsp_perf_direct_get
#endif

//now account for possible compatibility mode translations
#ifdef MCBSP_COMPATIBILITY_MODE 
static inline void bsp_begin( const int P ) {
	mcbsp_begin( (bsp_nprocs_t)P );
}

static inline void bsp_end( void ) {
	mcbsp_end();
}

static inline void bsp_init( void (*spmd)(void), int argc, char **argv ) {
	mcbsp_init( spmd, argc, argv );
}

static inline void bsp_abort( const char * const error_message, ... ) {
	va_list args;
	va_start( args, error_message );
	mcbsp_vabort( error_message, args );
	va_end( args );
}

static inline void bsp_vabort( const char * const error_message, va_list args ) {
	mcbsp_vabort( error_message, args );
}

static inline int bsp_nprocs( void ) {
	return (int)mcbsp_nprocs();
}

static inline int bsp_pid( void ) {
	return (int)mcbsp_pid();
}

static inline double bsp_time( void ) {
	return mcbsp_time();
}

static inline void bsp_sync( void ) {
	mcbsp_sync();
}

static inline void bsp_push_reg( void * const address, const int size ) {
	mcbsp_push_reg( address, (bsp_size_t)size );
}

static inline void bsp_pop_reg( void * const address ) {
	mcbsp_pop_reg( address );
}

static inline void bsp_put(
	const int pid,
	const void * const source,
	const void * const destination,
	const int offset,
	const int size
) {
	mcbsp_put( (bsp_pid_t)pid, source, destination, (bsp_size_t)offset, (bsp_size_t)size );
}

static inline void bsp_get(
	const int pid,
	const void * const source,
	const int offset,
	void * const destination,
	const int size
) {
	mcbsp_get( (bsp_pid_t)pid, source, (bsp_size_t)offset, destination, (bsp_size_t)size );
}

static inline void bsp_set_tagsize( int * const size ) {
	size_t convert = (size_t) *size;
	mcbsp_set_tagsize( &convert );
	*size = (int)convert;
}

static inline void bsp_send(
	const int pid,
	const void * const tag,
	const void * const payload,
	const int size
) {
	mcbsp_send( (bsp_pid_t)pid, tag, payload, (bsp_size_t)size );
}

#ifdef MCBSP_ENABLE_HP_DIRECTIVES
static inline void bsp_hpsend(
	const int pid,
	const void * const tag,
	const void * const payload,
	const int size
) {
	mcbsp_hpsend( (bsp_pid_t)pid, tag, payload, (bsp_size_t)size );
}
#endif

static inline void bsp_qsize(
	int * const packets,
	int * const accumulated_size
) {
	bsp_nprocs_t conv_packets;
	if( accumulated_size == NULL ) {
		mcbsp_qsize( &conv_packets, NULL );
	} else {
		bsp_size_t conv_size;
		mcbsp_qsize( &conv_packets, &conv_size );
		*accumulated_size = (int)conv_size;
	}
	*packets = (int)conv_packets;
}

static inline void bsp_get_tag(
	int * const status,
	void * const tag
) {
	bsp_size_t converted;
	mcbsp_get_tag( &converted, tag );
	if( converted != SIZE_MAX ) {
		*status = (int)converted;
	} else {
		*status = -1;
	}
}

static inline void bsp_move( void * const payload, const int max_copy_size ) {
	mcbsp_move( payload, (const bsp_size_t)max_copy_size );
}

static inline int bsp_hpmove( void* * const p_tag, void* * const p_payload ) {
	return (int)mcbsp_hpmove( p_tag, p_payload );
}

static inline void bsp_hpput(
	const int pid,
	const void * const source,
        const void * const destination,
	const int offset,
	const int size
) {
	mcbsp_hpput( (const bsp_pid_t)pid, source, destination, (const bsp_size_t)offset, (const bsp_size_t)size );
}

static inline void bsp_hpget(
	const int pid,
	const void * const source,
        const int offset,
	void * const destination,
	const int size
) {
	mcbsp_hpget( (const bsp_pid_t)pid, source, (const bsp_size_t)offset, destination, (const bsp_size_t)size );
}
#else
/**
 * The first statement in an SPMD program. Spawns P threads.
 *
 * If the bsp_begin() is the first statement in a program (i.e.,
 * it is the first executable statement in main), then a call to
 * bsp_init prior to bsp_begin is not necessary.
 *
 * Normally main() intialises the parallel computation and
 * provides an entry-point for the SPMD part of the computation
 * using bsp_init. This entry-point is a function wherein the
 * first executable statement is this bsp_begin primitive.
 *
 * Note: with MCBSP_COMPATIBILITY_MODE defined, P is of type int.
 *
 * @param P The number of threads requested for the SPMD program.
 *
 * @ingroup SPMD
 */
static inline void bsp_begin( const bsp_pid_t P ) {
	mcbsp_begin( P );
}

/**
 * Terminates an SPMD program.
 *
 * This should be the last statement in a block of code opened
 * by bsp_begin. It will end the thread which calls this primitive,
 * unless the current thread is the originator of the parallel run.
 * Only that original thread will resume with any remaining
 * sequential code.
 *
 * @ingroup SPMD
 */
static inline void bsp_end( void ) {
	mcbsp_end();
}

/**
 * Prepares for parallel SPMD execution of a given program.
 *
 * Provides an entry-point for new threads when spawned by
 * a bsp_begin. To properly start a new thread on
 * distributed-memory machines, as was the goal of BSPlib,
 * the program arguments are also supplied.
 *
 * The function spmd points is assumed to have bsp_begin as
 * its first statement, and bsp_end as its final statement.
 *
 * @param spmd Entry point for new threads in the SPMD program.
 * @param argc Number of supplied arguments in argv.
 * @param argv List of the application arguments.
 *
 * @ingroup SPMD
 */
static inline void bsp_init( void (*spmd)(void), int argc, char **argv ) {
	mcbsp_init( spmd, argc, argv );
}

/**
 * Aborts an SPMD program.
 *
 * Will print an error message once and notifies other threads
 * in the SPMD program to abort. Other threads stop at the next
 * bsp_sync or bsp_end at latest.
 * The error message is in the regular C printf format, with a
 * variable number of arguments.
 *
 * @param error_message The error message to display.
 *
 * @ingroup SPMD
 */
static inline void bsp_abort( const char * const error_message, ... ) {
	va_list args;
	va_start( args, error_message );
	mcbsp_vabort( error_message, args );
	va_end( args );
}

/**
 * Alternative bsp_abort call.
 *
 * Works with a va_list reference to pass a variable number of
 * arguments. Implemented primarily for if a user wants to
 * wrap bsp_abort in his or her own function with variable
 * arguments.
 *
 * @param error_message The error message to display.
 * @param args A list of arguments needed to display the
 *             above error_message.
 *
 * @ingroup SPMD
 */
static inline void bsp_vabort( const char * const error_message, va_list args ) {
	mcbsp_vabort( error_message, args );
}

/**
 * Returns the number of available processors or processes,
 * depending on whether we are inside or outside an BSP SPMD
 * section.
 *
 * Outside an SPMD section, this primitive returns the number
 * of BSP processors; i.e., the number of single units of
 * execution, where each such unit is capable of running a
 * BSP process. Note that the notion of a BSP process is one
 * that lives in the BSP algorithmic model, and the notion of
 * a BSP processor is one from the BSP computer model.
 * On modern architectures, this primitive returns the number
 * of supported hardware threads on the current architecture.
 *
 * Inside an SPMD section, this primitive returns the number
 * of SPMD processes that are active within the current BSP
 * run.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, the return
 *       type of this function is `int'. Otherwise, it is
 *       of type `unsigned int'.
 *
 * @return The number of available processes (inside SPMD).
 *         or the number of available BSP processors.
 *
 * @ingroup SPMD
 */
static inline bsp_pid_t bsp_nprocs( void ) {
	return mcbsp_nprocs();
}

/**
 * Returns a unique BSP process identifier. This function
 * may only be called from within a BSP SPMD section.
 *
 * A BSP process identifier during an SPMD run is a unique
 * integer from the set \f$ \{0,1,\ldots,P-1\} \f$, where P
 * is the value returned by bsp_nprocs.
 * This unique identifier is constant during the entire SPMD
 * program, that is, from bsp_begin until bsp_end.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, the return
 *       type of this function is `int'. Otherwise, it is 
 *       of type `unsigned int'.
 *
 * @return The unique BSP process ID.
 *
 * @ingroup SPMD
 */
static inline bsp_pid_t bsp_pid( void ) {
	return mcbsp_pid();
}

/**
 * Indicates the elapsed time in this SPMD run. This function
 * may only be called from within a BSP SPMD section.
 *
 * Returns the time elapsed since the start of the current
 * BSP process. Time is returned in seconds. The timers across
 * the various BSP processes from a single SPMD program are
 * not necessarily synchronised. (I.e., this is not a global
 * timer!).
 *
 * @return Elapsed time since the local bsp_begin, in seconds.
 *
 * @ingroup SPMD
 */
static inline double bsp_time( void ) {
	return mcbsp_time();
}

/**
 * Signals the end of a superstep and starts global
 * synchronisation.
 *
 * A BSP SPMD program is logically separated in
 * supersteps; during a superstep threads may perform
 * local computations and buffer communication
 * requests with other BSP threads.
 * These buffered communication requests are executed
 * during synchronisation. When synchronisation ends,
 * all communication is guaranteed to be finished and
 * threads start executing the next superstep.
 *
 * The available communication-queueing requests
 * during a superstep are: bsp_put, bsp_get, bsp_send.
 * Those primitives strictly adhere to the separation
 * of supersteps and communication. Other communication
 * primitives, bsp_hpput, bsp_hpget and bsp_direct_get
 * are less strict in this separation; refer to their
 * documentation for details.
 *
 * @ingroup SPMD
 */
static inline void bsp_sync( void ) {
	mcbsp_sync();
}

/**
 * Registers a memory area for communication.
 *
 * If an SPMD program defines a local variable x,
 * each of the P threads actually has its own memory
 * areas associated with that variable.
 * Communication requires threads to be aware of the
 * memory location of a destination variable. This
 * function achieves this.
 * The order of variable registration must be the
 * same across all threads in the SPMD program. The
 * size of the registered memory block may differ
 * from thread to thread. Registration takes effect
 * only after a synchronisation.
 *
 * Issuing a push_reg counts as a p-relation during
 * the next bsp_sync, worst case.
 *
 * @bug Due to an implementation choice, the overhead
 * of pushing k variables across all SPMD processes
 * is k^2. This is no issue if k is small, and scales
 * in a realistic sense if \f$ k^2 \leq P \f$.
 * An alternative implementation could reduce this
 * overhead cost to \f$ k\log k \f$ (red/black tree,
 * worst case) or k (hashmap, average case) albeit
 * at the cost of using (increasingly) more memory.
 * Contact the maintainers if this variant is indeed
 * preferable.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, size
 *       will be of type `int'. Otherwise, it is of
 *       type `size_t'.
 *
 * @param address Pointer to the memory area to register.
 * @param size    Size, in bytes, of the area to register.
 *
 * @ingroup DRMA
 */
static inline void bsp_push_reg( void * const address, const bsp_size_t size ) {
	mcbsp_push_reg( address, size );
}

/**
 * De-registers a pushed registration.
 *
 * Makes a memory region unavailable for communication.
 * The region should first have been registered using
 * bsp_push_reg, otherwise a run-time error will result.
 *
 * If the same memory address is registered multiple 
 * times, only the latest registration is cancelled.
 *
 * The order of deregistrations must be the same across
 * all threads to ensure correct execution. Like with
 * bsp_push_reg, this is entirely the responsibility
 * of the programmer; MulticoreBSP does check for
 * correctness (it cannot efficiently do so).
 *
 * Issuing a pop_reg counts as a p-relation during the
 * next bsp_sync, worst case.
 *
 * @param address Pointer to the memory region to
 *                deregister.
 *
 * @ingroup DRMA
 */
static inline void bsp_pop_reg( void * const address ) {
	mcbsp_pop_reg( address );
}

/**
 * Put data in a remote memory location.
 *
 * This is a non-blocking communication request.
 * Communication will be executed during the next 
 * synchronisation step. The remote memory location must 
 * be registered using bsp_push_reg in a previous 
 * superstep.
 *
 * The data to be communicated to the remote area will 
 * be buffered on request; i.e., the source memory 
 * location is free to change after this communication 
 * request; the communicated data will not reflect 
 * those changes.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, pid, 
 *       offset and size are of type `int'. Otherwise,
 *       pid is of type `unsigned int', and offset and
 *       size of type `size_t'.
 *
 * @param pid         The ID number of the remote thread.
 * @param source      Pointer to the source data.
 * @param destination Pointer to the registered remote 
 *                    memory area to send data to.
 * @param offset      Offset (in bytes) of the memory 
 *                    area. Offset must be positive and 
 *                    less than the remotely registered 
 *                    memory size.
 * @param size        Size (in bytes) of the data to be
 *                    communicated; i.e., all the data 
 *                    from address source up to address 
 *                    (source + size) at the current 
 *                    thread, is copied to 
 *                    (destination+offset) up to 
 *                    (destination+offset+size) at the 
 *                    thread with ID pid.
 *
 * @ingroup DRMA
 */
static inline void bsp_put(
	const bsp_pid_t pid,
	const void * const source,
	const void * const destination,
	const bsp_size_t offset,
	const bsp_size_t size
) {
	mcbsp_put( pid, source, destination, offset, size );
}

/**
 * Get data from a remote memory location.
 *
 * This is a non-blocking communication request.
 * Communication will be executed during the next
 * synchronisation step. The remote memory location must
 * be registered using bsp_push_reg in a previous
 * superstep.
 *
 * The data retrieved will be the data at the remote
 * memory location at the time of synchronisation. It
 * will not (and cannot) retrieve data at "this"
 * point in the SPMD program at the remote thread.
 * If other communication at the remote process
 * would change the data at the region of interest,
 * these changes are not included in the retrieved
 * data; in this sense, the get is buffered.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, pid, 
 *       offset and size are of type `int'. Otherwise,
 *       pid will be of type `unsigned int' and
 *       offset and size of type `size_t'.
 *
 * @param pid         The ID number of the remote thread.
 * @param source      Pointer to the registered remote 
 *                    memory area where to get data from.
 * @param offset      Offset (in bytes) of the remote
 *                    memory area. Offset must be 
 *                    positive and must be less than
 *                    the remotely registered  memory
 *                    size.
 * @param destination Pointer to the local destination
 *                    memory area.
 * @param size        Size (in bytes) of the data to be
 *                    communicated; i.e., all the data 
 *                    from address (source+offset) up
 *                    to address (source+offset+size)
 *                    at the remote thread, is copied
 *                    to destination up to 
 *                    (destination+size) at this thread.
 *
 * @ingroup DRMA
 */
static inline void bsp_get(
	const bsp_pid_t pid,
	const void * const source,
	const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
) {
	mcbsp_get( pid, source, offset, destination, size );
}

/**
 * Get data from a remote memory location.
 *
 * This is a blocking communication primitive:
 * communication is executed immediately and is not
 * queued until the next synchronisation step. The
 * remote memory location must be registered using 
 * bsp_push_reg in a previous superstep.
 *
 * The data retrieved will be the data at the remote
 * memory location at "this" time. There is no
 * guarantee that the remote thread is at the same
 * position in executing the SPMD program; it might
 * be anywhere in the current superstep. If the 
 * remote thread writes to the source memory block
 * in this superstep, the retrieved data may
 * partially consist of old and new data; this
 * function does not buffer nor is it atomic in
 * any way.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, pid, 
 *       offset and size are of type `int'. Otherwise,
 *       pid is of type `unsigned int' and offset &
 *       size of type `size_t'.
 *
 * @param pid         The ID number of the remote thread.
 * @param source      Pointer to the registered remote 
 *                    memory area where to get data from.
 * @param offset      Offset (in bytes) of the remote
 *                    memory area. Offset must be 
 *                    positive and must be less than
 *                    the remotely registered  memory
 *                    size.
 * @param destination Pointer to the local destination
 *                    memory area.
 * @param size        Size (in bytes) of the data to be
 *                    communicated; i.e., all the data 
 *                    from address source up to address 
 *                    (source + size) at the remote
 *                    thread, is copied to 
 *                    (destination+offset) up to 
 *                    (destination+offset+size) at this
 *                    thread.
 *
 * @ingroup DRMA
 *
 * @remark This function was first proposed in MulticoreBSP
 *         for Java by Yzelman & Bisseling (2012), then
 *         included in the MulticoreBSP for C library by
 *         Yzelman et al. (2014).
 *         It is *not* an original BSPlib primitive as 
 *         defined by Hill et al. (1998), and should never
 *         be introduced or used for distributed-memory
 *         communication.
 */
static inline void bsp_direct_get(
	const bsp_pid_t pid,
	const void * const source,
        const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
) {
	mcbsp_direct_get( pid, source, offset, destination, size );
}

/**
 * Sets the tag size of inter-thread messages.
 *
 * bsp_send can be used to send message tuples
 * (tag,payload). Tag must be of a fixed size,
 * the payload size may differ per message.
 *
 * This function sets the tagsize to the given
 * size. This new size will be valid from the
 * next bsp_sync() on.
 * All processes must call bsp_set_tagsize with
 * the same new size, or MulticoreBSP for C
 * will return an error.
 *
 * The given size will be overwritten with the
 * currently active tagsize upon function exit,
 * where `currently active' means the tag size
 * as expected by BSP if bsp_send were executed
 * during this superstep.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined,
 *       size is of type `int*'. Otherwise, it is
 *       of type `size_t*'.
 *
 * @param size Pointer to the new tag size
 *             (in bytes). On exit, the memory
 *             area pointed to is changed to
 *             reflect the old tag size.
 *
 * @ingroup BSMP
 */
static inline void bsp_set_tagsize( bsp_size_t * const size ) {
	mcbsp_set_tagsize( size );
}

/**
 * Sends a message to a remote thread.
 *
 * A message is actually a tuple (tag,payload). Tag is of a fixed size
 * (see bsp_set_tagsize), the payload size is set per message. Messages
 * will be available at the destination thread in the next superstep.
 *
 * Note: If MCBSP_COMPATIBILITY_MODE is defined, then pid and size are
 *       of type `int'. Otherwise, pid is of type `unsigned int' and
 *       size of type `size_t'.
 *
 * @param pid     ID of the remote thread to send this message to.
 * @param tag     Pointer to the tag data.
 * @param payload Pointer to the payload data.
 * @param size    Size of the payload data (in bytes).
 *
 * @ingroup BSMP
 */
static inline void bsp_send(
	const bsp_pid_t pid,
	const void * const tag,
	const void * const payload,
	const bsp_size_t size
) {
	mcbsp_send( pid, tag, payload, size );
}

#ifdef MCBSP_ENABLE_HP_DIRECTIVES
/**
 * This is a non-buffering and non-blocking send request.
 *
 * The function differs from the regular bsp_send in two major ways:
 * (1) the actual send communication may occur between now and the
 *     end of the next synchronisation phase, and
 * (2) the tag and payload data is read and sent somewhere between
 *     now and the end of the next synchronisation phase.
 * If you change the contents of the memory areas corresponding to
 * the tag and payload pointers before the next bsp_sync, the
 * communication induced by this bsp_hpsend will be undefined.
 *
 * The semantics of BSMP remain unchanged: the sent messages will
 * only become available at the remote processor when the next
 * computation superstep begins. The performance gain is two-fold:
 * (a) bsp_hpsend avoids buffering-on-send, and
 * (b) BSP may send messages during a computation phase, thus
 *     overlapping computation with communication.
 * Ad (a): normally, BSMP in BSP copies tag and payload data at
 * least three times. It buffers on bsp_send (buffer-on-send),
 * it buffers at the receiving processes' incoming BSMP queue 
 * (buffer-on-receive), and finally bsp_get_tag and bsp_move 
 * copy the data in the target user-supplied memory areas. To 
 * also eliminate the latter data movement, please consider
 * using bsp_hpmove.
 * Ad (b): it is not guaranteed this overlap results in faster 
 * execution time. You should think about if using these high-
 * performance primitives makes sense on a per-application 
 * basis, and factor in the extra costs of structuring your 
 * algorithm to enable correct use of these primitives.
 *
 * See bsp_send for general remarks about using BSMP primitives
 * in BSP. See bsp_hpget and bsp_hpput for equivalent (non-
 * buffering and non-blocking) high-performance communication
 * requests.
 *
 * Note: If MCBSP_COMPATIBILITY_MODE is defined, then pid and
 *       size are of type `int'. Otherwise, pid is of type 
 *       `unsigned int' and size of type `size_t'.
 *
 * @param pid     ID of the receiving thread.
 * @param tag     Pointer to the tag data
 *                (actual data is *not* buffered)
 * @param payload Pointer to the payload data.
 *                (actual data is *not* buffered)
 * @param size    Size of the payload data (in bytes).
 *
 * @ingroup BSMP
 *
 * @remark This function first appeared in MulticoreBSP
 *         for C by Yzelman et al. (2014). It is *not*
 *         an original BSPlib primitive as defined by
 *         Hill et al. (1998).
 */
static inline void bsp_hpsend(
	const bsp_pid_t pid,
	const void * const tag,
	const void * const payload,
	const bsp_size_t size
) {
	mcbsp_hpsend( pid, tag, payload, size );
}
#endif

/**
 * Queries the size of the incoming BSMP queue.
 *
 * @param packets Where to store the number of incoming messages.
 * @param accumulated_size Where to store the accumulated size
 * 		of all incoming messages (optional, can be NULL).
 *
 * @ingroup BSMP
 */
static inline void bsp_qsize(
	bsp_nprocs_t * const packets,
	bsp_size_t * const accumulated_size
) {
	mcbsp_qsize( packets, accumulated_size );
}

/**
 * Retrieves the tag of the first message in the queue of 
 * incoming messages.
 *
 * Also retrieves the size of the payload in that message.
 *
 * If there are no messages waiting in queue, status
 * will be set to the maximum possible value and no
 * tag data is retrieved.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, status
 *       is of type `int*', and *status will be set to
 *       -1 if there are no incoming messages.
 *       Otherwise, status is of type `unsigned int'.
 *
 * @param status Pointer to where to store the payload
 *               size (in bytes) of the first incoming
 *               message.
 * @param tag    Pointer to where to store the tag of
 *               the first incoming message.
 *
 * @ingroup BSMP
 */
static inline void bsp_get_tag(
	bsp_size_t * const status,
	void * const tag
) {
	mcbsp_get_tag( status, tag );
}

/**
 * Retrieves the payload from the first message in the
 * queue of incoming messages, and removes that message.
 *
 * If the incoming queue is empty, this function has
 * no effect. This function will copy a given maximum
 * of bytes from the message payload into a supplied
 * buffer. This maximum should equal or be larger
 * than the payload size (which can, e.g.,  be 
 * retrieved via bsp_get_tag).
 * The maximum can be 0 bytes; the net effect is
 * the efficient removal of the first message from
 * the queue.
 *
 * Note that BSMP (BSMP)
 * is doubly buffered: bsp_send buffers on send and
 * this function buffers again on receives.
 *
 * See bsp_hpmove if buffer-on-receive is unwanted.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined,
 *       max_copy_size is of type `int'. Otherwise,
 *       it is of type `size_t'.
 *
 * @param payload       Where to copy the message 
 *                      payload into.
 * @param max_copy_size The maximum number of
 *                      bytes to copy.
 *
 * @ingroup BSMP
 */
static inline void bsp_move( void * const payload, const bsp_size_t max_copy_size ) {
	mcbsp_move( payload, max_copy_size );
}

/**
 * Unbuffered retrieval of the payload of the first
 * message in the incoming message queue.
 *
 * The BSPlib implementation guarantees the 
 * returned memory areas remain valid during the
 * current computation phase.
 *
 * See bsp_move for general remarks. This function
 * returns a pointer to an area in the incoming
 * message buffer where the requested message tag
 * and message payload reside.
 * The function returns the size (in bytes) of the
 * message payload.
 *
 * If the BSMP queue is empty, this function will
 * return SIZE_MAX and leave p_tag and p_payload 
 * as is.
 *
 * Take care *not* to free the memory referred to
 * by these pointers; the data resides in a 
 * MulticoreBSP-managed buffer area and will be
 * freed by the BSP run-time system.
 *
 * Warning: pointers obtained using this primitive
 *          will no longer be valid after the next
 *          bsp_sync.
 *
 * Warning: use this high-performance function
 *          with care; out of bound access on tag
 *          or payload can easily cause segfaults.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined,
 *       then (1) the return type of this function
 *       is `int' (instead of `size_t'), and (2)
 *       should the incoming message queue be empty,
 *       this function returns -1 instead of
 *       SIZE_MAX.
 *
 * @param p_tag     Where to store the pointer to 
 *                  the message tag. This parameter
 *                  cannot equal NULL.
 * @param p_payload Where to store the pointer to
 *                  the message payload. This
 *                  parameter cannot equal NULL.
 *
 * @return Size (in bytes) of the payload, or
 *         SIZE_MAX if the message queue was empty.
 *
 * @ingroup BSMP
 */
static inline bsp_size_t bsp_hpmove( void* * const p_tag, void* * const p_payload ) {
	return mcbsp_hpmove( p_tag, p_payload );
}

/**
 * Put data in a remote memory location.
 *
 * This is a non-blocking communication request.
 * Communication will be executed sometime between now 
 * and during the next synchronisation step. Note that 
 * this differs from bsp_put. Communication is guaranteed 
 * to have finished before the next superstep. Note this
 * means that both source and destination memory areas 
 * might be read and written to at any time after 
 * issueing this communication request. This overlap of 
 * communication and computation is the fundamental
 * difference with the standard bsp_put.
 *
 * It is not guaranteed this overlap results in faster 
 * execution time. You should think about if using these 
 * high-performance primitives makes sense on a 
 * per-application basis, and factor in the extra costs 
 * of structuring your algorithm to enable correct use 
 * of these primitives.
 *
 * Otherwise usage is similar to that of bsp_put;
 * please refer to that function for further
 * documentation.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, pid, 
 *       offset and size are of type `int'. Otherwise,
 *       pid is of type `unsigned int', offset and
 *       size of type `size_t'.
 *
 * @param pid         The ID number of the remote thread.
 * @param source      Pointer to the source data.
 * @param destination Pointer to the registered remote 
 *                    memory area to send data to.
 * @param offset      Offset (in bytes) of the memory 
 *                    area. Offset must be positive and 
 *                    less than the remotely registered 
 *                    memory size.
 * @param size        Size (in bytes) of the data to be
 *                    communicated; i.e., all the data 
 *                    from address source up to address 
 *                    (source + size) at the current 
 *                    thread, is copied to 
 *                    (destination+offset) up to 
 *                    (destination+offset+size) at the 
 *                    thread with ID pid.
 *
 * @ingroup DRMA
 */
static inline void bsp_hpput(
	const bsp_pid_t pid,
	const void * const source,
        const void * const destination,
	const bsp_size_t offset,
	const bsp_size_t size
) {
	mcbsp_hpput( pid, source, destination, offset, size );
}

/**
 * Get data from a remote memory location.
 *
 * This is a non-blocking communication request.
 * Communication will be executed between now and the
 * next synchronisation step. Note that this differs
 * from bsp_get. Communication is guaranteed to have
 * finished before the next superstep. Note this 
 * means that both source and destination memory 
 * areas might be read and written to at any time
 * after issueing this communication request. This
 * overlap of communication and computation is the
 * fundamental difference with the standard bsp_get.
 *
 * It is not guaranteed this overlap results in faster 
 * execution time. You should think about if using these 
 * high-performance primitives makes sense on a 
 * per-application basis, and factor in the extra costs 
 * of structuring your algorithm to enable correct use 
 * of these primitives.
 *
 * Note the difference between this high-performance
 * get and bsp_direct_get is that the latter function
 * is blocking (performs the communication
 * immediately and waits for it to end).
 *
 * Otherwise usage is similar to that of bsp_get;
 * please refer to that function for further
 * documentation.
 *
 * Note: if MCBSP_COMPATIBILITY_MODE is defined, pid, 
 *       offset and size are of type `int'. Otherwise,
 *       pid is of type `unsigned int' and offset &
 *       size of type `size_t'.
 *
 * @param pid         The ID number of the remote thread.
 * @param source      Pointer to the registered remote 
 *                    memory area where to get data from.
 * @param offset      Offset (in bytes) of the remote
 *                    memory area. Offset must be 
 *                    positive and must be less than
 *                    the remotely registered  memory
 *                    size.
 * @param destination Pointer to the local destination
 *                    memory area.
 * @param size        Size (in bytes) of the data to be
 *                    communicated; i.e., all the data 
 *                    from address source up to address 
 *                    (source + size) at the remote
 *                    thread, is copied to 
 *                    (destination+offset) up to 
 *                    (destination+offset+size) at this
 *                    thread.
 *
 * @ingroup DRMA
 */
static inline void bsp_hpget(
	const bsp_pid_t pid,
	const void * const source,
        const bsp_size_t offset,
	void * const destination,
	const bsp_size_t size
) {
	mcbsp_hpget( pid, source, offset, destination, size );
}
#endif

#endif

