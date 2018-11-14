#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "MulticoreBSP-for-C/include/bsp.h"
#include <stdbool.h>

void spmd();

struct GoldBach {
	int prime1;
	int prime2;
};

struct GoldBach* createGoldBachPairs(bool *primes, int upperBound);
void printGoldBachArray(struct GoldBach* bacharray, int upperbound);
