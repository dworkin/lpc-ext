# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
# include <string.h>
# include "lpc_ext.h"
# include "data.h"
# include "instruction.h"
# include "code.h"
# include "jit.h"

# define FETCH1U(pc)	(*(pc)++)
# define FETCH1S(pc)	((int8_t) *(pc)++)
# define GET2(pc)	(((uint16_t) (pc)[-2] << 8) + (pc)[-1])
# define FETCH2U(pc)	((pc) += 2, GET2(pc))
# define FETCH2S(pc)	((pc) += 2, ((int16_t) (int8_t) (pc)[-2] << 8) + \
				    (pc)[-1])
# define GET3(pc)	(((uint32_t) (pc)[-3] << 16) + GET2(pc))
# define FETCH3U(pc)	((pc) += 3, GET3(pc))
# define FETCH3S(pc)	((pc) += 3, ((int32_t) (int8_t) (pc)[-3] << 16) + \
				    GET2(pc))
# define GET4(pc)	(((uint32_t) (pc)[-4] << 24) + GET3(pc))
# define FETCH4U(pc)	((pc) += 4, GET4(pc))
# define FETCH4S(pc)	((pc) += 4, ((int32_t) (int8_t) (pc)[-4] << 24) + \
				    GET3(pc))
# define GET5(pc)	(((uint64_t) (pc)[-5] << 32) + GET4(pc))
# define FETCH5U(pc)	((pc) += 5, GET5(pc))
# define FETCH5S(pc)	((pc) += 5, ((int64_t) (int8_t) (pc)[-5] << 32) + \
				    GET4(pc))
# define GET6(pc)	(((uint64_t) (pc)[-6] << 40) + GET5(pc))
# define FETCH6U(pc)	((pc) += 6,GET6(pc))
# define FETCH6S(pc)	((pc) += 6, ((int64_t) (int8_t) (pc)[-6] << 40) + \
				    GET5(pc))
# define GET7(pc)	(((uint64_t) (pc)[-7] << 48) + GET6(pc))
# define FETCH7U(pc)	((pc) += 7, GET7(pc))
# define FETCH7S(pc)	((pc) += 7, ((int64_t) (int8_t) (pc)[-7] << 48) + \
				    GET6(pc))
# define GET8(pc)	(((uint64_t) (pc)[-8] << 56) + GET7(pc))
# define FETCH8U(pc)	((pc) += 8, GET8(pc))
# define FETCH8S(pc)	((pc) += 8, (int64_t) GET8(pc))

# define FETCHI(pc, cc) (((cc)->inhSize > sizeof(char)) ? \
			  FETCH2U(pc) : FETCH1U(pc))

# define ELLIPSIS	0x08

# define PROTO_CLASS(p)	((p)[0])
# define PROTO_NARGS(p)	((p)[1])
# define PROTO_VARGS(p)	((p)[2])
# define PROTO_FTYPE(p)	((p)[5])

# define KF_CKRANGEFT	51
# define KF_CKRANGEF	52
# define KF_CKRANGET	53

typedef struct {
    LPCType *proto;		/* return and argument types */
    uint8_t nargs, vargs;	/* # arguments & optional arguments */
    bool lval;			/* has lvalue arguments? */
} CodeProto;

typedef struct CodeContext {
    size_t intSize;		/* integer size */
    size_t inhSize;		/* inherit size */
    CodeMap *map;		/* kfun mapping */
    CodeProto *kfun;		/* kfun prototypes */
    uint16_t nkfun;		/* # kfuns */
} CodeContext;

/*
 * NAME:	Code->type()
 * DESCRIPTION:	retrieve a return type or argument type
 */
static CodeByte *code_type(CodeContext *context, CodeByte *pc, LPCType *type)
{
    type->type = FETCH1U(pc);
    if (type->type == LPC_TYPE_CLASS) {
	type->inherit = FETCHI(pc, context);
	type->index = FETCH2U(pc);
    }

    return pc;
}

/*
 * NAME:	Code->init()
 * DESCRIPTION:	initialize code retriever
 */
CodeContext *code_init(int major, int minor, size_t intSize, size_t inhSize,
		       CodeMap *map, int nMap, CodeByte *protos, int nProto)
{
    CodeContext *context;
    int i, size;
    CodeProto *kfun;
    LPCType *proto;

    /* check VM version */
    if (major != VERSION_VM_MAJOR || minor > VERSION_VM_MINOR) {
	return NULL;
    }

    /* allocate code context */
    context = alloc(CodeContext, 1);
    context->intSize = intSize;
    context->inhSize = inhSize;
    context->map = alloc(CodeMap, nMap);
    memcpy(context->map, map, nMap * sizeof(CodeMap));

    /*
     * initialize kfun prototype table
     */
    context->kfun = alloc(CodeProto, context->nkfun = nProto);
    for (kfun = context->kfun, i = 0; i < context->nkfun; kfun++, i++) {
	if (protos[0] == 0) {
	    protos++;
	    kfun->proto = NULL;
	} else {
	    kfun->lval = FALSE;
	    kfun->nargs = PROTO_NARGS(protos);
	    kfun->vargs = PROTO_VARGS(protos);
	    size = kfun->nargs + kfun->vargs + 1;
	    kfun->proto = proto = alloc(LPCType, size);
	    protos = &PROTO_FTYPE(protos);
	    do {
		protos = code_type(context, protos, proto);
		if (proto->type == LPC_TYPE_LVALUE) {
		    kfun->lval = TRUE;
		}
		proto++;
	    } while (--size != 0);
	}
    }

    return context;
}

/*
 * NAME:	Code->clear()
 * DESCRIPTION:	clear code retriever
 */
void code_clear(CodeContext *context)
{
    CodeProto *kfun;
    int i;

    for (kfun = context->kfun, i = 0; i < context->nkfun; kfun++, i++) {
	if (kfun->proto != NULL) {
	    free(kfun->proto);
	}
    }
    free(context->kfun);

    free(context);
}

/*
 * NAME:	Code->new()
 * DESCRIPTION:	create a function retriever
 */
CodeFunction *code_new(CodeContext *context, CodeByte *pc)
{
    CodeFunction *function;
    uint16_t size;
    LPCType *proto;

    function = alloc(CodeFunction, 1);
    function->context = context;

    /* retrieve prototype */
    function->nargs = PROTO_NARGS(pc);
    function->vargs = PROTO_VARGS(pc);
    size = function->nargs + function->vargs + 1;
    function->proto = proto = alloc(LPCType, size);
    function->ellipsis = ((PROTO_CLASS(pc) & ELLIPSIS) != 0);
    pc = &PROTO_FTYPE(pc);
    do {
	pc = code_type(context, pc, proto++);
    } while (--size != 0);

    /* prepare to retrieve code from function */
    function->stack = FETCH2U(pc);
    function->locals = FETCH1U(pc);
    size = FETCH2U(pc);
    function->program = pc;
    function->lines = pc + size;
    function->pc = function->lc = 0;
    function->line = 0;
    function->list = NULL;
    function->last = &function->list;

    return function;
}

/*
 * NAME:	Code->switch_int()
 * DESCRIPTION:	retrieve int switch
 */
static CodeByte *code_switch_int(Code *code, CodeByte *pc, CodeContext *context)
{
    CodeCaseInt *caseInt;
    int i, bytes;

    code->size = FETCH2U(pc);
    code->u.caseInt = caseInt = alloc(CodeCaseInt, i = code->size);
    bytes = FETCH1U(pc);

    /* default */
    caseInt->addr = FETCH2U(pc);
    caseInt++;
    --i;

    /* cases */
    switch (bytes) {
    case 1:
	while (i != 0) {
	    caseInt->num = FETCH1S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 2:
	while (i != 0) {
	    caseInt->num = FETCH2S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 3:
	while (i != 0) {
	    caseInt->num = FETCH3S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 4:
	while (i != 0) {
	    caseInt->num = FETCH4S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 5:
	while (i != 0) {
	    caseInt->num = FETCH5S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 6:
	while (i != 0) {
	    caseInt->num = FETCH6S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 7:
	while (i != 0) {
	    caseInt->num = FETCH7S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;

    case 8:
	while (i != 0) {
	    caseInt->num = FETCH8S(pc);
	    caseInt->addr = FETCH2U(pc);
	    caseInt++;
	    --i;
	}
	break;
    }

    return pc;
}

/*
 * NAME:	Code->switch_range()
 * DESCRIPTION:	retrieve range switch
 */
static CodeByte *code_switch_range(Code *code, CodeByte *pc,
				   CodeContext *context)
{
    CodeCaseRange *caseRange;
    int i, bytes;

    code->size = FETCH2U(pc);
    code->u.caseRange = caseRange = alloc(CodeCaseRange, i = code->size);
    bytes = FETCH1U(pc);

    /* default */
    caseRange->addr = FETCH2U(pc);
    caseRange++;
    --i;

    /* cases */
    switch (bytes) {
    case 1:
	while (i != 0) {
	    caseRange->from = FETCH1S(pc);
	    caseRange->to = FETCH1S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 2:
	while (i != 0) {
	    caseRange->from = FETCH2S(pc);
	    caseRange->to = FETCH2S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 3:
	while (i != 0) {
	    caseRange->from = FETCH3S(pc);
	    caseRange->to = FETCH3S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 4:
	while (i != 0) {
	    caseRange->from = FETCH4S(pc);
	    caseRange->to = FETCH4S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 5:
	while (i != 0) {
	    caseRange->from = FETCH5S(pc);
	    caseRange->to = FETCH5S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 6:
	while (i != 0) {
	    caseRange->from = FETCH6S(pc);
	    caseRange->to = FETCH6S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 7:
	while (i != 0) {
	    caseRange->from = FETCH7S(pc);
	    caseRange->to = FETCH7S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    case 8:
	while (i != 0) {
	    caseRange->from = FETCH8S(pc);
	    caseRange->to = FETCH8S(pc);
	    caseRange->addr = FETCH2U(pc);
	    caseRange++;
	    --i;
	}
	break;

    }

    return pc;
}

/*
 * NAME:	Code->switch_string()
 * DESCRIPTION:	retrieve string switch
 */
static CodeByte *code_switch_string(Code *code, CodeByte *pc,
				    CodeContext *context)
{
    CodeCaseString *caseString;
    int i;

    code->size = FETCH2U(pc);
    code->u.caseString = caseString = alloc(CodeCaseString, i = code->size);

    /* default */
    caseString->addr = FETCH2U(pc);
    caseString++;
    --i;

    /* nil */
    if (FETCH1U(pc) == 0) {
	caseString->str.inherit = THIS;
	caseString->str.index = 0xffff;
	caseString->addr = FETCH2U(pc);
	caseString++;
	--i;
    }

    /* cases */
    while (i != 0) {
	caseString->str.inherit = FETCHI(pc, context);
	caseString->str.index = FETCH2U(pc);
	caseString->addr = FETCH2U(pc);
	caseString++;
	--i;
    }

    return pc;
}

/*
 * NAME:	Code->instr()
 * DESCRIPTION:	retrieve code from function
 */
Code *code_instr(CodeFunction *function)
{
    Code *code;
    CodeByte *pc, *numbers, instr;
    LPCFloat flt;
    uint16_t offset, fexp;
    int i;
    CodeProto *kfun;

    pc = function->program + function->pc;
    if (pc == function->lines) {
	return NULL;	/* no more code to retrieve */
    }

    /* allocate new code */
    code = alloc(Code, 1);
    code->list = NULL;
    *function->last = code;
    function->last = &code->list;

    /*
     * retrieve instruction
     */
    code->addr = function->pc;
    code->pop = FALSE;
    instr = FETCH1U(pc);

    /*
     * retrieve line number
     */
    numbers = function->lines + function->lc;
    code->line = function->line;
    offset = instr >> I_LINE_SHIFT;
    if (offset <= 2) {
	code->line += offset;
    } else {
	offset = FETCH1U(numbers);
	if (offset >= 128) {
	    code->line += offset - 128 - 64;
	} else {
	    code->line += ((offset << 8) | FETCH1U(numbers)) - 16384;
	}
    }
    function->line = code->line;
    function->lc = numbers - function->lines;

    /*
     * process instruction
     */
    switch (instr & I_INSTR_MASK) {
    case I_INT1:
	code->u.num = FETCH1S(pc);
	code->instruction = CODE_INT;
	break;

    case I_INT2:
	code->u.num = FETCH2S(pc);
	code->instruction = CODE_INT;
	break;

    case I_INT4:
	code->u.num = FETCH4S(pc);
	code->instruction = CODE_INT;
	break;

    case I_INT8:
	code->u.num = FETCH8S(pc);
	code->instruction = CODE_INT;
	break;

    case I_FLOAT6:
	flt.high = FETCH2U(pc);
	flt.low = FETCH4U(pc);
	fexp = (flt.high & ~0x8000) >> 4;
	if (fexp == 0) {
	    code->u.flt.high = 0;
	    code->u.flt.low = 0;
	} else {
	    code->u.flt.high = (((flt.high & 0x8000) + fexp + 0x3c00) << 16) +
			       ((flt.high & 0xf) << 12) + (flt.low >> 20);
	    code->u.flt.low = flt.low << 44;
	}
	code->instruction = CODE_FLOAT;
	break;

    case I_FLOAT12:
	code->u.flt.high = FETCH4U(pc);
	code->u.flt.low = FETCH8U(pc);
	code->instruction = CODE_FLOAT;
	break;

    case I_STRING:
	code->u.str.inherit = THIS;
	code->u.str.index = FETCH1U(pc);
	code->instruction = CODE_STRING;
	break;

    case I_NEAR_STRING:
	code->u.str.inherit = FETCH1U(pc);
	code->u.str.index = FETCH1U(pc);
	code->instruction = CODE_STRING;
	break;

    case I_FAR_STRING:
	code->u.str.inherit = FETCHI(pc, function->context);
	code->u.str.index = FETCH2U(pc);
	code->instruction = CODE_STRING;
	break;

    case I_LOCAL:
	i = FETCH1S(pc);
	if (i < 0) {
	    code->u.local = -(i + 1);
	    code->instruction = CODE_LOCAL;
	} else {
	    code->u.param = i;
	    code->instruction = CODE_PARAM;
	}
	break;

    case I_GLOBAL:
	code->u.var.inherit = THIS;
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_GLOBAL;
	break;

    case I_FAR_GLOBAL:
	code->u.var.inherit = FETCHI(pc, function->context);
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_GLOBAL;
	break;

    case I_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_INDEX:
	code->instruction = CODE_INDEX;
	break;

    case I_INDEX2:
	code->instruction = CODE_INDEX2;
	break;

    case I_AGGREGATE | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_AGGREGATE:
	code->instruction = (FETCH1U(pc) == 0) ?
			     CODE_AGGREGATE : CODE_MAP_AGGREGATE;
	code->size = FETCH2U(pc);
	break;

    case I_SPREAD:
	code->u.spread = FETCH1S(pc);
	if (code->u.spread >= 0) {
	    code->instruction = CODE_SPREAD;
	} else {
	    code->u.spread = -(code->u.spread + 2);
	    code->instruction = CODE_SPREAD_STORES;
	}
	break;

    case I_CAST | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CAST:
	pc = code_type(function->context, pc, &code->u.type);
	code->instruction = CODE_CAST;
	break;

    case I_INSTANCEOF | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_INSTANCEOF:
	code->u.str.inherit = FETCHI(pc, function->context);
	code->u.str.index = FETCH2U(pc);
	code->instruction = CODE_INSTANCEOF;
	break;

    case I_STORES:
	code->size = FETCH1U(pc);
	code->instruction = CODE_STORES;
	break;

    case I_STORE_LOCAL | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_LOCAL:
	i = FETCH1S(pc);
	if (i < 0) {
	    code->u.local = -(i + 1);
	    code->instruction = CODE_STORE_LOCAL;
	} else {
	    code->u.param = i;
	    code->instruction = CODE_STORE_PARAM;
	}
	break;

    case I_STORE_GLOBAL | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_GLOBAL:
	code->u.var.inherit = THIS;
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_STORE_GLOBAL;
	break;

    case I_STORE_FAR_GLOBAL | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_FAR_GLOBAL:
	code->u.var.inherit = FETCHI(pc, function->context);
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_STORE_GLOBAL;
	break;

    case I_STORE_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_INDEX:
	code->instruction = CODE_STORE_INDEX;
	break;

    case I_STORE_LOCAL_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_LOCAL_INDEX:
	i = FETCH1S(pc);
	if (i < 0) {
	    code->u.local = -(i + 1);
	    code->instruction = CODE_STORE_LOCAL_INDEX;
	} else {
	    code->u.param = i;
	    code->instruction = CODE_STORE_PARAM_INDEX;
	}
	break;

    case I_STORE_GLOBAL_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_GLOBAL_INDEX:
	code->u.var.inherit = THIS;
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_STORE_GLOBAL_INDEX;
	break;

    case I_STORE_FAR_GLOBAL_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_FAR_GLOBAL_INDEX:
	code->u.var.inherit = FETCHI(pc, function->context);
	code->u.var.index = FETCH1U(pc);
	code->instruction = CODE_STORE_GLOBAL_INDEX;
	break;

    case I_STORE_INDEX_INDEX | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_STORE_INDEX_INDEX:
	code->instruction = CODE_STORE_INDEX_INDEX;
	break;

    case I_JUMP_ZERO:
	code->pop = TRUE;
	code->u.addr = FETCH2U(pc);
	code->instruction = CODE_JUMP_ZERO;
	break;

    case I_JUMP_NONZERO:
	code->pop = TRUE;
	code->u.addr = FETCH2U(pc);
	code->instruction = CODE_JUMP_NONZERO;
	break;

    case I_JUMP:
	code->u.addr = FETCH2U(pc);
	code->instruction = CODE_JUMP;
	break;

    case I_SWITCH:
	code->pop = TRUE;
	switch (FETCH1U(pc)) {
	case SWITCH_INT:
	    pc = code_switch_int(code, pc, function->context);
	    code->instruction = CODE_SWITCH_INT;
	    break;

	case SWITCH_RANGE:
	    pc = code_switch_range(code, pc, function->context);
	    code->instruction = CODE_SWITCH_RANGE;
	    break;

	case SWITCH_STRING:
	    pc = code_switch_string(code, pc, function->context);
	    code->instruction = CODE_SWITCH_STRING;
	    break;
	}
	break;

    case I_CALL_KFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_KFUNC:
	code->u.kfun.func = function->context->map[FETCH1U(pc)];
	switch (code->u.kfun.func) {
	case KF_CKRANGEFT:
	    code->instruction = CODE_CHECK_RANGE;
	    break;

	case KF_CKRANGEF:
	    code->instruction = CODE_CHECK_RANGE_FROM;
	    break;

	case KF_CKRANGET:
	    code->instruction = CODE_CHECK_RANGE_TO;
	    break;

	default:
	    kfun = &function->context->kfun[code->u.kfun.func];
	    if (kfun->vargs != 0) {
		code->u.kfun.nargs = FETCH1U(pc);
	    } else {
		code->u.kfun.nargs = kfun->nargs;
	    }
	    code->instruction = (kfun->lval) ? CODE_KFUNC_STORES : CODE_KFUNC;
	    break;
	}
	break;

    case I_CALL_EFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_EFUNC:
	code->u.kfun.func = function->context->map[FETCH2U(pc)];
	kfun = &function->context->kfun[code->u.kfun.func];
	if (kfun->vargs != 0) {
	    code->u.kfun.nargs = FETCH1U(pc);
	} else {
	    code->u.kfun.nargs = kfun->nargs;
	}
	code->instruction = (kfun->lval) ? CODE_KFUNC_STORES : CODE_KFUNC;
	break;

    case I_CALL_CKFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_CKFUNC:
	code->u.kfun.func = function->context->map[FETCH1U(pc)];
	code->u.kfun.nargs = FETCH1U(pc);
	kfun = &function->context->kfun[code->u.kfun.func];
	code->instruction = (kfun->lval) ? CODE_KFUNC_STORES : CODE_KFUNC;
	break;

    case I_CALL_CEFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_CEFUNC:
	code->u.kfun.func = function->context->map[FETCH2U(pc)];
	code->u.kfun.nargs = FETCH1U(pc);
	kfun = &function->context->kfun[code->u.kfun.func];
	code->instruction = (kfun->lval) ? CODE_KFUNC_STORES : CODE_KFUNC;
	break;

    case I_CALL_AFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_AFUNC:
	code->u.dfun.inherit = 0;
	code->u.dfun.func = FETCH1U(pc);
	code->u.dfun.nargs = FETCH1U(pc);
	code->instruction = CODE_DFUNC;
	break;

    case I_CALL_DFUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_DFUNC:
	code->u.dfun.inherit = FETCHI(pc, function->context);
	code->u.dfun.func = FETCH1U(pc);
	code->u.dfun.nargs = FETCH1U(pc);
	code->instruction = CODE_DFUNC;
	break;

    case I_CALL_FUNC | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CALL_FUNC:
	code->u.fun.call = FETCH2U(pc);
	code->u.fun.nargs = FETCH1U(pc);
	code->instruction = CODE_FUNC;
	break;

    case I_CATCH | I_POP_BIT:
	code->pop = TRUE;
	/* fall through */
    case I_CATCH:
	code->u.addr = FETCH2U(pc);
	code->instruction = CODE_CATCH;
	break;

    case I_RLIMITS:
	code->pop = TRUE;
	code->instruction = (FETCH1U(pc) != 0) ?
			     CODE_RLIMITS : CODE_RLIMITS_CHECK;
	break;

    case I_RETURN:
	code->instruction = CODE_RETURN;
	break;

    default:
	fatal("retrieved unknown instruction 0x%02x", instr);
    }
    function->pc = pc - function->program;

    return code;
}

/*
 * NAME:	Code->remove()
 * DESCRIPTION:	remove retrieved code
 */
static void code_remove(Code *code)
{
    switch (code->instruction) {
    case CODE_SWITCH_INT:
	free(code->u.caseInt);
	break;

    case CODE_SWITCH_RANGE:
	free(code->u.caseRange);
	break;

    case CODE_SWITCH_STRING:
	free(code->u.caseString);
	break;

    default:
	break;
    }

    free(code);
}

/*
 * NAME:	Code->del()
 * DESCRIPTION:	delete a function retriever
 */
void code_del(CodeFunction *function)
{
    Code *code, *next;

    /* delete all code retrieved for this function */
    for (code = function->list; code != NULL; code = next) {
	next = code->list;
	code_remove(code);
    }

    free(function->proto);
    free(function);
}
