typedef uint8_t CodeByte;		/* code byte */
typedef uint16_t CodeSize;		/* function code size */
typedef uint16_t CodeLine;		/* line number */
typedef uint16_t CodeMap;		/* kfun map */

typedef struct {
    LPCInt num;				/* integer case label */
    CodeSize addr;			/* case address */
} CodeCaseInt;

typedef struct {
    LPCInt from;			/* range from case label */
    LPCInt to;				/* range to case label */
    CodeSize addr;			/* case address */
} CodeCaseRange;

typedef struct {
    LPCStringConst str;			/* string constant case label */
    CodeSize addr;			/* case address */
} CodeCaseString;

typedef struct Code {
    struct Code *next;			/* following instruction */
    CodeSize addr;			/* address of this instruction */
    CodeLine line;			/* line number of this instruction */
    bool pop;				/* pop stack? */
    enum {
	INT,
	FLOAT,
	STRING,
	PARAM,
	LOCAL,
	GLOBAL,
	INDEX,
	INDEX2,
	SPREAD,
	SPREAD_STORES,
	AGGREGATE,
	MAP_AGGREGATE,
	CAST,
	INSTANCEOF,
	CHECK_RANGE,
	CHECK_RANGE_FROM,
	CHECK_RANGE_TO,
	STORES,
	STORE_PARAM,
	STORE_LOCAL,
	STORE_GLOBAL,
	STORE_INDEX,
	STORE_PARAM_INDEX,
	STORE_LOCAL_INDEX,
	STORE_GLOBAL_INDEX,
	STORE_INDEX_INDEX,
	JUMP,
	JUMP_ZERO,
	JUMP_NONZERO,
	SWITCH_INT,
	SWITCH_RANGE,
	SWITCH_STRING,
	KFUNC,
	KFUNC_STORES,
	DFUNC,
	FUNC,
	CATCH,
	END_CATCH,
	RLIMITS,
	RLIMITS_CHECK,
	END_RLIMITS,
	RETURN
    } instruction;
    uint16_t size;			/* size */
    union {
	LPCInt num;			/* integer */
	LPCFloat flt;			/* float */
	LPCStringConst str;		/* string constant */
	LPCLocal param;			/* function parameter */
	LPCLocal local;			/* local variable */
	LPCGlobal var;			/* global variable */
	LPCType type;			/* type */
	int8_t spread;			/* spread */
	CodeSize addr;			/* address */
	CodeCaseInt *caseInt;		/* int case labels */
	CodeCaseRange *caseRange;	/* range case labels */
	CodeCaseString *caseString;	/* string case labels */
	LPCKFunc kfun;			/* kernel function call */
	LPCDFunc dfun;			/* direct function call */
	LPCVFunc fun;			/* virtual function call */
    } u;
} Code;

typedef struct {
    struct CodeContext *context;	/* code context */
    LPCType *proto;			/* function prototype */
    uint8_t fclass;			/* function class */
    uint8_t nargs, vargs;		/* # arguments & optional arguments */
    uint8_t locals;			/* # locals */
    uint16_t stack;			/* stack depth */
    CodeByte *program, *lines;		/* program & line numbers */
    CodeSize pc, lc;			/* program counter and line counter */
    CodeLine line;			/* current line */
    Code *list, *last;			/* list of code in this function */
} CodeFunction;

enum FunctionClass {
    CLASS_PRIVATE = 0x01,
    CLASS_STATIC = 0x02,
    CLASS_NOMASK = 0x04,
    CLASS_ELLIPSIS = 0x08,
    CLASS_VARARGS = 0x08,
    CLASS_ATOMIC = 0x10,
    CLASS_TYPECHECKED = 0x20,
    CLASS_UNDEFINED = 0x80
};

extern struct CodeContext *code_init	(int, int, size_t, size_t, CodeMap*,
					 int, CodeByte*, int);
extern void		   code_clear	(struct CodeContext*);
extern void		   code_object	(struct CodeContext*, LPCInherit,
					 CodeByte*, CodeByte*);
extern CodeFunction	  *code_new	(struct CodeContext*, CodeByte*);
extern void		   code_del	(CodeFunction*);
extern Code		  *code_instr	(CodeFunction*);
