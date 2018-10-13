/*
 * Copyright (c) 2013
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

/*! \file
 * Enables BSP programming in the ANSI C++98 standard.
 *
 * Exposes the regular C BSPlib interface of MulticoreBSP
 * for C. Additionally includes a BSP_program class from
 * which C++ BSP classes may be extended. This lives in
 * the mcbsp namespace.
 * This file also includes the MulticoreBSP for C affinity
 * and resiliency extensions. If only base functionality
 * is wanted, use the following include directive instead:
 *
 *     extern "C" {
 *         #include "bsp.h"
 *     }
 *
 * Include mcbsp-templates.hpp to additionally enable
 * templated BSP primitives. These are not automatically
 * included to prevent confusion when porting regular C
 * BSPlib code.
 */

/**
 * \defgroup cpp MulticoreBSP for C extensions for C++ programming
 *
 * MulticoreBSP for C defines a C++ wrapper for object-oriented SPMD control
 * via the mcbsp::BSP_PROGRAM class. Users can extend this class, implement
 * its virtual functions, and call mcbsp::BSP_PROGRAM::begin.
 *
 * There are also extensions available that make the common BSPlib primitives
 * like bsp_get and bsp_put work with the number of elements instead of with
 * bytes. See the file mcbsp-templates.hpp for details.
 */

#ifndef _HPP_MCBSP
#define _HPP_MCBSP

extern "C" {
	#include "bsp.h"
	#include "mcbsp-affinity.h"
	#include "mcbsp-resiliency.h"
}

#include <limits>


/**
 * Namespace in which the C++-style extensions for MulticoreBSP for C reside.
 *
 * @ingroup cpp
 */
namespace mcbsp {

	/**
	 * Abstract class which a user can extend to write BSP programs.
	 *
	 * Class-based BSP programming implies that each SPMD thread executes
	 * the same BSP_program::spmd() program on different class instances;
	 * thus class-local variables are automatically also thread-local.
	 * This is somewhat more elegant than declaring everything locally,
	 * within functions, as is necessary in C.
	 *
	 * Furthermore, the functions bsp_init(), bsp_begin(), and bsp_end()
	 * are implied and no longer need to be called explicitly; users
	 * instead call BSP_program::begin().
	 * 
	 * BSP algorithms using this C++ wrapper extend the BSP_program
	 * class, and provide implementations for the parallel part of the
	 * program in BSP_program::spmd().
	 * The user furthermore supplies the C++ wrapper a means of
	 * constructing new instances of their BSP program by implementing
	 * BSP_program::newInstance(). This lets the wrapper provide a
	 * separate instance to each thread executing the parallel program.
	 * 
	 * @see BSP_program::spmd()
	 * @see BSP_program::newInstance()
	 * @see BSP_program::begin()
	 *
	 * @ingroup cpp
	 */
	class BSP_program {

		private:

			/** The number of threads that should be spawned. */
			size_t P;

		protected:

			/**
			 * Base constructor is only accessible by self,
			 * derived instances, and friends.
			 */ 
			BSP_program() : P( std::numeric_limits< size_t >::max() ) {}

			/**
			 * The parallel SPMD code to be implemented by user.
			 *
			 * @ingroup cpp
			 */
			virtual void spmd() = 0;

			/** 
			 * Creates a new instance of the implementing class,
			 * which will be used by new threads spawned by
			 * bsp_begin().
			 *
			 * Note that this need not be a copy if your BSP
			 * program does not require this! The recommended
			 * implementation is the following:
			 *
			 * virtual BSP_program * newInstance() {
			 * 	return new FinalType();
			 * }
			 *
			 * where FinalType is the name of the BSP program you
			 * are implementing (FinalType must be a non-abstract
			 * subclass of BSP_program).
			 *
			 * @ingroup cpp
			 */
			virtual BSP_program * newInstance() = 0;

			/**
			 * Code that destroys instances creatured using the
			 * newInstance() function. The default implementation
			 * of this virtual function is as follows:
			 *
			 * virtual void destroyInstance( BSP_program * instance ) {
			 * 	delete instance;
			 * }
			 *
			 * Override this implementation only when this would
			 * not yield correct behaviour. Note that when the
			 * newInstance function is implemented as per its
			 * recommendation, no action is required.
			 *
			 * @param instance the instance, created by newInstance,
			 *                 that should be destroyed.
			 *
			 * @see newInstance()
			 *
			 * @ingroup cpp
			 */
			virtual void destroyInstance( BSP_program * const instance );

		public:

			/**
			 * Initialises and starts the current BSP program.
			 * Automatically implies (correct) calls to bsp_init()
			 * and bsp_begin().
			 *
			 * The parallel SPMD section is the overloaded function
			 * spmd() of this class instance.
			 *
			 * The SPMD program makes use of the same globally
			 * defined BSP primitives as available in plain C;
			 * there are no special C++ wrappers for communication.
			 * 
			 * bsp_end() is automatically called after the spmd()
			 * function exits.
			 *
			 * @ingroup cpp
			 */
			void begin( const bsp_pid_t P = bsp_nprocs() );

			/** Abstract classes should have virtual destructors. */
			virtual ~BSP_program();
	};

}

#endif

