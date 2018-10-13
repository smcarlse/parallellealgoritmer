/*
 * Copyright (c) 2015
 * 
 * File created 24/02/2015 by Albert-Jan N. Yzelman
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

/*! \file mcbsp-resiliency.h
 *
 * The MulticoreBSP for C interface for checkpointing
 *
 * The functions and data types defined in this file allow
 * a user to take control of the way MulticoreBSP for C
 * performs automatic checkpointing, and additionally
 * enables manual global checkpointing capabilities.
 *
 * To successfully use the primitives defined here,
 * MulticoreBSP for C should be compiled with the DMTCP
 * library. See the documentation for further details.
 * Using the functions defined in this header without
 * DMTCP support compiled in will result in run-time
 * errors.
 */

/**
 * \defgroup checkpointing MulticoreBSP for C interface to resiliency extensions
 *
 * MulticoreBSP for C supports checkpointing via the DMTCP software package.
 *
 * Checkpointing can occur in one of two modes:
 * <ul>
 *  <li>Manual: the user calls mcbsp_checkpoint() at optimal places within his
 *              or her algorithm;</li>
 *  <li>Automatic: MulticoreBSP for C performs automatic checkpointing given a
 *                 preset superstep frequency.</li>
 * </ul>
 * Checkpoint is only supported on Linux OSes. Full details follow in the below:
 *
 * Manual mode
 * ===========
 * For example use, see examples/checkpointed_loop.c.
 * 
 * Before building, first enable DMTCP in your include.mk
 * file. Then, (re-)build MulticoreBSP for C, and (re-)
 * build the examples; the checkpointed_loop program should
 * now be built automatically.
 * There are two ways to run the checkpointed_loop:
 *    1) issue `dmtcp_launch ./checkpointed_loop', or
 *    2) start `dmtcp_coordinator' as a separate process,
 *       and issue `dmtcp_checkpoint ./checkpointed_loop'
 * This runs the calculation once, and successfully,
 * while writing one checkpoint halfway through the
 * computation.
 * To restart from the saved checkpoint, simply issue
 *    `dmtcp_restart ckpt_checkpointed_loop_<...>'.
 * The brackets <...> contain an identifier that depends
 * on the instance of your run.
 *
 * Automatic mode
 * ==============
 * MCBSP_DEFAULT_CHECKPOINT_FREQUENCY created, and set
 * to 0 (=no automatic checkpointing). The machine.info 
 * file now takes a new key-value `checkpoint_frequency'
 * which should be followed by a positive integer C. If
 * C > 0, then MulticoreBSP automatically checkpoints
 * every C bsp_syncs.
 * The currently active checkpointing frequency can be
 * read with mcbsp_get_checkpointing_frequency, and set
 * using mcbsp_set_checkpointing_frequency, both defined
 * in `mcbsp-resiliency.h'. These values can be get and
 * set during run-time, before bsp_init, within an SPMD
 * section, and in-between.
 * The library optionally also supports an increased
 * checkpointing frequency when failure is imminent. It
 * relies on an auxiliary daemon to create a file named
 *     `/etc/bsp_failure'
 * whenever a part of the hardware fails, or when
 * hardware is detected to become more probable. This
 * file is expected to contain a string that describes
 * the type of hardware trouble detected.
 * When this file is detected to exist, MulticoreBSP
 * switches to a secondary checkpointing frequency
 * defined with key `safe_checkpoint_frequency'. By
 * default, this value equals that of
 *     `checkpoint_frequency'.
 * MulticoreBSP for C will also parse the last two
 * lines of `/etc/bsp_failure' and print these to the
 * standard error stream.
 * Other (imminent) hardware failures detections can
 * rely on the same mechanism, but necessarily will
 * depend on auxiliary daemons for failure detection
 * since such detections are highly platform dependent
 * endeavours.
 */

#ifndef _H_MCBSP_RESILIENCY
#define _H_MCBSP_RESILIENCY

#include <stdlib.h>

/** The default checkpointing frequency. */
extern size_t MCBSP_DEFAULT_CHECKPOINT_FREQUENCY;

/**
 * Synchronises all processes and performs a global
 * checkpointing step.
 *
 * To optimise checkpointing performance, users
 * should free all unused or unnecessary memory
 * areas before calling this function.
 * Should MulticoreBSP for C have been compiled
 * without checkpointing support, a call to this
 * primitive will result in an abort of the SPMD run
 * via the bsp_abort primitive.
 *
 * @ingroup checkpointing
 */
void mcbsp_checkpoint( void );

/**
 * Enables automatic checkpointing. When automatic checkpointing was 
 * already active, calling this function has no effect.
 *
 * @ingroup checkpointing
 */
void mcbsp_enable_checkpoints( void );

/**
 * Disables automatic checkpointing. When automatic checkpointing already was
 * inactive, calling this function has no effect.
 *
 * @ingroup checkpointing
 */
void mcbsp_disable_checkpoints( void );

/**
 * Allows the user to directly set the checkpointing frequency.
 * A zero frequency means no automatic checkpointing will be performed.
 * Calling this function with a non-zero frequency is in no way equivalent or
 * including a call to mcbsp_enable_checkpointing; if checkpointing was
 * disabled using mcbsp_disable_checkpointing, a non-zero checkpointing
 * frequency will have no effect until after a next mcbsp_enable_checkpointing.
 *
 * This function may be called before bsp_init, from within an SPMD section,
 * and anywhere in-between. It will both change the run-time settings (when
 * applicable) as well as change the machine model (i.e., the machine.info).
 *
 * @param _cp_f The new checkpointing frequency.
 *
 * @ingroup checkpointing
 */
void mcbsp_set_checkpoint_frequency( size_t _cp_f );

/**
 * Gets the currently active checkpointing frequency.
 * Will return the frequency that would have been adhered to if checkpointing
 * was active; i.e., even after a mcbsp_disable_checkpointing this function
 * may return a non-zero frequency.
 * A zero frequency means automatic checkpointing is completely disabled.
 *
 * This function may be called before bsp_init, from within an SPMD section,
 * and anywhere in-between. It will both change the run-time settings (when
 * applicable) as well as change the machine model (i.e., the machine.info).
 *
 * @return The currently active checkpointing frequency.
 *
 * @ingroup checkpointing
 */
size_t mcbsp_get_checkpoint_frequency( void );

/**
 * Allows the user to set the safe checkpointing frequency.
 * If this value has not been set, it will equal that of the normal
 * checkpointing frequency as may be set by mcbsp_set_checkpoint_frequency.
 * Safe checkpointing only takes effect when the MulticoreBSP for C library
 * detects the existance of the file `/etc/bsp_failure', which indicates
 * hardware failures have occurred, are occurring, or may be imminent.
 * Creation of this file is done by auxiliary daemons, and never by the BSP
 * run-time, since hardware failure detection is a highly platform-dependent
 * endeavour.
 * 
 * @param _safe_cp_f The checkpointing frequency used in case of (imminent)
 *                  hardware failures.
 *
 * @ingroup checkpointing
 */
void mcbsp_set_safe_checkpoint_frequency( size_t _safe_cp_f );

/**
 * Gets the currently active safe checkpointing frequency.
 * @see mcbsp_get_checkpoint_frequency
 * @see mcbsp_set_safe_checkpoint_frequency
 * 
 * @return The currently active safe checkpointing frequency.
 *
 * @ingroup checkpointing
 */
size_t mcbsp_get_safe_checkpoint_frequency( void );

#endif

