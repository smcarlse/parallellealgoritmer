RELPATH=./

include include.mk
include version.mk

ifeq ($(wildcard machine.info),)
	COPY_MACHINFO = true
else
	COPY_MACHINFO = false
endif

all: library shared-library tools

library: ${OBJECTS}
	mkdir include || true
	mkdir lib || true
	cp bsp.h include/
	cp bsp.hpp include/
	cp mcbsp-affinity.h include/
	cp mcbsp-profiling.h include/
	cp mcbsp-resiliency.h include/
	cp mcbsp-templates.hpp include/
	${AR} rcs lib/libmcbsp${VERSION}.a ${OBJECTS}

shared-library: ${OBJECTS:%.o=%.shared.o}
	${CPP} ${SHARED_LINKER_FLAGS} ${OBJECTS:%.o=%.shared.o} ${LFLAGS}

help:
	@echo " "
	@echo "MulticoreBSP for C make targets:"
	@echo "================================"
	@echo " "
	@echo "Default targets: library, shared-library, tools"
	@echo " "
	@echo "Targets:"
	@echo "-------------"
	@echo "       library: builds libmcbsp, statically linked version"
	@echo "shared-library: builds libmcbsp, dynamically linked version"
	@echo "         tests: builds and starts testing applications"
	@echo "         tools: builds the MulticoreBSP default tools (bspcc and bsprun)"
	@echo "    include.mk: initialises the build environment. See that file for detailed build options."
	@echo "  machine.info: generates a recommended machine.info configuration file, using user input."
	@echo "         clean: removes all files created by compilation."
	@echo " "
	@echo "Note that this help listing does not display any additional targets some of the build options might enable."
	@echo " "

bsp.o: bsp.c bsp.h mcbsp.h
	${CC} ${CFLAGS} -c -o $@ $(^:%.h=)

bsp.debug.o: bsp.c bsp.h mcbsp.h
	${CC} ${DEBUGFLAGS} ${CFLAGS} -DMCBSP_USE_MUTEXES -DMCBSP_MODE=2 -c -o $@ bsp.c

bsp.profile.o: bsp.c bsp.h mcbsp.h
	${CC} ${DEBUGFLAGS} ${CFLAGS} -DMCBSP_MODE=3 -c -o $@ bsp.c

bsp.shared.o: bsp.c bsp.h mcbsp.h
	${CC} -fPIC ${CFLAGS} -c -o $@ $(^:%.h=)

bsp.debug.shared.o: bsp.c bsp.h mcbsp.h
	${CC} -fPIC ${DEBUGFLAGS} ${CFLAGS} -DMCBSP_USE_MUTEXES -DMCBSP_MODE=2 -c -o $@ bsp.c

bsp.profile.shared.o: bsp.c bsp.h mcbsp.h
	${CC} -fPIC ${DEBUGFLAGS} ${CFLAGS} -DMCBSP_MODE=3 -c -o $@ bsp.c

CLEAN_EXECS+=test_internal test_util test_bsp test_abort tests/test_abort.out test_spmd test_hp

tests: mcbsp.o mcutil.o bsp.o ${TESTOBJ}
	${CC} ${CFLAGS} -o test_internal tests/internal.o mcbsp.o mcutil.o bsp.o ${LFLAGS}
	${CC} ${CFLAGS} -o test_util     tests/util.o     mcbsp.o mcutil.o bsp.o ${LFLAGS}
	${CC} ${CFLAGS} -o test_bsp      tests/bsp.o      mcbsp.o mcutil.o bsp.o ${LFLAGS}
	${CC} ${CFLAGS} -o test_abort	 tests/abort.o	  mcbsp.o mcutil.o bsp.o ${LFLAGS}
	${CC} ${CFLAGS} -o test_spmd     tests/spmd.o     mcbsp.o mcutil.o bsp.o ${LFLAGS}
	${CC} ${CFLAGS} -o test_hp       tests/hp.o       mcbsp.o mcutil.o bsp.o ${LFLAGS}
	rm -f tests/internal.o tests/util.o tests/bsp.o tests/abort.o tests/test_abort.out tests/spmd.o tests/hp.o
	if ${COPY_MACHINFO}; then cp tests/machine.info ./; fi
	./test_internal
	./test_util > /dev/null; if [ "$$?" -ne "0" ]; then echo "SUCCESS"; else echo "FAILURE"; fi
	./test_bsp
	./test_abort 2> tests/test_abort.out; diff tests/test_abort.out tests/test_abort.chk; if [ $$? -eq 0 ]; then echo "SUCCESS"; else echo "FAILURE"; fi
	./test_spmd
	./test_hp
	if ${COPY_MACHINFO}; then rm -f machine.info; fi

include.mk: include.default
	cp include.default include.mk

tools: tools/bspcc tools/bspcxx tools/bsprun

tools/bspcc: bspcc.in deplibs.sh library
	echo "#!/bin/bash" > $@
	echo "MCBSP_PATH=\"`pwd`/\"" >> $@
	echo "MCBSP_LFLAGS=\"\$${MCBSP_PATH}lib/libmcbsp${VERSION}.a `./deplibs.sh ${CC}`\"" >> $@
	echo "BSP_LANG=\"c99\"" >> $@
	echo "PREF_CC=\"${CC}\"" >> $@
	echo "PREF_CXX=\"${CPP}\"" >> $@
	cat bspcc.in >> $@
	chmod +x $@

tools/bspcxx: deplibs.sh library
	echo "#!/bin/bash" > $@
	echo "MCBSP_PATH=\"`pwd`/\"" >> $@
	echo "MCBSP_LFLAGS=\"\$${MCBSP_PATH}lib/libmcbsp${VERSION}.a `./deplibs.sh ${CPP}`\"" >> $@
	echo "BSP_LANG=\"c++98\"" >> $@
	echo "PREF_CC=\"${CC}\"" >> $@
	echo "PREF_CXX=\"${CPP}\"" >> $@
	cat bspcc.in >> $@
	chmod +x $@

machine.info: | machine.info benchmarks
	(cd benchmarks; make generate_machine_info)
	benchmarks/generate_machine_info; \
	touch machine.info

bsp-active-hooks.c: mcbsp-default-hooks.c
	cp $^ $@

bsp-active-hooks.h: mcbsp-default-hooks.h
	cp $^ $@

bsp-active-hooks.o: bsp-active-hooks.c bsp-active-hooks.h

#To add specific hardware support, simply include the auxiliary targets
#makefile here.
#
#For example, if our machine has the accelerator `foo' installed, and a
#plugin for the foo accelerator is distributed with MulticoreBSP in the
#base directory ./accelerators/foo/*, then add the following line:
#
#include ${RELPATH}accelerators/foo/targets.mk

clean:
	rm -f ${CLEAN_OBJECTS}
	rm -f tests/test_abort.out
	rm -f ${CLEAN_EXECS}
	rm -rf include
	rm -rf lib
	rm -f tools/bspcc tools/bspcxx
	rm -f bsp-active-hooks.?

