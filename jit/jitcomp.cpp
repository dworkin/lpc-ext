# ifndef WIN32
# include <unistd.h>
# else
# include <Windows.h>
# include <io.h>
# include <direct.h>
# endif
# include <stdlib.h>
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

# ifndef WIN32
# define O_BINARY		0
# else
# define open			_open
# define close			_close
# define read			_read
# define write			_write
# define mkdir(path, mode)	_mkdir(path)
# define chdir			_chdir
# define dup			_dup
# define dup2			_dup2
# endif

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
static bool jitComp(CodeObject *object, uint8_t *prog, int nFunctions,
		    char *base)
{
# ifdef DISASM
    Code::producer(&DisCode::create);
    Block::producer(&DisBlock::create);

    for (int i = 0; i < nFunctions; i++) {
	CodeFunction func(object, prog);
	DisFunction::emit(stderr, &func);
	prog = func.endProg();
	fprintf(stderr, "\n");
    }
    return false;
# endif

# ifdef GENCLANG
    Code::producer(&ClangCode::create);
    Block::producer(&ClangBlock::create);

    ClangObject clang(object, prog, nFunctions);
    return clang.emit(base);
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
    uint8_t cmdhash[17];
    char reply;
    int out;
    uint8_t protos[65536];
    CodeContext *cc;

    if (argc != 2 || chdir(argv[1]) < 0) {
	return 1;
    }
    mkdir("cache", 0750);

    /* read init params from 0 */
    if (read(0, &info, sizeof(JitInfo)) != sizeof(JitInfo)) {
	return 2;
    }
    if (read(0, protos, info.protoSize) != info.protoSize) {
	return 2;
    }

    /* force output to stderr */
    out = dup(1);
    dup2(2, 1);

    if (!CodeContext::validVM(info.major, info.minor)) {
	reply = false;
	write(out, &reply, 1);
	return 3;
    }

    cc = new CodeContext(info.intSize, info.inheritSize, protos, info.nBuiltins,
			 info.nKfuns, info.typechecking);
    reply = true;
    write(out, &reply, 1);

    cmdhash[0] = '\0';
    while (read(0, cmdhash + 1, 16) == 16) {
	char path[1000];
	JitCompile comp;
	int fd;
	uint8_t *prog, *ftypes, *vtypes;

	filename(path, cmdhash + 1);
	fd = open(path, O_RDONLY | O_BINARY);
	read(fd, &comp, sizeof(JitCompile));
	prog = new uint8_t[comp.progSize];
	ftypes = new uint8_t[comp.fTypeSize];
	vtypes = new uint8_t[comp.vTypeSize];
	read(fd, prog, comp.progSize);
	read(fd, ftypes, comp.fTypeSize);
	read(fd, vtypes, comp.vTypeSize);
	close(fd);

	CodeObject object(cc, comp.nInherits, ftypes, vtypes);
	if (jitComp(&object, prog, comp.nFunctions, path)) {
	    write(out, cmdhash, 17);
	}

	delete[] vtypes;
	delete[] ftypes;
	delete[] prog;
    }

    return 0;
}
