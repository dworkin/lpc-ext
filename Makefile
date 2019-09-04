#
# Makefile for kfun shared objects
#
EXT=0.13
DEFINES=
DEBUG=-ggdb
CCFLAGS=$(DEFINES) $(DEBUG)
CC=cc

CFLAGS=	-fPIC -DPIC $(CCFLAGS)
LD=$(CC)
LDFLAGS=-shared $(DEBUG)

OBJ=	src/lpc_ext.o
LIBLIB=	kfun/rgx/libiberty

all: lower_case.$(EXT) regexp.$(EXT) jit.$(EXT)

src/lpc_ext.o:	src/lpc_ext.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc src/lpc_ext.c

kfun/lower_case.o:	kfun/lower_case.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc kfun/lower_case.c

lower_case.$(EXT):	kfun/lower_case.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

jit/jitcomp::
	make -C jit 'DEFINES=$(DEFINES)' 'DEBUG=$(DEBUG)'

jit/jit.o::
	make -C jit 'CFLAGS=$(CFLAGS)' jit.o

jit.$(EXT):	jit/jit.o $(OBJ) jit/jitcomp
	$(LD) -o $@ $(LDFLAGS) jit/jit.o $(OBJ)

kfun/rgx/regexp.o:	kfun/rgx/regexp.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc -I$(LIBLIB) kfun/rgx/regexp.c

kfun/rgx/regex.o:	kfun/rgx/libiberty/regex.c
	$(CC) -o $@ -c $(CFLAGS) -I$(LIBLIB) `cat $(LIBLIB)/config` $+

regexp.$(EXT):		kfun/rgx/regexp.o kfun/rgx/regex.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

clean:
	rm -f lower_case.$(EXT) regexp.$(EXT) jit.$(EXT) src/*.o kfun/*.o \
	      kfun/rgx/*.o
	make -C jit clean
