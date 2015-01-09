typedef uint16_t CodeAddr;		/* code address */
typedef uint8_t CodeByte;		/* code byte */
typedef uint16_t CodeLine;		/* line number */

typedef struct {
    LPCInt num;				/* integer case label */
    CodeAddr addr;			/* case address */
} CodeCaseInt;

typedef struct {
    LPCInt from;			/* range from case label */
    LPCInt to;				/* range to case label */
    CodeAddr addr;			/* case address */
} CodeCaseRange;

typedef struct {
    LPCStringConst str;			/* string constant case label */
    CodeAddr addr;			/* case address */
} CodeCaseString;

typedef struct Code {
    struct Code *list;			/* previous instruction */
    CodeAddr addr;			/* address of this instruction */
    CodeLine line;			/* line number of this instruction */
    enum {
	CODE_INT,
	CODE_FLOAT,
	CODE_STRING,
	CODE_PARAM,
	CODE_LOCAL,
	CODE_GLOBAL,
	CODE_INDEX,
	CODE_INDEX2,
	CODE_SPREAD,
	CODE_SPREAD_STORES,
	CODE_AGGREGATE,
	CODE_MAP_AGGREGATE,
	CODE_CAST,
	CODE_INSTANCEOF,
	CODE_CHECK_RANGE,
	CODE_CHECK_RANGE_FROM,
	CODE_CHECK_RANGE_TO,
	CODE_STORES,
	CODE_STORE_PARAM,
	CODE_STORE_LOCAL,
	CODE_STORE_GLOBAL,
	CODE_STORE_INDEX,
	CODE_STORE_PARAM_INDEX,
	CODE_STORE_LOCAL_INDEX,
	CODE_STORE_GLOBAL_INDEX,
	CODE_STORE_INDEX_INDEX,
	CODE_JUMP_ZERO,
	CODE_JUMP_NONZERO,
	CODE_SWITCH_INT,
	CODE_SWITCH_RANGE,
	CODE_SWITCH_STRING,
	CODE_KFUNC,
	CODE_KFUNC_STORES,
	CODE_DFUNC,
	CODE_FUNC,
	CODE_CATCH,
	CODE_RLIMITS,
	CODE_RETURN
    } instruction;
    bool pop;				/* pop stack? */
    uint16_t size;			/* size of switch list */
    union {
	LPCInt num;			/* integer */
	LPCFloat flt;			/* float */
	LPCStringConst str;		/* string constant */
	CodeLocal param;		/* function parameter */
	CodeLocal local;		/* local variable */
	CodeGlobal var;			/* global variable */
	LPCType type;			/* type */
	CodeAddr addr;			/* address */
	CodeCaseInt *caseInt;		/* int case labels */
	CodeCaseRange *caseRange;	/* range case labels */
	CodeCaseString *caseString;	/* string case labels */
	LPCKFunc kfunc;			/* kernel function call */
	LPCDFunc dfunc;			/* direct function call */
	LPCVFunc func;			/* virtual function call */
    } u;
    struct Code *next;			/* following instruction */
} Code;

typedef struct {
    struct CodeContext *context;	/* code context */
    char *name;				/* function name */
    LPCType *proto;			/* function prototype */
    uint8_t nargs, vargs;		/* # arguments & optional arguments */
    CodeByte *program, *pc;		/* program & program counter */
    CodeByte *lines, *lc;		/* line info and line counter */
    CodeLine line;			/* current line */
    Code *list;				/* list of code in this function */
} CodeFunction;

extern struct CodeContext *code_init	(int, int, size_t, size_t);
extern CodeFunction	  *code_new	(struct CodeContext*, char*, CodeByte*);
extern void		   code_del	(CodeFunction*);
extern Code		  *code_instr	(CodeFunction*);
extern void		   code_clear	(struct CodeContext*);
