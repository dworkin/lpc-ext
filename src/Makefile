#
# Makefile for extension modules
#
EXT=1.4
DEFINES=			# -DLARGENUM
DEBUG=-ggdb
CCFLAGS=$(DEFINES) $(DEBUG)
CC=cc

CFLAGS=	-fPIC -DPIC $(CCFLAGS)
LD=$(CC)
LDFLAGS=-shared $(DEBUG)

OBJ=	lpc_ext.o
LIBLIB=	kfun/rgx/libiberty
ZLIB=	1.2.11			# 1.2.8, 1.2.11, 1.2.12, 1.2.13, 1.3, 1.3.1
ZLIBDIR=$(ZLIB)			# use 1.2.12 for versions 1.2.13 to 1.3.1

all:	../lower_case.$(EXT) ../regexp.$(EXT) ../dbase.$(EXT)

jit:	../jit.$(EXT)

zlib:	../zlib.$(EXT)

crypto:	../crypto.$(EXT)

tls:	../tls.$(EXT)

lpc_ext.o:	lpc_ext.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. lpc_ext.c

kfun/lower_case.o:	kfun/lower_case.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. kfun/lower_case.c

../lower_case.$(EXT):	kfun/lower_case.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

kfun/rgx/regexp.o:	kfun/rgx/regexp.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. -I$(LIBLIB) kfun/rgx/regexp.c

kfun/rgx/regex.o:	kfun/rgx/libiberty/regex.c
	$(CC) -o $@ -c $(CFLAGS) -I$(LIBLIB) `cat $(LIBLIB)/config` $+

../regexp.$(EXT):	kfun/rgx/regexp.o kfun/rgx/regex.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

dbase/dbase.o:	dbase/dbase.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. dbase/dbase.c

../dbase.$(EXT):	dbase/dbase.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+

jit/jitcomp::
	$(MAKE) -C jit 'DEFINES=$(DEFINES)' 'DEBUG=$(DEBUG)'

jit/jit.o::
	$(MAKE) -C jit 'CFLAGS=$(CFLAGS)' jit.o

../jit.$(EXT):		jit/jit.o $(OBJ) jit/jitcomp
	$(LD) -o $@ $(LDFLAGS) jit/jit.o $(OBJ)

kfun/zlib/zlib.o:	kfun/zlib/zlib.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. -Ikfun/zlib/$(ZLIBDIR) -DVERSION=$(ZLIB) \
	      kfun/zlib/zlib.c

../zlib.$(EXT):		kfun/zlib/zlib.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+ -lz

kfun/crypto.o:	kfun/crypto.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. kfun/crypto.c

../crypto.$(EXT):	kfun/crypto.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+ -lcrypto

kfun/tls/tls.o:	kfun/tls/tls.c lpc_ext.h
	$(CC) -o $@ -c $(CFLAGS) -I. kfun/tls/tls.c

../tls.$(EXT):		kfun/tls/tls.o $(OBJ)
	$(LD) -o $@ $(LDFLAGS) $+ -lssl

clean:
	rm -f ../lower_case.$(EXT) ../regexp.$(EXT) ../zlib.$(EXT) \
	      ../jit.$(EXT) ../dbase.$(EXT) ../crypto.$(EXT) ../tls.$(EXT) \
	      *.o kfun/*.o kfun/*/*.o dbase/*.o
	$(MAKE) -C jit clean
