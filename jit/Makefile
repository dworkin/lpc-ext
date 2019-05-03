DEFINES=
DEBUG=
CXX=c++
CXXFLAGS=$(DEFINES) $(DEBUG) -I../src

OBJ=	code.o block.o typed.o flow.o disasm.o jitcomp.o

all:	jitcomp

jitcomp: $(OBJ)
	$(CXX) $(OBJ) -o jitcomp

clean:
	rm -f $(OBJ) jitcomp

$(OBJ):		../src/lpc_ext.h data.h jitcomp.h
code.o:		instruction.h code.h
block.o:	code.h stack.h block.h
typed.o:	instruction.h code.h stack.h block.h typed.h
flow.o:		code.h stack.h block.h typed.h flow.h
disasm.o:	code.h stack.h block.h typed.h flow.h disasm.h
jitcomp.o:	code.h stack.h block.h typed.h flow.h disasm.h jit.h
