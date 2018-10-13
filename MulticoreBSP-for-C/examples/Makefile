
RELPATH = ../
include ../include.mk
include ../version.mk

CC=../tools/bspcc
CPP=../tools/bspcxx

TARGETS=multistart parallel_loop hello_world hello_world_cpp hierarchical

ifdef WITH_DMTCP
TARGETS:=${TARGETS} checkpointed_loop checkpointed_communication
endif

all: ../lib/libmcbsp${VERSION}.a ${CC} ${CPP} ${TARGETS}

valgrind: $(addprefix valgrind_, $(TARGETS))

valgrind_%: %
	valgrind --leak-check=full ./$^

../lib/libmcbsp${VERSION}.a:
	(cd ..; make)

../tools/bspcc:
	(cd ..; make tools)

../tools/bspcxx:
	(cd ..; make tools)

multistart: multistart.o
	${CC} ${CFLAGS} -o $@ $^

parallel_loop: parallel_loop.o
	${CC} ${CFLAGS} -o $@ $^ -lm

hello_world: hello_world.o
	${CC} ${CPPFLAGS} -o $@ $^

hello_world_cpp: hello_world_cpp.cpp.o
	${CPP} ${CPPFLAGS} -o $@ $^

hierarchical: hierarchical.o
	${CC} ${CPPFLAGS} -o $@ $^

ifdef WITH_DMTCP
checkpointed_loop: checkpointed_loop.o
	${CC} ${CFLAGS} -o $@ $^ ${LFLAGS} -lm

checkpointed_communication: checkpointed_communication.o
	${CC} ${CFLAGS} -o $@ $^ ${LFLAGS}
else
checkpointed_loop:
	@echo "Target \`$@' skipped since we are not compiling with DMTCP";

checkpointed_communication:
	@echo "Target \`$@' skipped since we are not compiling with DMTCP";
endif

clean:
	rm -f multistart.o parallel_loop.o hello_world.o hello_world_cpp.cpp.o hierarchical.o multistart parallel_loop hello_world hello_world_cpp hierarchical checkpointed_loop.o checkpointed_loop checkpointed_communication

