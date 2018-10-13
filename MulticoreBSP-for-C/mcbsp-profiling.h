/*
 * Copyright (c) 2015 
 * 
 * File created 01/06/2015 by Albert-Jan N. Yzelman
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

/*! \file mcbsp-profiling.h
 *
 * This file provides an API to change the behaviour of MulticoreBSP during
 * profiling mode. Currently, the only added functionality is to change the
 * current superstep name, so to enable easier analysis of profiling output.
 */

/**
 * \defgroup profiling MulticoreBSP for C profiling extension
 *
 * The functions defined here provide an interface to the profiling behaviour
 * of MulticoreBSP for C.
 */

#ifndef _H_MCBSP_PROFILING
#define _H_MCBSP_PROFILING

/**
 * When in profile mode, give a discriptive name to the current superstep.
 * This name will be displayed during profile output. If a custom name is
 * not given by means of this function, then the i-th superstep will be
 * simply named as `Superstep i'.
 * Names need not be unique, though every superstep will have its own
 * output record; the results will not be aggregated.
 *
 * Names have a maximum length of 255 chars; any additional characters
 * will be ignored.
 *
 * @param name The descriptive name of the current superstep.
 *
 * @ingroup profiling
 */
void mcbsp_profiling_set_description( const char * const name );

#endif

