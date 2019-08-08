# include <stdlib.h>
# include <unistd.h>
# include <stdint.h>
# include <stdarg.h>
# include <stdbool.h>
# include <pthread.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <dlfcn.h>
# ifdef SOLARIS
# include <link.h>
# endif
# include <stdio.h>
# include "lpc_ext.h"
# include "jit.h"

static char configDir[1000];

/*
 * NAME:	JIT->init()
 * DESCRIPTION:	initialize JIT compiler interface
 */
static int jit_init(int major, int minor, size_t intSize, size_t inheritSize,
		    int nBuiltins, int nKfuns, uint8_t *protos,
		    size_t protoSize, void **vmtab)
{
    char path[1000];
    JitInfo info;
    bool result;
    void *h;
    void (*init)(void**);

    /*
     * pass information to the JIT compiler backend
     */
    info.major = major;
    info.minor = minor;
    info.intSize = intSize;
    info.inheritSize = inheritSize;
    info.nBuiltins = nBuiltins;
    info.nKfuns = nKfuns;
    info.protoSize = protoSize;

    if (lpc_ext_write(&info, sizeof(JitInfo)) != sizeof(JitInfo) ||
	lpc_ext_write(protos, protoSize) != protoSize ||
	lpc_ext_read(&result, 1) != 1 ||
	result != true) {
	return false;
    }

    /*
     * dynamically load vm.so
     */
    sprintf(path, "%s/cache/vm.so", configDir);
    h = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (h == (void *) NULL) {
	return false;
    }
    init = (void *) dlsym(h, "init");
    if (init == NULL) {
	dlclose(h);
	return false;
    }

    /*
     * initialize vm object with function table
     */
    (*init)(vmtab);
    return true;
}

/*
 * NAME:	JIT->finish()
 * DESCRIPTION:	finish JIT compiler interface
 */
static void jit_finish(void)
{
}


typedef struct {
    uint64_t index;	/* object index */
    uint64_t instance;	/* object instance */
    bool compiled;	/* compiled */
    char hash[16];	/* hash */
} CacheEntry;

# define NOBJECTS	800

static CacheEntry objects[NOBJECTS];
static pthread_mutex_t lock;

/*
 * NAME:	md5hash()
 * DESCRIPTION:	compute MD5 hash
 */
static void md5hash(uint8_t *hash, unsigned char *buffer, size_t size)
{
    uint32_t digest[4];
    unsigned char tmp[64];
    size_t sz;

    /*
     * compute MD5 hash
     */
    (*lpc_md5_start)(digest);
    for (sz = size; sz >= 64; buffer += 64, sz -= 64) {
	(*lpc_md5_block)(digest, buffer);
    }
    memcpy(tmp, buffer, sz);
    (*lpc_md5_end)(hash, digest, tmp, sz, size);
}

/*
 * NAME:	filename()
 * DESCRIPTION:	hash to filename
 */
static void filename(char *buffer, uint8_t *hash)
{
    static const char hex[] = "0123456789abcdef";
    int i;

    for (i = 0; i < 16; i++) {
	*buffer++ = hex[hash[i] >> 4];
	*buffer++ = hex[hash[i] & 0xf];
    }
    *buffer = '\0';
}

/*
 * NAME:	JIT->compile()
 * DESCRIPTION:	JIT compile an object
 */
static void jit_compile(uint64_t index, uint64_t instance, int nInherits,
			uint8_t *prog, size_t progSize, int nFunctions,
			uint8_t *funcTypes, size_t fTypeSize, uint8_t *varTypes,
			size_t vTypeSize)
{
    size_t size;
    JitCompile *comp;
    unsigned char *p;
    uint8_t hash[16];
    char file[33], path[1000];
    int fd;

    /*
     * fill buffer with data for compiler backend
     */
    size = sizeof(JitCompile) + progSize + fTypeSize + vTypeSize;
    unsigned char buffer[size];
    p = buffer;
    comp = (JitCompile *) p;
    memset(comp, '\0', sizeof(JitCompile));
    comp->nInherits = nInherits;
    comp->nFunctions = nFunctions;
    comp->progSize = progSize;
    comp->fTypeSize = fTypeSize;
    comp->vTypeSize = vTypeSize;
    p += sizeof(JitCompile);
    memcpy(p, prog, progSize);
    p += progSize;
    memcpy(p, funcTypes, fTypeSize);
    p += fTypeSize;
    memcpy(p, varTypes, vTypeSize);

    /*
     * compute MD5 hash
     */
    md5hash(hash, buffer, size);

    /*
     * write to file
     */
    filename(file, hash);
    fprintf(stderr, "%lld: %s\n", (long long) index, file);
    sprintf(path, "%s/cache/%c%c", configDir, file[0], file[1]);
    mkdir(path, 0750);
    sprintf(path + strlen(path), "/%s", file);
    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0640);
    write(fd, buffer, size);
    close(fd);

    /*
     * inform backend
     */
    lpc_ext_write(hash, 16);
}

/*
 * NAME:	JIT->execute()
 * DESCRIPTION:	attempt to execute a function
 */
static int jit_execute(uint64_t index, uint64_t instance, int version, int func)
{
    int retval;

    if (index >= NOBJECTS) {
	return 0;
    }

    pthread_mutex_lock(&lock); {
	if (objects[index].instance != instance) {
	    if (objects[index].instance == 0) {
		objects[index].instance = instance;
		objects[index].compiled = false;
		retval = -1;	/* compile */
	    } else {
		retval = 0;
	    }
	} else {
	    retval = 0;
	}
    } pthread_mutex_unlock(&lock);

    return retval;
}

static void jit_release(uint64_t index, uint64_t instance)
{
}

/*
 * NAME:	LPC->ext_init()
 * DESCRIPTION:	initialize JIT compiler frontend
 */
int lpc_ext_init(int major, int minor, const char *config)
{
    char jitcomp[2000];

    pthread_mutex_init(&lock, NULL);

    strcpy(configDir, config);
    sprintf(jitcomp, "exec %s/jitcomp %s", config, config);
    lpc_ext_spawn(jitcomp);
    (*lpc_ext_jit)(&jit_init, &jit_finish, &jit_compile, &jit_execute,
		   &jit_release);

    return 1;
}
