DEFINES=
DEBUG=
CXX=c++
CXXFLAGS=$(DEFINES) $(DEBUG) -I../src

OBJ=	code.o block.o typed.o disasm.o jitcomp.o

all:	jitcomp

jitcomp: $(OBJ)
	$(CXX) $(OBJ) -o jitcomp

clean:
	rm -f $(OBJ) jitcomp
