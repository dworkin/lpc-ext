typedef struct {
    uint8_t major;		/* 2 */
    uint8_t minor;		/* 3 */
    uint16_t typechecking;	/* typechecking mode */
    size_t intSize;		/* size of Int */
    size_t inheritSize;		/* size of inherit offset */
    int nBuiltins;		/* # builtin prototypes */
    int nKfuns;			/* # kfun prototypes */
    size_t protoSize;		/* size of all prototypes together */
} JitInfo;

typedef struct {
    uint8_t typechecking;	/* typechecking mode (affects checksum) */
    uint8_t intInheritSize;	/* integer & inherit size */
    uint16_t nInherits;		/* # inherited objects */
    int nFunctions;		/* # functions */
    size_t progSize;		/* program size */
    size_t fTypeSize;		/* function type size */
    size_t vTypeSize;		/* variable type size */
} JitCompile;
