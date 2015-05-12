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
	CODE_JUMP,
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
	CODE_END_CATCH,
	CODE_RLIMITS,
	CODE_RLIMITS_CHECK,
	CODE_END_RLIMITS,
	CODE_RETURN
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
    uint8_t nargs, vargs;		/* # arguments & optional arguments */
    bool ellipsis;			/* has ellipsis? */
    uint8_t locals;			/* # locals */
    uint16_t stack;			/* stack depth */
    CodeByte *program, *lines;		/* program & line numbers */
    CodeSize pc, lc;			/* program counter and line counter */
    CodeLine line;			/* current line */
    Code *list, *last;			/* list of code in this function */
} CodeFunction;

extern struct CodeContext *code_init	(int, int, size_t, size_t, CodeMap*,
					 int, CodeByte*, int);
extern void		   code_clear	(struct CodeContext*);
extern void		   code_object	(struct CodeContext*, LPCInherit,
					 CodeByte*, CodeByte*);
extern CodeFunction	  *code_new	(struct CodeContext*, CodeByte*);
extern void		   code_del	(CodeFunction*);
extern Code		  *code_instr	(CodeFunction*);
