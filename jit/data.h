# define LPC_TYPE_CLASS		7
# define LPC_TYPE_MASK		0x0f
# define LPC_TYPE_REF(t)	((t) >> 4)

typedef int64_t LPCInt;

struct LPCFloat {
    uint32_t high;		/* 1 sign, 15 exponent, ... */
    uint64_t low;		/* ... 80 mantissa */
};

typedef uint16_t LPCInherit;

struct LPCStringConst {
    LPCInherit inherit;		/* program */
    uint16_t index;		/* index in string constant table */
};

typedef uint8_t Type;

struct LPCType {
    Type type;			/* compile-time type */
    LPCInherit inherit;		/* optional class string program */
    uint16_t index;		/* optional class string index */
};

typedef uint8_t LPCParam;
typedef uint8_t LPCLocal;

struct LPCGlobal {
    LPCInherit inherit;		/* program */
    uint8_t index;		/* index in variable table */
    Type type;			/* type of variable */
};

typedef uint16_t LPCKFun;

struct LPCKFunCall {
    LPCKFun func;		/* index in kfun table */
    LPCParam nargs;		/* # arguments */
    LPCParam lval;		/* # non-lvalue parameters or 0 */
    Type type;			/* return type */
};

struct LPCDFunCall {
    LPCInherit inherit;		/* program */
    uint8_t func;		/* index in function table */
    LPCParam nargs;		/* # arguments */
    Type type;			/* return type */
};

struct LPCVFunCall {
    uint16_t call;		/* index in call table */
    LPCParam nargs;		/* # arguments */
};
