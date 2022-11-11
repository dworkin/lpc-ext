typedef uint8_t CodeByte;		/* code byte */
typedef uint16_t CodeSize;		/* function code size */
typedef uint16_t CodeLine;		/* line number */

class CodeContext {
public:
    class Kfun {
    public:
	LPCType *proto;		/* return and argument types */
	LPCParam nargs, vargs;	/* # arguments & optional arguments */
	bool lval;		/* has lvalue arguments? */
    };

    CodeContext(size_t intSize, size_t inhSize, CodeByte *protos, int nBuiltins,
		int nKfuns, int typechecking);
    virtual ~CodeContext();

    CodeByte *type(CodeByte *pc, LPCType *vType);

    static bool validVM(int major, int minor);

    size_t intSize;		/* integer size */
    size_t inhSize;		/* inherit size */
    Kfun *kfuns;		/* kfun prototypes */
    LPCKFun nkfun;		/* # kfuns */
    int typechecking;		/* typechecking mode */
};

class CodeObject {
public:
    CodeObject(CodeContext *context, LPCInherit nInherits, CodeByte *funcTypes,
	       CodeByte *varTypes);
    virtual ~CodeObject();

    Type varType(LPCGlobal *var);
    Type funcType(LPCDFunCall *func);

    CodeContext *context;	/* context */
    LPCInherit nInherits;	/* # inherits */

private:
    CodeByte **funcTypes;	/* function types */
    CodeByte **varTypes;	/* variable types */
};

class CodeFunction {
public:
    enum Class {
	CLASS_PRIVATE = 0x01,
	CLASS_STATIC = 0x02,
	CLASS_NOMASK = 0x04,
	CLASS_ELLIPSIS = 0x08,
	CLASS_VARARGS = 0x08,
	CLASS_ATOMIC = 0x10,
	CLASS_TYPECHECKED = 0x20,
	CLASS_UNDEFINED = 0x80
    };

    CodeFunction(CodeObject *object, CodeByte *prog);
    virtual ~CodeFunction();

    CodeByte *getPC(CodeSize *addr);
    void setPC(CodeByte *pc);
    CodeLine getLine(CodeByte instr);
    CodeByte *endProg();

    CodeObject *object;			/* code object */
    LPCType *proto;			/* function prototype */
    uint8_t fclass;			/* function class */
    LPCParam nargs, vargs;		/* # arguments & optional arguments */
    LPCLocal locals;			/* # locals */
    uint16_t stack;			/* stack depth */

private:
    CodeByte *program, *lines;		/* program & line numbers */
    CodeSize pc, lc;			/* program counter and line counter */
    CodeLine line;			/* current line */
};


class Code {
public:
    struct CaseInt {
	LPCInt num;			/* integer case label */
	CodeSize addr;			/* case address */
    };
    struct CaseRange {
	LPCInt from;			/* range from case label */
	LPCInt to;			/* range to case label */
	CodeSize addr;			/* case address */
    };
    struct CaseString {
	LPCStringConst str;		/* string constant case label */
	CodeSize addr;			/* case address */
    };

    Code(CodeFunction *function);
    virtual ~Code();

    virtual void evaluateTypes(class TypedContext *context);
    virtual void evaluateFlow(class FlowContext *context);
    virtual void emit(class GenContext *context) = 0;

    static Code *produce(CodeFunction *function);
    static void producer(Code *(*factory)(CodeFunction*));

    Code *next;				/* following instruction */
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
	SPREAD_LVAL,
	AGGREGATE,
	MAP_AGGREGATE,
	CAST,
	INSTANCEOF,
	CHECK_RANGE,
	CHECK_RANGE_FROM,
	CHECK_RANGE_TO,
	STORES,
	STORES_LVAL,
	STORE_PARAM,
	STORE_LOCAL,
	STORE_GLOBAL,
	STORE_INDEX,
	STORE_PARAM_INDEX,
	STORE_LOCAL_INDEX,
	STORE_GLOBAL_INDEX,
	STORE_INDEX_INDEX,
	STORES_SPREAD,
	STORES_CAST,
	STORES_PARAM,
	STORES_LOCAL,
	STORES_GLOBAL,
	STORES_INDEX,
	STORES_PARAM_INDEX,
	STORES_LOCAL_INDEX,
	STORES_GLOBAL_INDEX,
	STORES_INDEX_INDEX,
	JUMP,
	JUMP_ZERO,
	JUMP_NONZERO,
	SWITCH_INT,
	SWITCH_RANGE,
	SWITCH_STRING,
	KFUNC,
	KFUNC_LVAL,
	KFUNC_SPREAD,
	KFUNC_SPREAD_LVAL,
	DFUNC,
	DFUNC_SPREAD,
	FUNC,
	FUNC_SPREAD,
	CATCH,
	CAUGHT,
	END_CATCH,
	RLIMITS,
	RLIMITS_CHECK,
	END_RLIMITS,
	RETURN
    } instruction;
    CodeSize size;			/* size */
    union {
	LPCInt num;			/* integer */
	LPCFloat flt;			/* float */
	LPCStringConst str;		/* string constant */
	LPCParam param;			/* function parameter */
	LPCLocal local;			/* local variable */
	LPCGlobal var;			/* global variable */
	LPCType type;			/* type */
	int8_t spread;			/* spread */
	CodeSize target;		/* target address */
	CaseInt *caseInt;		/* int case labels */
	CaseRange *caseRange;		/* range case labels */
	CaseString *caseString;		/* string case labels */
	LPCKFunCall kfun;		/* kernel function call */
	LPCDFunCall dfun;		/* direct function call */
	LPCVFunCall fun;		/* virtual function call */
    };

private:
    CodeByte *switchInt(CodeByte *pc);
    CodeByte *switchRange(CodeByte *pc);
    CodeByte *switchStr(CodeByte *pc, CodeContext *context);

    static Code *(*factory)(CodeFunction*);
};
