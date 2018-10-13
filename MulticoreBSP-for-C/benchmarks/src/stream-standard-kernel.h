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


#include <stddef.h>

/**
 * Kernel that does one store and one load for each i in (0,1,...,n-1),
 * as follows:
 *                 a[ i ] = b[ i ];
 * 
 * @param a Output vector.
 * @param b Input vector.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel2(
	float * const restrict a, const float * const restrict b,
	const size_t n, const size_t rep
);

/**
 * Kernel that does one store and two loads for each i in (0,1,...,n-1),
 * as follows:
 *                 a[ i ] = b[ i ] + c[ i ];
 * 
 * @param a Output vector.
 * @param b First input vector.
 * @param c Second input vector.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel3a(
	float * const restrict a, const float * const restrict b,
	const float * const restrict c, const size_t n, const size_t rep
);

/**
 * Kernel that does one store and two loads for each i in (0,1,...,n-1),
 * as follows:
 *                 a[ i ] = b[ i ] + c[ i ] * d;

 * Note that this is the same as kernel3a, with the addition of one
 * floating point multiplication. The constant d should be kept in a
 * CPU register throughout the computation and should not cause any
 * additional memory traffic.
 * 
 * @param a Output vector.
 * @param b First input vector.
 * @param c Second input vector.
 * @param d A constant floating point scalar.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel3b(
	float * const restrict a, const float * const restrict b,
	const float * const restrict c, const float d,
	const size_t n, const size_t rep
);

/**
 * Kernel that does one store and three loads for each i in (0,1,...,n-1),
 * as follows:
 *                 a[ i ] = b[ i ] + c[ i ] * d[ i ];

 * @param a Output vector.
 * @param b First input vector.
 * @param c Second input vector.
 * @param d Third input vector.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel4(
	      float * const restrict a, const float * const restrict b,
	const float * const restrict c, const float * const restrict d,
	const size_t n, const size_t rep
);

/**
 * Kernel that does one store and one load for each i in (0,1,...,n-1),
 * in-place, as follows:
 *                 a[ i ] = a[ i ] * b;
 * 
 * Note that this is the same as kernel1, with the addition of one
 * floating point multiplication. The constant b should be kept in a
 * CPU register throughout the computation and should not cause any
 * additional memory traffic.
 *
 * @param a IO vector.
 * @param b A constant scalar.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel2i(
	float * const restrict a, const float b,
	const size_t n, const size_t rep
);

/**
 * Kernel that does one store and two loads for each i in (0,1,...,n-1),
 * in-place, as follows:
 *                 a[ i ] = a[ i ] + b[ i ];
 * 
 * @param a Output vector.
 * @param b First input vector.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel3ai(
	float * const restrict a, const float * const restrict b,
	const size_t n, const size_t rep
);

/**
 * Kernel that does one store and two loads for each i in (0,1,...,n-1),
 * in-place, as follows:
 *                 a[ i ] = a[ i ] + b[ i ] * c;

 * Note that this is the same as kernel3ai, with the addition of one
 * floating point multiplication. The constant c should be kept in a
 * CPU register throughout the computation and should not cause any
 * additional memory traffic.
 * 
 * @param a Output vector.
 * @param b First input vector.
 * @param c A constant floating point scalar.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel3bi(
	float * const restrict a, const float * const restrict b,
	const float c, const size_t n, const size_t rep
);

/**
 * Kernel that does one store and three loads for each i in (0,1,...,n-1),
 * in-place, as follows:
 *                 a[ i ] = a[ i ] + b[ i ] * c[ i ];

 * @param a Output vector.
 * @param b First input vector.
 * @param c Second input vector.
 * @param n Vector length (of both a and b).
 * @param rep Number of times to repeat the operation.
 */
void kernel4i(
	      float * const restrict a, const float * const restrict b,
	const float * const restrict c, const size_t n, const size_t rep
);

