#
# Makefile for kfun shared objects
#
EXT=1.2
DEFINES=
DEBUG=-ggdb
CCFLAGS=$(DEFINES) $(DEBUG)
CC=cc

CFLAGS=	-fPIC -DPIC $(CCFLAGS)
LD=$(CC)
LDFLAGS=-shared $(DEBUG)

OBJ=	src/lpc_ext.o
LIBLIB=	kfun/rgx/libiberty
ZLIB=	1.2.11

all:	lower_case.$(EXT) regexp.$(EXT)

jit:	jit.$(EXT)

zlib:	zlib.$(EXT)

crypto:	crypto.$(EXT)

src/lpc_ext.o:	src/lpc_ext.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc src/lpc_ext.c

kfun/lower_case.o:	kfun/lower_case.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc kfun/lower_case.c

lower_case.$(EXT):	kfun/lower_case.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

kfun/rgx/regexp.o:	kfun/rgx/regexp.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc -I$(LIBLIB) kfun/rgx/regexp.c

kfun/rgx/regex.o:	kfun/rgx/libiberty/regex.c
	$(CC) -o $@ -c $(CFLAGS) -I$(LIBLIB) `cat $(LIBLIB)/config` $+

regexp.$(EXT):		kfun/rgx/regexp.o kfun/rgx/regex.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

jit/jitcomp::
	$(MAKE) -C jit 'DEFINES=$(DEFINES)' 'DEBUG=$(DEBUG)'

jit/jit.o::
	$(MAKE) -C jit 'CFLAGS=$(CFLAGS)' jit.o

jit.$(EXT):	jit/jit.o $(OBJ) jit/jitcomp
	$(LD) -o $@ $(LDFLAGS) jit/jit.o $(OBJ)

kfun/zlib/zlib.o:	kfun/zlib/zlib.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc -Ikfun/zlib/$(ZLIB) -DVERSION=$(ZLIB) \
	      kfun/zlib/zlib.c

zlib.$(EXT):	kfun/zlib/zlib.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+ -lz

kfun/crypto.o:	kfun/crypto.c src/lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -Isrc kfun/crypto.c

crypto.$(EXT):	kfun/crypto.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+ -lcrypto

clean:
	rm -f lower_case.$(EXT) regexp.$(EXT) zlib.$(EXT) jit.$(EXT) \
	      crypto.$(EXT) src/*.o kfun/*.o kfun/rgx/*.o kfun/zlib/*.o \
	      kfun/crypto/*.o
	$(MAKE) -C jit clean
