#include "main.h"

// If build has not defined MAX_PRIMES
//#ifndef MAX_PRIMES
#define MAX_PRIMES 100000
//#endif // !MAX_PRIMES
#define MAX_PRIMES_ROOT sqrt(MAX_PRIMES)

// If build has not defined CORES
//#ifndef CORES
#define CORES 2
//#endif
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

int numberOfSyncs[2] = {0,0};
bool done[2] = {0,0};

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
    		/*if (pid == 1 && i ==4)
    			continue;*/
			int globalStart = i*2;
			int blockSize;
			if (pid % 2 != 0) {
				blockSize = ceil((MAX_PRIMES - globalStart) / cores);
			} else {
				blockSize = (MAX_PRIMES - globalStart) / cores;
			}
			int myStart = globalStart + blockSize * pid;
			int myEnd = myStart + blockSize;
			/*while(myStart % i != 0) {
				myStart++;
			}
			if (myStart >= MAX_PRIMES|| myEnd == myStart){
				continue;
			}*/
    		crossOutMultiples(sample, myStart, myEnd, i, pid);
			for (int j = 0; j < cores; j++) {
				//printf("mystart:%d\n", sample[myStart]);
				bsp_put(j, sample + myStart, vector, myStart, blockSize);
				bsp_sync();
				bsp_get(j, vector, 0, sample, MAX_PRIMES);
			}numberOfSyncs[pid]++;
			if (i == MAX_PRIMES / 2 -1){
				done[pid] = 1;
			}
			bsp_sync();
			
    	}
    	
    }
    printf("numbers of sync %d, %d\n",numberOfSyncs[0], numberOfSyncs[1] );
    /*while(numberOfSyncs[0] == numberOfSyncs[1] && !done[1]) {
    	printf("TESSTTTT numbers of sync %d, %d\n",numberOfSyncs[0], numberOfSyncs[1] );
    	bsp_sync();
    	numberOfSyncs[pid]++;
    }*/

    //bsp_sync();

    printf("Total time: %f\n", bsp_time() - start);

    int sum = 0;

    for (int i = 0; i < MAX_PRIMES; i++) {
    	if (vector[i]) {
    		sum++;
    	}
    }
    printf("Number of primes%d prosessor %d\n", sum, pid);

	bsp_end();


	//printf("Time crossed out: %f\n", bsp_time() - start);		

	//bsp_push_reg(primes + myStart, blockSize);

	//bsp_sync();	

	/*// Send data to other processors		
	// Calculate range of blocks
	int leftBlock = rangeStart / BLOCK_SIZE;
	int rightBlock = ceil(rangeEnd / (double)BLOCK_SIZE);
	
	//Get the relevant array
	int* partial = &primesArray[leftBlock];

	for (int i = 0; i < cores; i++) {		
		bsp_put(i, partial, vector, leftBlock * sizeof(BitBlock), (rightBlock - leftBlock) * sizeof(BitBlock));				
	}
	bsp_sync();
	
	//Merge preprocessed primes into the result
	for (int i = 0; i < bitarray_blocks(MAX_PRIMES_ROOT); i++) {						
		vector[i] |= preprocess[i];
	}

	printf("Total time: %f\n", bsp_time() - start);
	int amount = countPrimes(vector, MAX_PRIMES);
	printf("Amount of primes: %d\n", amount);
	
	// Print out twin primes
	if (pid == 0) {
		for (int i = 2; i < MAX_PRIMES-2; i++) {
			if (bitarray_get(vector, i) == 0 && bitarray_get(vector, i + 2) == 0 ) {
				printf("%d:%d, ", i, i+2);
			}
		}
	}

	// Print out goldbach primes
	if (pid == 0) {
		struct GoldBach* bacharray = createGoldBachPairs(vector, MAX_PRIMES);
		printGoldBachArray(bacharray, MAX_PRIMES);
	}

	// Clean up memory	
	free(primesArray);
	free(preprocess);
	bsp_end();*/
}

struct GoldBach* createGoldBachPairs(Bitarray primes, int upperBound) {
	
	struct GoldBach* bacharray = (struct GoldBach*)malloc(sizeof(struct GoldBach) * upperBound / 2);
	bacharray[4 / 2] = (struct GoldBach){ 2, 2 };

	for (int i = 2; i < upperBound / 2; i++) {
		if (bitarray_get(primes, i)) continue;
		for (int j = i; i + j < upperBound; j++) {
			if (bitarray_get(primes, j)) continue;

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