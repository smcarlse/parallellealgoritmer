/*
 * Copyright (c) 2015
 *
 * File created by A. N. Yzelman, February 2015.
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


#include <mcbsp.h>
#include <mcbsp-resiliency.h>
#include <stdio.h>

int main() {
	bsp_begin( bsp_nprocs() );

	//get spmd info
	const unsigned int P = bsp_nprocs();
	const unsigned int s = bsp_pid();

	//sanity check
	if( P > 100 ) {
		bsp_abort( "Program halted, #processes (%u) would cause a buffer overflow in the remainder code. Please take P <= 100.\n" );
	}

	//open up a file, one per process
	char filename[ 14 ];
	sprintf( filename, "file_io_cpr.%d", bsp_pid() );
	FILE * const f = fopen( filename, "w" );
	
	//do phase I access
	fprintf( f, "%u writes phase I  message into this file\n", s );

	//checkpoint
	mcbsp_checkpoint();

	//do phase II acess
	fprintf( f, "%u writes phase II message into this file\n", s );

	//close file
	fclose( f );

	//done
	bsp_end();

}

