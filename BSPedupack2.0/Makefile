CC= gcc
CFLAGS= -std=c99 -Wall -O3

#LFLAGS= libmcbsp2.0.3.a -pthread -lrt -lm
#For MacOS use instead:
LFLAGS= libmcbsp2.0.3.a -lm

OBJIP= bspinprod.o bspedupack.o
OBJBEN= bspbench.o bspedupack.o
OBJSORT= bspsort_test.o bspsort.o bspedupack.o
OBJSEQSORT= bspsort_test.o bspsort_seq.o bspedupack.o
OBJLU= bsplu_test.o bsplu.o bspedupack.o
OBJFFT= bspfft_test.o bspfft.o bspedupack.o
OBJMV= bspmv_test.o bspmv.o bspedupack.o

all: inprod bench sort seqsort lu fft matvec

inprod: $(OBJIP)
	$(CC) $(CFLAGS) -o inprod $(OBJIP) $(LFLAGS)

bench: $(OBJBEN)
	$(CC) $(CFLAGS) -o bench $(OBJBEN) $(LFLAGS)

sort: $(OBJSORT)
	$(CC) $(CFLAGS) -o sort $(OBJSORT) $(LFLAGS)

seqsort: $(OBJSEQSORT)
	$(CC) $(CFLAGS) -o seqsort $(OBJSEQSORT) $(LFLAGS)

lu: $(OBJLU)
	$(CC) $(CFLAGS) -o lu $(OBJLU) $(LFLAGS)

fft: $(OBJFFT)
	$(CC) $(CFLAGS) -o fft $(OBJFFT) $(LFLAGS)

matvec: $(OBJMV)
	$(CC) $(CFLAGS) -o matvec $(OBJMV) $(LFLAGS)

bspinprod.o:  bspedupack.h
bspbench.o:   bspedupack.h
bspsort.o:    bspedupack.h
bspsort_test.o:    bspedupack.h
bspsort_seq.o:    bspedupack.h
bsplu.o:    bspedupack.h
bsplu_test.o:    bspedupack.h
bspfft.o:    bspedupack.h
bspfft_test.o:    bspedupack.h
bspmv.o:    bspedupack.h
bspmv_test.o:    bspedupack.h
bspedupack.o: bspedupack.h

.PHONY: clean
clean:
	rm -f $(OBJIP) $(OBJBEN) $(OBJSORT) $(OBJSEQSORT) $(OBJLU) $(OBJFFT) inprod  bench sort seqsort lu fft
