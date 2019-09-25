# ifndef WIN32
# include <stdlib.h>
# include <unistd.h>
# include <pthread.h>
# include <string.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <dlfcn.h>
# else
# include <Windows.h>
# include <process.h>
# include <io.h>
# include <direct.h>
# endif
# include <stdint.h>
# include <stdarg.h>
# include <stdbool.h>
# ifdef SOLARIS
# include <link.h>
# endif
# include <fcntl.h>
# include <stdio.h>
# include "lpc_ext.h"
# include "jit.h"


typedef struct Program {
    uint8_t hash[16];		/* program hash */
    void *handle;		/* dll handle */
    LPC_function *functions;	/* function table */
    uint64_t refCount;		/* reference count */
    struct Program *next;	/* next in linked list */
} Program;

typedef struct Object {
    uint64_t index;		/* object index */
    uint64_t instance;		/* object instance */
    Program *program;		/* program */
    struct Object *next;	/* next in linked list */
} Object;

# define NOBJECTS	1024

static Program *programs[NOBJECTS];
static Object *objects[NOBJECTS];

/*
 * NAME:	Program->find()
 * DESCRIPTION:	find entry by hash
 */
static Program **p_find(uint8_t *hash)
{
    Program **r;

    for (r = &programs[*(uint64_t *) hash % NOBJECTS]; *r != NULL;
	 r = &(*r)->next) {
	if (memcmp((*r)->hash, hash, 16) == 0) {
	    break;
	}
    }

    return r;
}

/*
 * NAME:	Program->compile()
 * DESCRIPTION:	link cache entry to hash
 */
static Program *p_new(uint8_t *hash)
{
    Program **r, *p;

    r = p_find(hash);
    p = *r;
    if (p == NULL) {
	p = *r = malloc(sizeof(Program));
	memcpy(p->hash, hash, 16);
	p->handle = NULL;
	p->functions = NULL;
	p->refCount = 0;
	p->next = NULL;
    }
    p->refCount++;

    return p;
}

/*
 * NAME:	Program->del()
 * DESCRIPTION:	remove a program
 */
static void *p_del(Program *p)
{
    void *handle;

    if (--(p->refCount) == 0) {
	handle = p->handle;
	*p_find(p->hash) = p->next;
	free(p);
	return handle;
    }

    return NULL;
}

/*
 * NAME:	Object->find()
 * DESCRIPTION:	find object
 */
static Object **o_find(uint64_t index, uint64_t instance)
{
    Object **r;

    for (r = &objects[index % NOBJECTS]; *r != NULL; r = &(*r)->next) {
	if ((*r)->index == index && (*r)->instance == instance) {
	    break;
	}
    }

    return r;
}

/*
 * NAME:	Object->new()
 * DESCRIPTION:	create a new cache entry
 */
static Object *o_new(Object **r, uint64_t index, uint64_t instance)
{
    Object *o;

    o = *r = malloc(sizeof(Object));
    o->index = index;
    o->instance = instance;
    o->program = NULL;
    o->next = NULL;

    return o;
}

/*
 * NAME:	Object->del()
 * DESCRIPTION:	remove a cache entry
 */
static void *o_del(Object **r)
{
    Object *o;
    void *handle;

    o = *r;
    handle = (o->program != NULL) ? p_del(o->program) : NULL;
    *r = o->next;
    free(o);

    return handle;
}


static char configDir[1000];
static int typechecking;
static void **vm;

# ifndef WIN32
# define THREAD_START(func)	pthread_create(&tid, NULL, func, NULL)
# define THREAD_STOP()		pthread_join(tid, NULL)
# define MUTEX_INIT(lock)	pthread_mutex_init(lock, NULL)
# define MUTEX_DESTROY(lock)	pthread_mutex_destroy(lock)
# define MUTEX_LOCK(lock)	pthread_mutex_lock(lock)
# define MUTEX_UNLOCK(lock)	pthread_mutex_unlock(lock)
# define DLL_OPEN(mod)		dlopen(mod, RTLD_NOW | RTLD_LOCAL)
# define DLL_CLOSE(handle)	dlclose(handle)
# define DLL_SYM(handle, sym)	dlsym(handle, sym)
# define O_BINARY		0
# define DLL_EXT		".so"

typedef void* Handle;

static pthread_mutex_t lock;
static pthread_t tid;
# else
# define THREAD_START(func)	_beginthread((void (*)(void*)) func, 0, NULL)
# define THREAD_STOP()		/* */
# define MUTEX_INIT(lock)	InitializeCriticalSection(lock)
# define MUTEX_DESTROY(lock)	DeleteCriticalSection(lock)
# define MUTEX_LOCK(lock)	EnterCriticalSection(lock)
# define MUTEX_UNLOCK(lock)	LeaveCriticalSection(lock)
# define DLL_OPEN(mod)		LoadLibrary((LPCSTR) mod)
# define DLL_CLOSE(handle)	FreeLibrary(handle)
# define DLL_SYM(handle, sym)	GetProcAddress(handle, (LPCSTR) sym)
# define alloca			_alloca
# define access			_access
# define mkdir(path, mode)	_mkdir(path)
# define open			_open
# define write			_write
# define close			_close
# define DLL_EXT		".dll"

typedef HMODULE Handle;

static CRITICAL_SECTION lock;
# endif

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
    (*lpc_md5_end)(hash, digest, tmp, (uint16_t) sz, size);
}

/*
 * NAME:	JIT->thread()
 * DESCRIPTION:	receive objects compiled or removed
 */
static void *jit_thread(void *arg)
{
    uint8_t hash[24];
    Handle handle;

    while (lpc_ext_read(hash + 7, 17) == 17) {
	if (hash[7] == '\0') {
	    char fname[33];
	    char module[1000];
	    Program *p;
	    LPC_function *functions;

	    /* compiled */
	    filename(fname, hash + 8);
	    sprintf(module, "%s/cache/%c%c/%s" DLL_EXT, configDir, fname[0],
		    fname[1], fname);
	    p = NULL;
	    handle = DLL_OPEN(module);
	    if (handle != NULL) {
		functions = (LPC_function *) DLL_SYM(handle, "functions");
		if (functions != NULL) {
		    MUTEX_LOCK(&lock); {
			p = *p_find(hash + 8);
			if (p != NULL) {
			    p->handle = handle;
			    p->functions = functions;
			} else {
			    p = NULL;
			}
		    } MUTEX_UNLOCK(&lock);
		}
	    }

	    if (p == NULL && handle != NULL) {
		DLL_CLOSE(handle);
	    }
	} else {
	    uint64_t index, instance;

	    /* removed */
	    index = *((uint64_t *) (hash + 8));
	    instance = *((uint64_t *) (hash + 16));

	    MUTEX_LOCK(&lock); {
		Object **r;

		r = o_find(index, instance);
		if (*r != NULL) {
		    handle = o_del(r);
		    if (handle != NULL) {
			DLL_CLOSE(handle);
		    }
		}
	    } MUTEX_UNLOCK(&lock);
	}
    }

    return NULL;
}

/*
 * NAME:	JIT->init()
 * DESCRIPTION:	initialize JIT compiler interface
 */
static int jit_init(int major, int minor, size_t intSize, size_t inheritSize,
		    int tc, int nBuiltins, int nKfuns, uint8_t *protos,
		    size_t protoSize, void **vmtab)
{
    JitInfo info;
    bool result;

    /*
     * pass information to the JIT compiler backend
     */
    info.major = major;
    info.minor = minor;
    info.typechecking = typechecking = tc;
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
    vm = vmtab;

    /*
     * create loader thread
     */
    MUTEX_INIT(&lock);
    THREAD_START(&jit_thread);

    return true;
}

/*
 * NAME:	JIT->finish()
 * DESCRIPTION:	finish JIT compiler interface
 */
static void jit_finish(void)
{
    THREAD_STOP();
    MUTEX_DESTROY(&lock);
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
    unsigned char buffer[131072], *p;
    uint8_t hash[24];
    Object *c;
    char file[33], path[1000];
    int fd;

    /*
     * fill buffer with data for compiler backend
     */
    size = sizeof(JitCompile) + progSize + fTypeSize + vTypeSize;
    p = buffer;
    comp = (JitCompile *) p;
    memset(comp, '\0', sizeof(JitCompile));
    comp->typechecking = typechecking;
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
    md5hash(hash + 8, buffer, size);
    MUTEX_LOCK(&lock); {
	c = *o_find(index, instance);
	c->program = p_new(hash + 8);
    } MUTEX_UNLOCK(&lock);

    if (c->program->functions == NULL) {
	filename(file, hash + 8);
	sprintf(path, "%s/cache/%c%c", configDir, file[0], file[1]);
	mkdir(path, 0750);
	sprintf(path + strlen(path), "/%s", file);
	if (access(path, 0) == 0) {
	    /*
	     * reuse compiled object XXX may not be compiled yet?
	     */
	    hash[7] = '\0';
	    lpc_ext_writeback(hash + 7, 17);
	} else {
	    /*
	     * write to file
	     */
	    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0640);
	    write(fd, buffer, size);
	    close(fd);

	    /*
	     * inform backend
	     */
	    lpc_ext_write(hash + 8, 16);
	}
    }
}

/*
 * NAME:	JIT->execute()
 * DESCRIPTION:	attempt to execute a function
 */
static int jit_execute(uint64_t index, uint64_t instance, int version, int func,
		       void *arg)
{
    Object **r, *o;

    MUTEX_LOCK(&lock); {
	r = o_find(index, instance);
	o = *r;
	if (o == NULL) {
	    o_new(r, index, instance);
	}
    } MUTEX_UNLOCK(&lock);

    if (o != NULL) {
	if (o->program != NULL && o->program->functions != NULL) {
	    (o->program->functions[func])(vm, arg);
	    return 1;
	} else {
	    return 0;
	}
    } else {
	return -1;
    }
}

/*
 * NAME:	JIT->functions()
 * DESCRIPTION:	attempt to get the JIT-compiled function table
 */
static int jit_functions(uint64_t index, uint64_t instance, int version,
			 LPC_function **functions)
{
    Object **r, *o;

    MUTEX_LOCK(&lock); {
	r = o_find(index, instance);
	o = *r;
	if (o == NULL) {
	    o_new(r, index, instance);
	}
    } MUTEX_UNLOCK(&lock);

    if (o != NULL) {
	if (o->program != NULL && o->program->functions != NULL) {
	    *functions = o->program->functions;
	    return 1;
	} else {
	    return 0;
	}
    } else {
	return -1;
    }
}

/*
 * NAME:	JIT->release()
 * DESCRIPTION:	release JIT-compiled program
 */
static void jit_release(uint64_t index, uint64_t instance)
{
    uint8_t buf[24];

    buf[7] = '\1';
    *((uint64_t *) (buf + 8)) = index;
    *((uint64_t *) (buf + 16)) = instance;
    lpc_ext_writeback(buf + 7, 17);
}

/*
 * NAME:	LPC->ext_init()
 * DESCRIPTION:	initialize JIT compiler frontend
 */
int lpc_ext_init(int major, int minor, const char *config)
{
    char jitcomp[2000];

    strcpy(configDir, config);
# ifndef WIN32
    sprintf(jitcomp, "exec %s/jitcomp %s", config, config);
# else
    sprintf(jitcomp, "%s/jitcomp.exe %s", config, config);
# endif
    if (lpc_ext_spawn(jitcomp)) {
	(*lpc_ext_jit)(&jit_init, &jit_finish, &jit_compile, &jit_execute,
		       &jit_release, &jit_functions);
	return 1;
    }

    return 0;
}
