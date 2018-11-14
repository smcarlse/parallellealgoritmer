#include "main.h"

#define MAX_PRIMES 1000

#define CORES 2

int crossOuts = 0;
bool primes[MAX_PRIMES];

void crossOutMultiples(bool *array, int start, int end, int number, int pid) {
	for (int i = start; i < end; i++) {
		crossOuts++;
		if(i % number == 0)
			array[i] = 0;
	}
}

int main( int argc, char ** argv ) {
	bsp_init(&spmd, argc, argv );
    printf("cores: %d\n", bsp_nprocs());

	spmd();

    return EXIT_SUCCESS;
}


void spmd() {
	int cores = CORES;

	bool sample[MAX_PRIMES];
	sample[0] = 0;
	sample[1] = 0;
	for (int i = 2; i < MAX_PRIMES; i++){
		sample[i] = 1;
	}
	bsp_begin(cores);
	double start = bsp_time();

	int pid = bsp_pid();


	bool* vector = (bool*)malloc(MAX_PRIMES);

	vector[0] = 0;
	vector[1] = 0;
	for (int i = 2; i < MAX_PRIMES; i++){
		vector[i] = 1;
	}

	bsp_push_reg(vector, MAX_PRIMES);

	bsp_sync();


	for (int i = 2; i < MAX_PRIMES / 2; i++) {
    	if (sample[i] == 0) {
    		continue;
    	} else {
			int globalStart = i*2;
			int blockSize;
			if (pid % 2 != 0) {
				blockSize = ceil((MAX_PRIMES - globalStart) / cores);
			} else {
				blockSize = (MAX_PRIMES - globalStart) / cores;
			}
			int myStart = globalStart + blockSize * pid;
			int myEnd = myStart + blockSize;

    		crossOutMultiples(sample, myStart, myEnd, i, pid);
			for (int j = 0; j < cores; j++) {
				//printf("mystart:%d\n", sample[myStart]);
				bsp_put(j, sample + myStart, vector, myStart, blockSize);
				bsp_sync();
				bsp_get(j, vector, 0, sample, MAX_PRIMES);
			}
			bsp_sync();
			
    	}
    	
    }

    printf("Total time: %f\n", bsp_time() - start);

    int sum = 0;

    for (int i = 0; i < MAX_PRIMES; i++) {
    	if (vector[i]) {
    		sum++;
    	}
    }
    printf("Number of primes%d prosessor %d\n", sum, pid);

	
	// Print out twin primes
	if (pid == 0) {
		for (int i = 2; i < MAX_PRIMES-2; i++) {
			if (vector[i] == 1 && vector[i + 2] == 1 ) {
				printf("%d:%d, ", i, i+2);
			}
		}
	}

	// Print out goldbach primes
	if (pid == 0) {
		struct GoldBach* bacharray = createGoldBachPairs(vector, MAX_PRIMES);
	}

    // Clean up memory	
	free(vector);
	bsp_end();

}

struct GoldBach* createGoldBachPairs(bool* primes, int upperBound) {
	
	struct GoldBach* bacharray = (struct GoldBach*)malloc(sizeof(struct GoldBach) * upperBound / 2);
	bacharray[4 / 2] = (struct GoldBach){ 2, 2 };

	for (int i = 2; i < upperBound / 2; i++) {
		if (! primes[i]) continue;
		for (int j = i; i + j < upperBound; j++) {
			if (! primes[j]) continue;
			//i and j are both primes,
			bacharray[(i + j) / 2] = (struct GoldBach) { i, j };
		}
	}
	return bacharray;
}

void printGoldBachArray(struct GoldBach* bacharray, int upperbound) {
	for (int i = 2; i < upperbound / 2; i++) {
		struct GoldBach gb = bacharray[i];
		printf("%d: [%d,%d]\n", i * 2, gb.prime1, gb.prime2);
	}
}