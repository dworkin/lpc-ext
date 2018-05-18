# include <stdlib.h>
# include <stdint.h>
# include <new>
# include <string.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "instruction.h"
# include "code.h"
# include "jitcomp.h"

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

# define PROTO_CLASS(p)	((p)[0])
# define PROTO_NARGS(p)	((p)[1])
# define PROTO_VARGS(p)	((p)[2])
# define PROTO_FTYPE(p)	((p)[5])

# define KF_CKRANGEFT	51
# define KF_CKRANGEF	52
# define KF_CKRANGET	53

/*
 * check VM version
 */
bool CodeContext::validVM(int major, int minor)
{
    return (major == VERSION_VM_MAJOR && minor <= VERSION_VM_MINOR);
}

/*
 * retrieve a return type or argument type
 */
CodeByte *CodeContext::type(CodeByte *pc, LPCType *vType)
{
    vType->type = FETCH1U(pc);
    if ((vType->type & LPC_TYPE_MASK) == LPC_TYPE_CLASS) {
	vType->inherit = FETCHI(pc, this);
	vType->index = FETCH2U(pc);
    }

    return pc;
}

CodeContext::CodeContext(size_t intSize, size_t inhSize, KfunMap *map, int nMap,
			 CodeByte *protos, int nProto)
{
    int i, size;
    Kfun *kfun;
    LPCType *proto;

    /* allocate code context */
    this->intSize = intSize;
    this->inhSize = inhSize;
    this->map = new KfunMap[nMap];
    memcpy(this->map, map, nMap * sizeof(KfunMap));

    /*
     * initialize kfun prototype table
     */
    kfuns = new Kfun[nkfun = nProto];
    for (kfun = kfuns, i = nkfun; i > 0; kfun++, --i) {
	if (protos[0] == 0) {
	    protos++;
	    kfun->proto = NULL;
	} else {
	    kfun->lval = false;
	    kfun->nargs = PROTO_NARGS(protos);
	    kfun->vargs = PROTO_VARGS(protos);
	    size = kfun->nargs + kfun->vargs + 1;
	    kfun->proto = proto = new LPCType[size];
	    protos = &PROTO_FTYPE(protos);
	    do {
		protos = type(protos, proto);
		if (proto->type == LPC_TYPE_LVALUE) {
		    kfun->lval = true;
		}
		proto++;
	    } while (--size != 0);
	}
    }
}

CodeContext::~CodeContext()
{
    Kfun *kfun;
    int i;

    for (kfun = kfuns, i = nkfun; i > 0; kfun++, --i) {
	if (kfun->proto != NULL) {
	    delete[] kfun->proto;
	}
    }
    delete[] kfuns;
}

CodeObject::CodeObject(CodeContext *context, LPCInherit nInherits,
		       CodeByte *funcTypes, CodeByte *varTypes)
{
    CodeByte **fTypes, **vTypes, len;
    LPCInherit i;

    this->context = context;
    this->nInherits = nInherits;
    this->funcTypes = fTypes = new CodeByte*[nInherits];
    this->varTypes = vTypes = new CodeByte*[nInherits];
    for (i = nInherits; i > 0; --i) {
	len = *funcTypes++;
	if (len != 0) {
	    *fTypes = new CodeByte[len];
	    memcpy(*fTypes++, funcTypes, len);
	    funcTypes += len;
	} else {
	    *fTypes++ = NULL;
	}
	len = *varTypes++;
	if (len != 0) {
	    *vTypes = new CodeByte[len];
	    memcpy(*vTypes++, varTypes, len);
	    varTypes += len;
	} else {
	    *vTypes++ = NULL;
	}
    }
}

CodeObject::~CodeObject()
{
    CodeByte **fTypes, **vTypes;
    LPCInherit i;

    for (fTypes = funcTypes, vTypes = varTypes, i = nInherits; i > 0;
	 fTypes++, vTypes++, --i) {
	if (*fTypes != NULL) {
	    delete[] *fTypes;
	}
	if (*vTypes != NULL) {
	    delete[] *vTypes;
	}
    }
    delete[] funcTypes;
    delete[] varTypes;
}

/*
 * NAME:	Code->new()
 * DESCRIPTION:	create a function retriever
 */
CodeFunction::CodeFunction(CodeObject *object, CodeByte **prog)
{
    CodeContext *context;
    CodeByte *pc;
    uint16_t size;
    LPCType *proto;

    this->object = object;
    context = object->context;

    /* retrieve prototype */
    pc = *prog;
    nargs = PROTO_NARGS(pc);
    vargs = PROTO_VARGS(pc);
    size = nargs + vargs + 1;
    this->proto = proto = new LPCType[size];
    fclass = PROTO_CLASS(pc);
    pc = &PROTO_FTYPE(pc);
    do {
	pc = context->type(pc, proto++);
    } while (--size != 0);

    /* retrieve code from function */
    this->pc = lc = 0;
    line = 0;
    first = NULL;
    if (!(fclass & CLASS_UNDEFINED)) {
	stack = FETCH2U(pc);
	locals = FETCH1U(pc);
	size = FETCH2U(pc);
	program = pc;
	lines = pc + size;

	while (program + this->pc != lines) {
	    Code *code;

	    /* add new code */
	    code = new Code(this);
	    code->next = NULL;
	    if (first == NULL) {
		first = last = code;
	    } else {
		last->next = code;
		last = code;
	    }
	}
    } else {
	program = lines = pc;
    }

    *prog = lines + lc;
}

/*
 * NAME:	Code->switch_int()
 * DESCRIPTION:	retrieve int switch
 */
CodeByte *Code::switchInt(CodeByte *pc)
{
    CaseInt *caseInt;
    int i, bytes;

    size = FETCH2U(pc);
    this->caseInt = caseInt = new CaseInt[i = size];
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
CodeByte *Code::switchRange(CodeByte *pc)
{
    CaseRange *caseRange;
    int i, bytes;

    size = FETCH2U(pc);
    this->caseRange = caseRange = new CaseRange[i = size];
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
CodeByte *Code::switchStr(CodeByte *pc, CodeContext *context)
{
    CaseString *caseString;
    int i;

    size = FETCH2U(pc);
    this->caseString = caseString = new CaseString[i = size];

    /* default */
    caseString->addr = FETCH2U(pc);
    caseString++;
    --i;

    /* nil */
    if (FETCH1U(pc) == 0) {
	caseString->str.inherit = 0;
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

Code::Code(CodeFunction *function)
{
    CodeContext *context;
    CodeByte *pc, *numbers, instr;
    LPCFloat xfloat;
    uint16_t offset, fexp;
    int i;
    CodeContext::Kfun *kf;

    context = function->object->context;
    pc = function->program + function->pc;

    /*
     * retrieve instruction
     */
    addr = function->pc;
    pop = false;
    instr = FETCH1U(pc);

    /*
     * retrieve line number
     */
    numbers = function->lines + function->lc;
    line = function->line;
    offset = instr >> I_LINE_SHIFT;
    if (offset <= 2) {
	line += offset;
    } else {
	offset = FETCH1U(numbers);
	if (offset >= 128) {
	    line += offset - 128 - 64;
	} else {
	    line += ((offset << 8) | FETCH1U(numbers)) - 16384;
	}
    }
    function->line = line;
    function->lc = numbers - function->lines;

    /*
     * process instruction
     */
    switch (instr & I_INSTR_MASK) {
    case I_INT1:
	num = FETCH1S(pc);
	instruction = INT;
	break;

    case I_INT2:
	num = FETCH2S(pc);
	instruction = INT;
	break;

    case I_INT4:
	num = FETCH4S(pc);
	instruction = INT;
	break;

    case I_INT8:
	num = FETCH8S(pc);
	instruction = INT;
	break;

    case I_FLOAT6:
	xfloat.high = FETCH2U(pc);
	xfloat.low = FETCH4U(pc);
	fexp = (xfloat.high & ~0x8000) >> 4;
	if (fexp == 0) {
	    flt.high = 0;
	    flt.low = 0;
	} else {
	    flt.high = (((xfloat.high & 0x8000) + fexp + 0x3c00) << 16) +
		       ((xfloat.high & 0xf) << 12) + (xfloat.low >> 20);
	    flt.low = xfloat.low << 44;
	}
	instruction = FLOAT;
	break;

    case I_FLOAT12:
	flt.high = FETCH4U(pc);
	flt.low = FETCH8U(pc);
	instruction = FLOAT;
	break;

    case I_STRING:
	str.inherit = function->object->nInherits - 1;
	str.index = FETCH1U(pc);
	instruction = STRING;
	break;

    case I_NEAR_STRING:
	str.inherit = FETCH1U(pc);
	str.index = FETCH1U(pc);
	instruction = STRING;
	break;

    case I_FAR_STRING:
	str.inherit = FETCHI(pc, context);
	str.index = FETCH2U(pc);
	instruction = STRING;
	break;

    case I_LOCAL:
	i = FETCH1S(pc);
	if (i < 0) {
	    local = -(i + 1);
	    instruction = LOCAL;
	} else {
	    param = i;
	    instruction = PARAM;
	}
	break;

    case I_GLOBAL:
	var.inherit = function->object->nInherits - 1;
	var.index = FETCH1U(pc);
	var.type = function->object->varTypes[var.inherit][var.index];
	instruction = GLOBAL;
	break;

    case I_FAR_GLOBAL:
	var.inherit = FETCHI(pc, context);
	var.index = FETCH1U(pc);
	var.type = function->object->varTypes[var.inherit][var.index];
	instruction = GLOBAL;
	break;

    case I_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_INDEX:
	instruction = INDEX;
	break;

    case I_INDEX2:
	instruction = INDEX2;
	break;

    case I_AGGREGATE | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_AGGREGATE:
	instruction = (FETCH1U(pc) == 0) ? AGGREGATE : MAP_AGGREGATE;
	size = FETCH2U(pc);
	break;

    case I_SPREAD:
	spread = FETCH1S(pc);
	if (spread >= 0) {
	    instruction = SPREAD;
	} else {
	    spread = -(spread + 2);
	    instruction = SPREAD_STORES;
	}
	break;

    case I_CAST | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CAST:
	pc = context->type(pc, &type);
	instruction = CAST;
	break;

    case I_INSTANCEOF | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_INSTANCEOF:
	str.inherit = FETCHI(pc, context);
	str.index = FETCH2U(pc);
	instruction = INSTANCEOF;
	break;

    case I_STORES:
	size = FETCH1U(pc);
	instruction = STORES;
	break;

    case I_STORE_LOCAL | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_LOCAL:
	i = FETCH1S(pc);
	if (i < 0) {
	    local = -(i + 1);
	    instruction = STORE_LOCAL;
	} else {
	    param = i;
	    instruction = STORE_PARAM;
	}
	break;

    case I_STORE_GLOBAL | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_GLOBAL:
	var.inherit = function->object->nInherits - 1;
	var.index = FETCH1U(pc);
	instruction = STORE_GLOBAL;
	break;

    case I_STORE_FAR_GLOBAL | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_FAR_GLOBAL:
	var.inherit = FETCHI(pc, context);
	var.index = FETCH1U(pc);
	instruction = STORE_GLOBAL;
	break;

    case I_STORE_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_INDEX:
	instruction = STORE_INDEX;
	break;

    case I_STORE_LOCAL_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_LOCAL_INDEX:
	i = FETCH1S(pc);
	if (i < 0) {
	    local = -(i + 1);
	    instruction = STORE_LOCAL_INDEX;
	} else {
	    param = i;
	    instruction = STORE_PARAM_INDEX;
	}
	break;

    case I_STORE_GLOBAL_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_GLOBAL_INDEX:
	var.inherit = function->object->nInherits - 1;
	var.index = FETCH1U(pc);
	instruction = STORE_GLOBAL_INDEX;
	break;

    case I_STORE_FAR_GLOBAL_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_FAR_GLOBAL_INDEX:
	var.inherit = FETCHI(pc, context);
	var.index = FETCH1U(pc);
	instruction = STORE_GLOBAL_INDEX;
	break;

    case I_STORE_INDEX_INDEX | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_STORE_INDEX_INDEX:
	instruction = STORE_INDEX_INDEX;
	break;

    case I_JUMP_ZERO:
	pop = true;
	target = FETCH2U(pc);
	instruction = JUMP_ZERO;
	break;

    case I_JUMP_NONZERO:
	pop = true;
	target = FETCH2U(pc);
	instruction = JUMP_NONZERO;
	break;

    case I_JUMP:
	target = FETCH2U(pc);
	instruction = JUMP;
	break;

    case I_SWITCH:
	pop = true;
	switch (FETCH1U(pc)) {
	case I_SWITCH_INT:
	    pc = switchInt(pc);
	    instruction = SWITCH_INT;
	    break;

	case I_SWITCH_RANGE:
	    pc = switchRange(pc);
	    instruction = SWITCH_RANGE;
	    break;

	case I_SWITCH_STRING:
	    pc = switchStr(pc, context);
	    instruction = SWITCH_STRING;
	    break;
	}
	break;

    case I_CALL_KFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_KFUNC:
	kfun.func = context->map[FETCH1U(pc)];
	switch (kfun.func) {
	case KF_CKRANGEFT:
	    instruction = CHECK_RANGE;
	    break;

	case KF_CKRANGEF:
	    instruction = CHECK_RANGE_FROM;
	    break;

	case KF_CKRANGET:
	    instruction = CHECK_RANGE_TO;
	    break;

	default:
	    kf = &context->kfuns[kfun.func];
	    if (kf->vargs != 0) {
		kfun.nargs = FETCH1U(pc);
	    } else {
		kfun.nargs = kf->nargs;
	    }
	    kfun.type = kf->proto[0].type;
	    instruction = (kf->lval) ? KFUNC_STORES : KFUNC;
	    break;
	}
	break;

    case I_CALL_EFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_EFUNC:
	kfun.func = context->map[FETCH2U(pc)];
	kf = &context->kfuns[kfun.func];
	if (kf->vargs != 0) {
	    kfun.nargs = FETCH1U(pc);
	} else {
	    kfun.nargs = kf->nargs;
	}
	kfun.type = kf->proto[0].type;
	instruction = (kf->lval) ? KFUNC_STORES : KFUNC;
	break;

    case I_CALL_CKFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_CKFUNC:
	kfun.func = context->map[FETCH1U(pc)];
	kfun.nargs = FETCH1U(pc);
	kf = &context->kfuns[kfun.func];
	kfun.type = kf->proto[0].type;
	instruction = (kf->lval) ? KFUNC_STORES : KFUNC;
	break;

    case I_CALL_CEFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_CEFUNC:
	kfun.func = context->map[FETCH2U(pc)];
	kfun.nargs = FETCH1U(pc);
	kf = &context->kfuns[kfun.func];
	kfun.type = kf->proto[0].type;
	instruction = (kf->lval) ? KFUNC_STORES : KFUNC;
	break;

    case I_CALL_AFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_AFUNC:
	dfun.inherit = 0;
	dfun.func = FETCH1U(pc);
	dfun.nargs = FETCH1U(pc);
	dfun.type = function->object->funcTypes[0][dfun.func];
	instruction = DFUNC;
	break;

    case I_CALL_DFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_DFUNC:
	dfun.inherit = FETCHI(pc, context);
	dfun.func = FETCH1U(pc);
	dfun.nargs = FETCH1U(pc);
	dfun.type = function->object->funcTypes[dfun.inherit][dfun.func];
	instruction = DFUNC;
	break;

    case I_CALL_FUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_FUNC:
	fun.call = FETCH2U(pc);
	fun.nargs = FETCH1U(pc);
	instruction = FUNC;
	break;

    case I_CATCH | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CATCH:
	target = FETCH2U(pc);
	instruction = CATCH;
	break;

    case I_RLIMITS:
	pop = true;
	instruction = (FETCH1U(pc) != 0) ? RLIMITS : RLIMITS_CHECK;
	break;

    case I_RETURN:
	instruction = RETURN;
	break;

    default:
	fatal("retrieved unknown instruction 0x%02x", instr);
    }
    function->pc = pc - function->program;
}

/*
 * NAME:	Code->remove()
 * DESCRIPTION:	remove retrieved code
 */
Code::~Code()
{
    switch (instruction) {
    case SWITCH_INT:
	delete caseInt;
	break;

    case SWITCH_RANGE:
	delete caseRange;
	break;

    case SWITCH_STRING:
	delete caseString;
	break;

    default:
	break;
    }
}

/*
 * NAME:	Code->del()
 * DESCRIPTION:	delete a function retriever
 */
CodeFunction::~CodeFunction()
{
    Code *code, *next;

    /* delete all code retrieved for this function */
    for (code = first; code != NULL; code = next) {
	next = code->next;
	delete code;
    }

    delete[] proto;
}
