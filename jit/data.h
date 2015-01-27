# define T_TYPE			0x0f
# define T_NIL			0x00	/* runtime only */
# define T_INT			0x01
# define T_FLOAT		0x02
# define T_STRING		0x03
# define T_OBJECT		0x04
# define T_ARRAY		0x05	/* runtime only */
# define T_MAPPING		0x06
# define T_LWOBJECT		0x07	/* runtime only */
# define T_CLASS		0x07    /* declaration only */
# define T_MIXED		0x08    /* declaration only */
# define T_VOID			0x09    /* declaration only */
# define T_LVALUE		0x0a	/* kfun declaration only */

# define T_REF			0xf0    /* reference count mask */
# define REFSHIFT		4

typedef int64_t LPCInt;

typedef struct {
    uint32_t high;		/* 1 sign, 15 exponent, ... */
    uint64_t low;		/* ... 80 mantissa */
} LPCFloat;

typedef uint16_t LPCInherit;

# define INHERIT_PROG		0xffff

typedef struct {
    LPCInherit inherit;		/* program */
    uint16_t index;		/* index in string constant table */
} LPCStringConst;

typedef struct {
    uint8_t type;		/* compile-time type */
    LPCInherit inherit;		/* optional class string program */
    uint16_t index;		/* optional class string index */
} LPCType;

typedef uint8_t LPCLocal;

typedef struct {
    LPCInherit inherit;		/* program */
    uint8_t index;		/* index in variable table */
} LPCGlobal;

typedef struct {
    uint16_t func;		/* index in kfun table */
    uint8_t nargs;		/* # arguments */
} LPCKFunc;

typedef struct {
    LPCInherit inherit;		/* program */
    uint8_t func;		/* index in function table */
    uint8_t nargs;		/* # arguments */
} LPCDFunc;

typedef struct {
    uint16_t call;		/* index in call table */
    uint8_t nargs;		/* # arguments */
} LPCVFunc;
