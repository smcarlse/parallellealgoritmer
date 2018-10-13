
#Uncomment the below to enable checkpointing under Linux, using http://dmtcp.sourceforge.net
#WITH_DMTCP=yes

#Debug flags:
DEBUGFLAGS=-O0 -g -DMCBSP_SHOW_PINNING

#Optimisation flags:
OPT=${NOWARN} -O3 -funroll-loops -g -DNDEBUG
#OPT=${DEBUGFLAGS}

#Linkage flags for shared library build
#Linux:
SHARED_LINKER_FLAGS=-shared -Wl,-soname,libmcbsp.so.${MAJORVERSION} -o lib/libmcbsp.so.${VERSION}
#OS X:
#SHARED_LINKER_FLAGS=-Wl,-U,_main -dynamiclib -install_name "libmcbsp.${MAJORVERSION}.dylib" -current_version ${VERSION} -compatibility_version ${MAJORVERSION}.0 -o libmcbsp.${MAJORVERSION}.dylib

#add -fPIC when we compile with DMTCP
ifdef WITH_DMTCP
 OPT:=${OPT} -fPIC -DMCBSP_WITH_DMTCP
endif

#Compiler-specific flags
GCCFLAGS=-Wall -Wextra -Wimplicit-fallthrough=2
LLVMFLAGS=-Weverything -Wno-padded -Wno-missing-noreturn -Wno-cast-align -Wno-covered-switch-default -Wno-unreachable-code -Wno-format-nonliteral -Wno-float-equal -Wno-disabled-macro-expansion -Wno-reserved-id-macro
ICCFLAGS=-Wall -Wextra -diag-disable 378

#GNU Compiler Collection:
CFLAGS:=${GCCFLAGS} ${OPT} -I. -DMCBSP_USE_SYSTEM_MEMCPY
CC=gcc -pthread -std=c99 -mtune=native -march=native
CPPFLAGS:=-Weffc++ -Wall -Wextra ${OPT} -I. -DMCBSP_USE_SYSTEM_MEMCPY
CPP=g++ -pthread -std=c++98 -mtune=native -march=native
CPP11=g++ -pthread -std=c++11 -mtune=native -march=native
LFLAGS:=`${RELPATH}deplibs.sh ${CC}`
AR=ar

#clang LLVM compiler:
#CFLAGS:=${LLVMFLAGS} ${OPT} -I.
#CC=clang -std=c99
#CPPFLAGS:=${LLVMFLAGS} ${OPT} -I.
#CPP=clang++ -std=c++98
#CPP11=clang++ -std=c++11 -Wno-c++98-compat
#LFLAGS:=`${RELPATH}deplibs.sh ${CC}`
#AR=ar

#Intel C++ Compiler:
#CFLAGS:=${ICCFLAGS} ${OPT} -I.
#CC=icc -std=c99 -xHost
#CPPFLAGS:=-ansi ${ICCFLAGS} ${OPT} -I.
#CPP=icc -xHost
#CPP11=icc -std=c++11 -xHost
#LFLAGS:=`${RELPATH}deplibs.sh ${CC}`
#AR=ar

#MinGW 32-bit Windows cross-compiler:
#PTHREADS_WIN32_PATH=pthreads-w32/
#CFLAGS:=${GCCFLAGS} ${OPT} -I. -I${PTHREADS_WIN32_PATH}include
#CC=i686-w64-mingw32-gcc -ansi -std=c99 -mtune=native -march=native
#CPPFLAGS:=${GCCFLAGS} ${OPT} -I. -I${PTHREADS_WIN32_PATH}include
#CPP=i686-w64-mingw32-g++ -ansi -std=c++98 -mtune=native -march=native
#CPP11=i686-w64-mingw32-g++ -ansi -std=c++11 -mtune=native -march=native
#LFLAGS:=${RELPATH}${PTHREADS_WIN32_PATH}lib/x86/libpthreadGC2.a
#AR=i686-w64-mingw32-ar

#MinGW 64-bit Windows cross-compiler:
#NOWARN=
#PTHREADS_WIN32_PATH=pthreads-w32/
#CFLAGS:=${GCCFLAGS} ${OPT} -I. -I${PTHREADS_WIN32_PATH}include
#CC=x86_64-w64-mingw32-gcc -ansi -std=c99 -mtune=native -march=native
#CPPFLAGS:=${GCCFLAGS} ${OPT} -I. -I${PTHREADS_WIN32_PATH}include
#CPP=x86_64-w64-mingw32-g++ -ansi -std=c++98 -mtune=native -march=native
#CPP11=x86_64-w64-mingw32-g++ -ansi -std=c++11 -mtune=native -march=native
#LFLAGS:=${RELPATH}${PTHREADS_WIN32_PATH}lib/x64/libpthreadGC2.a
#AR=x86_64-w64-mingw32-ar

OBJECTS=bsp-active-hooks.o mcutil.o mcbsp.o bsp.o bsp.cpp.o bsp.debug.o bsp.profile.o
TESTOBJ=tests/internal.o tests/util.o tests/bsp.o tests/abort.o tests/spmd.o tests/hp.o
CLEAN_OBJECTS=${OBJECTS} ${OBJECTS:%.o=%.shared.o} ${KROBJECTS} ${TESTOBJ}
CLEAN_EXECS=

#To add specific hardware support, simply include the auxiliary file here.
#
#For example, if our machine has the accelerator `foo' installed, and a
#plugin for the foo accelerator is distributed with MulticoreBSP in the
#base directory ./accelerators/foo/*, then add the following line:
#
#include ${RELPATH}accelerators/foo/include.mk

%.shared.o: %.c %.h
	${CC} -fPIC ${CFLAGS} -c -o $@ $(^:%.h=)

%.shared.o: %.c
	${CC} -fPIC ${CFLAGS} -c -o $@ $^

%.o: %.c %.h
	${CC} ${CFLAGS} -c -o $@ $(^:%.h=)

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $^

%.cpp.o: %.cpp %.hpp
	${CPP} ${CPPFLAGS} -c -o $@ $(^:%.hpp=)

%.cpp.o: %.cpp
	${CPP} ${CPPFLAGS} -c -o $@ $^

%.cpp.shared.o: %.cpp %.hpp
	${CPP} -fPIC ${CPPFLAGS} -c -o $@ $(^:%.hpp=)

%.cpp.shared.o: %.cpp
	${CPP} -fPIC ${CPPFLAGS} -c -o $@ $^

%.cpp11.o: %.cpp %.hpp
	${CPP11} ${CPPFLAGS} -c -o $@ $(^:%.hpp=)

%.cpp11.o: %.cpp
	${CPP11} ${CPPFLAGS} -c -o $@ $^

%.cpp11.shared.o: %.cpp %.hpp
	${CPP11} -fPIC ${CPPFLAGS} -c -o $@ $(^:%.hpp=)

%.cpp11.shared.o: %.cpp
	${CPP11} -fPIC ${CPPFLAGS} -c -o $@ $^

