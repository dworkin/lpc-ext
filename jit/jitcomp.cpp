# include <stdlib.h>
# include <unistd.h>
# include <stdint.h>
# include <stdarg.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <string.h>
# include <stdio.h>
extern "C" {
# include "lpc_ext.h"
# include "jit.h"
}
# include "data.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "flow.h"
# ifdef DISASM
# include "disasm.h"
# endif
# ifdef GENCLANG
# include "genclang.h"
# endif
# include "jitcomp.h"

/*
 * fatal error
 */
void fatal(const char *format, ...)
{
    va_list argp;
    char buf[1024];

    va_start(argp, format);
    vsprintf(buf, format, argp);
    va_end(argp);
    fprintf(stderr, "Fatal error: %s\n", buf);

    abort();
}

/*
 * JIT compile a single object using a particular code generator
 */
static void jitComp(CodeObject *object, uint8_t *prog, int nFunctions,
		    char *base)
{
# ifdef DISASM
    Code::producer(&DisCode::create);
    Block::producer(&DisBlock::create);

    for (int i = 0; i < nFunctions; i++) {
	CodeFunction func(object, prog);
	CodeSize size;
	Block *b = Block::function(&func);

	if (b != NULL) {
	    size = b->fragment();
	    if (size != 0) {
		b->emit(stderr, &func, size);
	    }
	    b->clear();
	}
	prog = func.endProg();
	fprintf(stderr, "\n");
    }
# endif
# ifdef GENCLANG
    Code::producer(&ClangCode::create);
    Block::producer(&ClangBlock::create);

    ClangObject clang(object, prog, nFunctions);
    clang.emit(base);
# endif
}

/*
 * hash to filename
 */
static void filename(char *buffer, uint8_t *hash)
{
    static const char hex[] = "0123456789abcdef";
    int i;

    sprintf(buffer, "cache/%c%c/", hex[hash[0] >> 4], hex[hash[0] & 0xf]);
    buffer += 9;
    for (i = 0; i < 16; i++) {
	*buffer++ = hex[hash[i]>> 4];
	*buffer++ = hex[hash[i] & 0xf];
    }
    *buffer = '\0';
}

/*
 * main function
 */
int main(int argc, char *argv[])
{
    JitInfo info;
    uint8_t hash[16];
    char reply;
    CodeContext *cc;

    if (argc != 2 || chdir(argv[1]) < 0) {
	return 1;
    }

    /* read init params from 0 */
    if (read(0, &info, sizeof(JitInfo)) != sizeof(JitInfo)) {
	return 2;
    }
    uint8_t protos[info.protoSize];
    if (read(0, protos, info.protoSize) != info.protoSize) {
	return 2;
    }

    if (!CodeContext::validVM(info.major, info.minor)) {
	reply = false;
	write(1, &reply, 1);
	return 3;
    }

    cc = new CodeContext(info.intSize, info.inheritSize, protos, info.nBuiltins,
			 info.nKfuns);
    reply = true;
    write(1, &reply, 1);

    while (read(0, hash, 16) == 16) {
	char path[1000];
	JitCompile comp;
	int fd;

	filename(path, hash);
	fd = open(path, O_RDONLY);
	read(fd, &comp, sizeof(JitCompile));
	uint8_t prog[comp.progSize];
	read(fd, prog, comp.progSize);
	uint8_t ftypes[comp.fTypeSize];
	read(fd, ftypes, comp.fTypeSize);
	uint8_t vtypes[comp.vTypeSize];
	read(fd, vtypes, comp.vTypeSize);
	close(fd);

	CodeObject object(cc, comp.nInherits, ftypes, vtypes);
	jitComp(&object, prog, comp.nFunctions, path);
    }

    return 0;
}
