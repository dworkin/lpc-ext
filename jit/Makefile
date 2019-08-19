#DISASM=1
GENCLANG=1

DEFINES=
DEBUG=
ifdef DISASM
  CODEGEN=-DDISASM
endif
ifdef GENCLANG
  CODEGEN=-DGENCLANG
endif
CXX=c++
CXXFLAGS=$(DEFINES) $(DEBUG) $(CODEGEN) -I../src

OBJ=	code.o block.o typed.o flow.o jitcomp.o
ifdef DISASM
  OBJ+=disasm.o
endif
ifdef GENCLANG
  OBJ+=genclang.o
endif

all:	jitcomp

jitcomp: $(OBJ)
	$(CXX) $(OBJ) -o jitcomp

jit.o:	jit.h ../src/lpc_ext.h
	$(CC) -c $(CFLAGS) $(CODEGEN) -I../src jit.c

clean:
	rm -rf jit.o $(OBJ) gentt.h jitcomp cache

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
jitcomp.o:	code.h stack.h block.h typed.h flow.h jit.h
ifdef DISASM
jitcomp.o:	disasm.h
endif
ifdef GENCLANG
jitcomp.o:	genclang.h
endif
