typedef struct {
    uint8_t major;		/* 2 */
    uint8_t minor;		/* 4 */
    uint16_t flags;            /* flags */
    size_t intSize;		/* size of Int */
    size_t inheritSize;		/* size of inherit offset */
    int nBuiltins;		/* # builtin prototypes */
    int nKfuns;			/* # kfun prototypes */
    size_t protoSize;		/* size of all prototypes together */
} JitInfo;

typedef struct {
    uint8_t flags;             /* flags (affects checksum) */
    uint8_t intInheritSize;	/* integer & inherit size */
    uint16_t nInherits;		/* # inherited objects */
    int nFunctions;		/* # functions */
    size_t progSize;		/* program size */
    size_t fTypeSize;		/* function type size */
    size_t vTypeSize;		/* variable type size */
} JitCompile;

/* flags */
# define JIT_TYPECHECKING      0x0f    /* typechecking mode */
# define JIT_NOREF             0x10    /* no reference counting */
