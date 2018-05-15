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
# include "disassemble.h"
# include "jitcomp.h"

/*
 * NAME:	fatal()
 * DESCRIPTION:	fatal error
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
 * NAME:	jit->alloc()
 * DESCRIPTION:	allocate memory in JIT compiler
 */
void *jit_alloc(size_t size)
{
    void *mem;

    mem = malloc(size);
    if (mem == NULL) {
	fatal("cannot allocate memory");
    }
    return mem;
}

static struct CodeContext *cc;

static void jit_compile(int nInherits, uint8_t *prog, int nFunctions,
			uint8_t *funcTypes, uint8_t *varTypes)
{
    CodeFunction *func;

    code_object(cc, nInherits, funcTypes, varTypes);
    func = code_new(cc, prog);
    if (!(func->fclass & CLASS_UNDEFINED)) {
	while (code_instr(func) != NULL) ;
	dis_program(func);
	fprintf(stderr, "\n");
    }
    code_del(func);
}

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

int main(int argc, char *argv[])
{
    JitInfo info;
    uint8_t hash[16];
    uint16_t *map;
    uint8_t *protos;
    char reply;

    if (argc != 2 || chdir(argv[1]) < 0) {
	return 1;
    }

    /* read init params from 0 */
    if (read(0, &info, sizeof(JitInfo)) != sizeof(JitInfo)) {
	return 2;
    }
    map = alloc(uint16_t, info.mapSize);
    protos = alloc(uint8_t, info.protoSize);
    if (read(0, map, info.mapSize * sizeof(uint16_t)) !=
					    info.mapSize * sizeof(uint16_t) ||
	read(0, protos, info.protoSize) != info.protoSize) {
	return 2;
    }

    cc = code_init(info.major, info.minor, info.intSize, info.inheritSize,
		   map, info.mapSize, protos, info.nProtos);
    if (cc == NULL) {
	reply = false;
	write(1, &reply, 1);
	return 3;
    }
    reply = true;
    write(1, &reply, 1);
    free(protos);
    free(map);

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

	jit_compile(comp.nInherits, prog, comp.nFunctions, ftypes, vtypes);
    }

    return 0;
}
