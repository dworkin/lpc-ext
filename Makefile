#
# Makefile for kfun shared objects
#
HOST=GNU
EXT=0.9
DEFINES=
DEBUG=
CCFLAGS=$(DEFINES) $(DEBUG) -Isrc
CC=cc

CFLAGS=	-fPIC -DPIC $(CCFLAGS)
LD=$(CC)
LDFLAGS=-shared

OBJ=	src/lpc_ext.o
LIBLIB=	kfun/rgx/libiberty

all: lower_case.$(EXT) regexp.$(EXT)

src/lpc_ext.o:	src/lpc_ext.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) src/lpc_ext.c

kfun/lower_case.o:	kfun/lower_case.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) kfun/lower_case.c

lower_case.$(EXT):	kfun/lower_case.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

kfun/rgx/regexp.o:	kfun/rgx/regexp.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I$(LIBLIB) kfun/rgx/regexp.c

kfun/rgx/regex.o:	kfun/rgx/libiberty/regex.c
	$(CC) -o $@ -c $(CFLAGS) -I$(LIBLIB) `cat $(LIBLIB)/config` $+

regexp.$(EXT):		kfun/rgx/regexp.o kfun/rgx/regex.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

clean:
	rm -f lower_case.$(EXT) regexp.$(EXT) src/*.o kfun/*.o kfun/rgx/*.o
