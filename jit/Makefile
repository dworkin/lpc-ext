DEFINES=
DEBUG=
CXX=c++
CXXFLAGS=$(DEFINES) $(DEBUG) -I../src

OBJ=	code.o block.o typed.o flow.o disasm.o genclang.o jitcomp.o

all:	jitcomp

jitcomp: $(OBJ)
	$(CXX) $(OBJ) -o jitcomp

clean:
	rm -f $(OBJ) gentt.h jitcomp

gentt.h:
	clang -v  2>&1 | \
	sed -n 's/Target: \(.*\)/# define TARGET_TRIPLE "\1"/p' > gentt.h

$(OBJ):		../src/lpc_ext.h data.h jitcomp.h
code.o:		instruction.h code.h
block.o:	code.h stack.h block.h
typed.o:	instruction.h code.h stack.h block.h typed.h
flow.o:		code.h stack.h block.h typed.h flow.h
disasm.o:	code.h stack.h block.h typed.h flow.h disasm.h
genclang.o:	instruction.h code.h stack.h block.h typed.h flow.h gentt.h \
		genclang.h
jitcomp.o:	code.h stack.h block.h typed.h flow.h genclang.h jit.h
