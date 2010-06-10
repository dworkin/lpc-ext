#
# Makefile for kfun shared objects
#
HOST=GNU
EXT=0.5
DEFINES=
DEBUG=
CCFLAGS=$(DEFINES) $(DEBUG) -Isrc
CC=cc
LD=ld

ifeq ($(HOST),DARWIN)
# OS X
CFLAGS=	-DPIC $(CCFLAGS)
LDFLAGS=-bundle -flat_namespace -undefined suppress
else
# generic gcc
CFLAGS=	-fPIC -DPIC $(CCFLAGS)
LDFLAGS=-shared
endif

OBJ=	src/lpc_ext.o
LIBLIB=	kfun/rgx/libiberty

all: lower_case.$(EXT) regexp.$(EXT)

src/lpc_ext.o:	src/lpc_ext.c
	$(CC) -c $(CFLAGS) -o $@ $+

kfun/lower_case.o:	kfun/lower_case.c
	$(CC) -c $(CFLAGS) -o $@ $+

lower_case.$(EXT):	kfun/lower_case.o $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $+

kfun/rgx/regexp.o:	kfun/rgx/regexp.c
	$(CC) -c $(CFLAGS) -I$(LIBLIB) -o $@ $+

kfun/rgx/regex.o:	kfun/rgx/libiberty/regex.c
	$(CC) -c $(CFLAGS) -I$(LIBLIB) `cat $(LIBLIB)/config` -o $@ $+

regexp.$(EXT):		kfun/rgx/regexp.o kfun/rgx/regex.o $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $+
