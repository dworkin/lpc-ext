DEFINES=
DEBUG=
CXX=c++
CXXFLAGS=$(DEFINES) $(DEBUG) -I../src

OBJ=	code.o block.o disassemble.o jitcomp.o

all:	jitcomp

jitcomp: $(OBJ)
	$(CXX) $(OBJ) -o jitcomp

clean:
	rm -f $(OBJ) jitcomp
