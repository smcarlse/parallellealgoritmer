#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "multicore-bsp/include/mcbsp.h"
#include "bitarray.c"

#define MAX_PRIMES 1000
#define MAX_PRIMES_ROOT sqrt(MAX_PRIMES)

#define DEBUG

// Global prime array
Bitarray bitarray;
int int_blocks;

int count_primes(Bitarray bitarray, int start, int max) {
	int sum = 0;
	for (int i = start; i < max && i <= MAX_PRIMES; i++) {
		if (get_bit(bitarray, i) == 0) {
			sum += 1;
		}
	}
	printf("count:%d->%d = %d\n", start, max, sum);
	return sum;
}

void print_primes(Bitarray bitarray, int max) {
	for (int i = 2 ; i < max; i++) {
		if (get_bit(bitarray, i) == 0) {
			printf("%d, ", i);
		}
	}
	printf("\n");
}

void preProcessPrimes(int amount) {
	int root = (int)ceil(sqrt(amount));
	for (int i = 2; i <= root; i++) {
		if (get_bit(bitarray, i) == 0) {
			cross_out(&bitarray, i, 0, amount * BLOCK_SIZE, 0);
		}
	}		
	printf("Preprocess primes till %d: %d\n\n", amount, count_primes(bitarray, 2, amount));
}

void cross_out(Bitarray *bitarray, int prime, int block_start, int block_end, int inclusive) {
	//Get first number to cross out
	int start = ceil(((block_start * BLOCK_SIZE) / (double)prime)) * prime;	
	
	//printf("start: %d/%d, ", start, prime);
	
	int i = inclusive ? 0 : 2;
	for (; start + i * prime <= block_end * BLOCK_SIZE && start + i*prime <= MAX_PRIMES; i++) {
		//printf("%d, ", start+i*prime);
		set_bit(bitarray, start + i * prime);
	}	
	//printf("\n\n");
}



void spmd() {

    /* Calculate cores */
	double time = 0;
	double start_time = 0;
    int cores = bsp_nprocs();
	cores = 4;


	//Begin multistuff 
	bsp_begin(cores);
	int pid = bsp_pid();
	start_time = bsp_time();


	int *localSumArray = (int*)malloc(sizeof(int) * cores);
	bsp_push_reg(localSumArray, sizeof(int) * cores);


	// Calculate the amount of primes we will calculate before
	int preProcessPrimeBlocks = ceil(MAX_PRIMES_ROOT / BLOCK_SIZE);
	int preProcessPrimeAmount = preProcessPrimeBlocks * BLOCK_SIZE;
	int blocksRemaining = (int_blocks) - preProcessPrimeBlocks;
	

#ifdef DEBUG
	printf("total blocks: %d\n", int_blocks);
	printf("Preprocessing blocks: %d\nPreprocessing totals: %d\n", preProcessPrimeBlocks, preProcessPrimeAmount);
	printf("Blocks left: %d\nTime since start: %f\n\n", blocksRemaining, bsp_time());
	bsp_sync();
#endif
	if (pid == 0) {
		// Calculate the first few primes	
		preProcessPrimes(preProcessPrimeAmount);		
		
	}	
	bsp_sync();
	int calculatedPreProcessPrimes = count_primes(bitarray, 2, preProcessPrimeAmount);

	// The amount of blocks we can divide among the processors
	int startBlock = pid * (blocksRemaining / cores) + preProcessPrimeBlocks;
	int endBlock = (pid+1)* (blocksRemaining / cores) + preProcessPrimeBlocks;
	if (pid == cores - 1) endBlock = blocksRemaining + 1;

	printf("(%d) s:%d, e:%d\n", pid, startBlock, endBlock);
	bsp_sync();
	time = bsp_time();
	//From start to end, cross out primes
	for (int i = 2; i <= preProcessPrimeAmount; i++) {
		if (get_bit(bitarray, i) == 0) {
			cross_out(&bitarray, i,  startBlock, endBlock, 1);
		}
	}
	#ifdef DEBUG
	printf("(%d) cross out time: %f\n", pid, bsp_time() - time);	
	bsp_sync();
	#endif

	int local_sum = count_primes(bitarray, startBlock * BLOCK_SIZE, endBlock * BLOCK_SIZE);
	for (int i = 0; i < cores; i++) {
		bsp_put(i, &local_sum, localSumArray, pid * sizeof(int), sizeof(int));
	}
	#ifdef DEBUG
	printf("Local sum: %d, Total time: %f seconds\n", local_sum, bsp_time() - start_time);
	bsp_sync();
	#endif

	if (pid == 0) {
		int globalSum = calculatedPreProcessPrimes;
		for (int i = 0; i < cores; i++) {
			globalSum += localSumArray[i];
		}
		printf("Total amount of prime numbers: %d", globalSum);
	}

	bsp_end();   
	free(bitarray);
	free(localSumArray);
}

int main( int argc, char ** argv ) {
    printf("cores: %d\n", bsp_nprocs());
	
	// Initiate bitarray
	bitarray = create_bitarray(&int_blocks, MAX_PRIMES);
	for (int i = 0; i < int_blocks; i++) {
		bitarray[i] = 0;
	}
	set_bit(&bitarray, 0);
	set_bit(&bitarray, 1);
    bsp_init( &spmd, argc, argv );	
    spmd();

    printf("\nPress enter to continue...");
    getchar();

    return EXIT_SUCCESS;
}


