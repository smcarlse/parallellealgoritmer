#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

#define MAX_PRIMES 1000000
#define MAX_PRIMES_ROOT sqrt(MAX_PRIMES)

#define DEBUG

int crossOuts = 0;

void crossOutMultiples(bool *array, int number) {
	for (int i = number * 2; i < MAX_PRIMES; i+=number) {
		crossOuts++;
		array[i] = 0;
	}
}

int main( int argc, char ** argv ) {

    clock_t start = clock();
    
	bool primes[MAX_PRIMES];
	primes[0] = 0;
	primes[1] = 0;
	for (int i = 2; i < MAX_PRIMES; i++){
		primes[i] = 1;
	}


    for (int i = 2; i < MAX_PRIMES / 2; i++) {
    	if (primes[i] == 0) {
    		continue;
    	} else {
    		crossOutMultiples(primes, i);
    	}
    }
    
    clock_t time = clock() - start;

    int sum = 0;

    for (int i = 0; i < MAX_PRIMES; i++) {
    	if (primes[i]) {
    		sum++;
    	}
    }

    printf("Number of primes%d\n", sum);

	printf("Total seconds %ld,%ld\n", (time / CLOCKS_PER_SEC),(time%CLOCKS_PER_SEC));
	printf("Total costs: %d\n", crossOuts);	
    printf("\nPress enter to continue...");
    getchar();

    return EXIT_SUCCESS;
}


