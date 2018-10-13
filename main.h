#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "MulticoreBSP-for-C/include/bsp.h"
#include "bitarray.c"
#include <stdbool.h>

void spmd();
void calculateRange(int pid, int cores, int * rangeStart, int * rangeEnd);
Bitarray preProcessingPrimes(int upper);
Bitarray crossOutPrimes(Bitarray bitarray, int rangeStart, int rangeEnd);
int countPrimes(Bitarray bitarray, int bound);

struct GoldBach {
	int prime1;
	int prime2;
};

struct GoldBach* createGoldBachPairs(Bitarray primes, int upperBound);
void printGoldBachArray(struct GoldBach* bacharray, int upperbound);
