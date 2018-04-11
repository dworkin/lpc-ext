# include <stdlib.h>
# include <stdarg.h>
# include <stdio.h>
# include "jit.h"

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
    printf("Fatal error: %s\n", buf);

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
