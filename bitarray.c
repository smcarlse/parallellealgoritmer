#include <stdlib.h>
#include <stdio.h>

#define BLOCK_SIZE 32

typedef int BitBlock;
typedef int* Bitarray;

int bitarray_blocks(int amount) {
	return (amount / 32) + 1;
}

/*
 * Create a bitarray,
 */
Bitarray bitarray_create(int *block_amounts, int amount) {
	/* Integers have 32 bits. we divide amount by 32 and add 1 to find how many integers we need to cover the amount*/
	int needed_integers = bitarray_blocks(amount);
	
	*block_amounts = needed_integers;

	Bitarray bitarray = (Bitarray)malloc(needed_integers * sizeof(BitBlock));

	for (int i = 0; i < needed_integers; i++) {
		bitarray[i] = 0;
	}
	return bitarray;
}

int bitarray_get(Bitarray array, int position) {
	
	/* Get integer with needed bit
	 * Shift to right so that needed torrent is on the left
	 * Our bitposition is from right to left, so inverse the position within the integer.
	 * Shift with that amount
	 */
	
	/* Get the last bit, and trash the rest */	
	return (array[position / BLOCK_SIZE] >> (position % BLOCK_SIZE)) & 1;
}

void bitarray_set(Bitarray *array, int position) {
	
	/* Set the bit position to value */
	(*array)[position / BLOCK_SIZE] |= (1 << (position % BLOCK_SIZE));	
}