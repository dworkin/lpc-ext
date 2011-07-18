HOST=	DARWIN
DEFINES=-D$(HOST)
DEBUG=	-O -g
CCFLAGS=$(DEFINES) $(DEBUG)
CFLAGS=	-I.. $(CCFLAGS)
LDFLAGS=
LIBS=
CC=	gcc
LD=	$(CC)
DMAKE=	make

SRC=	login.o relay.o
OBJ=	login.o relay.o

relay:	$(OBJ)
	$(LD) $(DEBUG) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

clean:
	rm -f relay $(OBJ)


$(OBJ): login.h
