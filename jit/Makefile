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

$(OBJ):		../src/lpc_ext.h data.h jitcomp.h
code.o:		instruction.h code.h
data.o:		code.h stack.h block.h
typed.o:	instruction.h code.h stack.h block.h typed.h
disasm.o:	code.h stack.h block.h typed.h disasm.h
jitcomp.o:	code.h stack.h block.h typed.h disasm.h jit.h
