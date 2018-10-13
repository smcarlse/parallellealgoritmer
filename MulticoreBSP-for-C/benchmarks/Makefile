
include ../include.mk
include ../version.mk

BSPCC=../tools/bspcc
BSPCPP=../tools/bspcxx

all: ${BSPCC} ${BSPCPP} plot_results.m

nompi: machine.info put hpput get hpget bsp_nprocs stream-memcpy stream-hw-triad stream-sw-triad barrier

plot_results.m: machine.info put hpput get hpget mpiput mpiget mpicollectives bsp_nprocs stream-memcpy stream-hw-triad stream-sw-triad barrier src/plot_results.header src/plot_results.footer
	rm -f /tmp/mcbsp_bench.log
	./put   | tee -a /tmp/mcbsp_bench.log
	./hpput | tee -a /tmp/mcbsp_bench.log
	./get   | tee -a /tmp/mcbsp_bench.log
	./hpget | tee -a /tmp/mcbsp_bench.log
	mpirun -np `./bsp_nprocs` ./mpiput | tee -a /tmp/mcbsp_bench.log
	mpirun -np `./bsp_nprocs` ./mpiget | tee -a /tmp/mcbsp_bench.log
	mpirun -np `./bsp_nprocs` ./mpicollectives | tee -a /tmp/mcbsp_bench.log
	cp src/plot_results.header ./plot_results.m
	grep ms /tmp/mcbsp_bench.log | cut -d' ' -f 4 >> ./plot_results.m
	echo "];" >> ./plot_results.m
	echo " " >> ./plot_results.m
	rm -f /tmp/mcbsp_stream.log
	./stream-memcpy   | tee -a /tmp/mcbsp_stream.log
	./stream-hw-triad | tee -a /tmp/mcbsp_stream.log
	./stream-sw-triad | tee -a /tmp/mcbsp_stream.log
	echo " " >> ./plot_results.m
	echo "%results of the streams benchmark for out-of-cache data movement, values are in GB/s; these are the slowest speeds measured. Each row contains a figure for a different triad kernel." >> ./plot_results.m
	echo "streams = [" >> ./plot_results.m
	grep Slowest /tmp/mcbsp_stream.log | cut -d' ' -f12 >> ./plot_results.m
	echo "];" >> ./plot_results.m
	echo " " >> ./plot_results.m
	echo "%results of the streams benchmark for out-of-cache data movement, values are in GB/s; these are the standard deviations measured and match the values in the streams array." >> ./plot_results.m
	echo "streams_stddev = [" >> ./plot_results.m
	grep statistics /tmp/mcbsp_stream.log | cut -d' ' -f 14 >> ./plot_results.m
	echo "];" >> ./plot_results.m
	echo " " >> ./plot_results.m
	rm -f /tmp/mcbsp_barrier.log
	./barrier | tee -a /tmp/mcbsp_barrier.log
	echo "%results of the barrier benchmark, values are in ms., first is max, second is variance." >> ./plot_results.m
	echo "barrier = [" >> ./plot_results.m
	grep Time /tmp/mcbsp_barrier.log | cut -d' ' -f 15 | cut -d',' -f 1 >> ./plot_results.m
	grep Time /tmp/mcbsp_barrier.log | cut -d' ' -f 18 >> ./plot_results.m
	echo "];" >> ./plot_results.m
	echo " " >> ./plot_results.m
	cat src/plot_results.footer >> ./plot_results.m

../include.mk: ../Makefile
	(cd ..; make include.mk)

../lib/libmcbsp${VERSION}.a:
	(cd ..; make)

../include:
	(cd ..; make)

../tools/bspcc:
	(cd ..;make tools)

../tools/bspcxx:
	(cd ..;make tools)

machine.info: generate_machine_info
	./generate_machine_info
	touch machine.info

put: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -DPRIMITIVE=0 -o put src/collectives.c

hpput: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -DPRIMITIVE=1 -o hpput src/collectives.c

get: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -DPRIMITIVE=2 -o get src/collectives.c

hpget: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -DPRIMITIVE=3 -o hpget src/collectives.c

mpiput: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	mpicc -std=c99 -mtune=native -march=native ${GCCFLAGS} ${OPT} -DPRIMITIVE=10 -I../include/ -o mpiput src/collectives.c

mpiget: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	mpicc -std=c99 -mtune=native -march=native ${GCCFLAGS} ${OPT} -DPRIMITIVE=11 -I../include/ -o mpiget src/collectives.c

mpicollectives: src/collectives.c ../lib/libmcbsp${VERSION}.a ../include/
	mpicc -std=c99 -mtune=native -march=native ${GCCFLAGS} ${OPT} -DPRIMITIVE=12 -I../include/ -o mpicollectives src/collectives.c

generate_machine_info: ../lib/libmcbsp${VERSION}.a src/generate_machine_info.cpp ../include/
	${BSPCPP} -Wextra -O1 -g -o generate_machine_info src/generate_machine_info.cpp

bsp_nprocs: ../lib/libmcbsp${VERSION}.a src/bsp_nprocs.c ../include/
	${BSPCC} ${CFLAGS} -o bsp_nprocs src/bsp_nprocs.c

timerlib.o: src/timerlib.c
	${CC} ${CFLAGS} -c -o $@ $^

stream-memcpy: src/stream-memcpy.c timerlib.o ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -D_POSIX_C_SOURCE=200112L -o $@ src/stream-memcpy.c timerlib.o -lm

stream-standard-kernel.o: src/stream-standard-kernel.c
	${BSPCC} ${CFLAGS} -c -o $@ src/stream-standard-kernel.c

stream-hw-triad: src/stream-triad.c stream-standard-kernel.o timerlib.o ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -funroll-loops -D_POSIX_C_SOURCE=200112L -o $@ src/stream-triad.c stream-standard-kernel.o timerlib.o -lm

stream-sw-triad: src/stream-triad.c stream-standard-kernel.o timerlib.o ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -funroll-loops -DSW_TRIAD -D_POSIX_C_SOURCE=200112L -o $@ src/stream-triad.c stream-standard-kernel.o timerlib.o -lm

barrier: src/barrier.c timerlib.o ../lib/libmcbsp${VERSION}.a ../include/
	${BSPCC} ${CFLAGS} -o $@ src/barrier.c timerlib.o -lm

clean:
	rm -f machine.info generate_machine_info bsp_nprocs put hpput get hpget mpiput mpiget mpicollectives stream-memcpy stream-hw-triad stream-sw-triad stream-standard-kernel.o barrier timerlib.o

