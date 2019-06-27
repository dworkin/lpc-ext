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

# define FETCHI(pc, cc) (((cc)->inhSize > sizeof(char)) ? \
			  FETCH2U(pc) : FETCH1U(pc))

CodeContext::CodeContext(size_t intSize, size_t inhSize, CodeByte *protos,
			 int nBuiltins, int nKfuns)
{
    int i, size;
    Kfun *kfun;
    LPCType *proto;

    /* allocate code context */
    this->intSize = intSize;
    this->inhSize = inhSize;

    /*
     * initialize kfun prototype table
     */
    kfuns = new Kfun[nkfun = KF_BUILTINS + nKfuns];
    for (kfun = kfuns, i = nBuiltins; i > 0; kfun++, --i) {
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
    for (i = nBuiltins; i < KF_BUILTINS; kfun++, i++) {
	kfun->proto = NULL;
    }
    for (i = nKfuns; i > 0; kfun++, --i) {
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

/*
 * check VM version
 */
bool CodeContext::validVM(int major, int minor)
{
    return (major == VERSION_VM_MAJOR && minor <= VERSION_VM_MINOR);
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
 * return variable type
 */
Type CodeObject::varType(LPCGlobal *var)
{
    return varTypes[var->inherit][var->index];
}

/*
 * return function type
 */
Type CodeObject::funcType(LPCDFunCall *func)
{
    return funcTypes[func->inherit][func->func];
}


/*
 * create a function
 */
CodeFunction::CodeFunction(CodeObject *object, CodeByte *prog)
{
    CodeContext *context;
    uint16_t size;
    LPCType *proto;

    this->object = object;
    context = object->context;

    /* retrieve prototype */
    program = prog;
    nargs = PROTO_NARGS(program);
    vargs = PROTO_VARGS(program);
    size = nargs + vargs + 1;
    this->proto = proto = new LPCType[size];
    fclass = PROTO_CLASS(program);
    program = &PROTO_FTYPE(program);
    do {
	program = context->type(program, proto++);
    } while (--size != 0);

    /* retrieve code from function */
    this->pc = lc = 0;
    line = 0;
    if (!(fclass & CLASS_UNDEFINED)) {
	stack = FETCH2U(program);
	locals = FETCH1U(program);
	size = FETCH2U(program);
	lines = program + size;
    } else {
	stack = locals = size = 0;
	lines = program;
    }
}

/*
 * delete a function retriever
 */
CodeFunction::~CodeFunction()
{
    delete[] proto;
}

/*
 * get the current program
 */
CodeByte *CodeFunction::getPC(CodeSize *addr)
{
    *addr = pc;
    return program + pc;
}

/*
 * set the current program
 */
void CodeFunction::setPC(CodeByte *pc)
{
    this->pc = pc - program;
}

/*
 * get the current line from a function
 */
CodeLine CodeFunction::getLine(CodeByte instr)
{
    uint16_t offset;
    CodeByte *numbers;

    offset = instr >> I_LINE_SHIFT;
    if (offset <= 2) {
	line += offset;
    } else {
	numbers = lines + lc;
	offset = FETCH1U(numbers);
	if (offset >= 128) {
	    line += offset - 128 - 64;
	} else {
	    line += ((offset << 8) | FETCH1U(numbers)) - 16384;
	}
	lc = numbers - lines;
    }

    return line;
}

CodeByte *CodeFunction::endProg()
{
    return (program + pc == lines) ? lines + lc : NULL;
}


/*
 * retrieve int switch
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
 * retrieve range switch
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
 * retrieve string switch
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
    CodeByte *pc, instr;
    LPCFloat xfloat;
    uint16_t fexp;
    int i;
    CodeContext::Kfun *kf;

    this->function = function;
    context = function->object->context;
    pc = function->getPC(&addr);

    /*
     * retrieve instruction
     */
    pop = false;
    instr = FETCH1U(pc);

    /*
     * retrieve line number
     */
    line = function->getLine(instr);

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
	var.type = function->object->varType(&var);
	instruction = GLOBAL;
	break;

    case I_FAR_GLOBAL:
	var.inherit = FETCHI(pc, context);
	var.index = FETCH1U(pc);
	var.type = function->object->varType(&var);
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
	instruction = SPREAD;
	spread = FETCH1S(pc);
	if (spread >= 0) {
	    pc = context->type(pc, &type);
	} else {
	    spread = -(spread + 2);
	    if (spread >= 0) {
		instruction = SPREAD_LVAL;
	    }
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

    case I_STORES | I_POP_BIT:
	pop = true;
	/* fall through */
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
	kfun.func = FETCH1U(pc);
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
	    if (kf->lval) {
		kfun.lval = kf->nargs;
		instruction = KFUNC_LVAL;
	    } else {
		kfun.lval = 0;
		instruction = KFUNC;
	    }
	    break;
	}
	break;

    case I_CALL_EFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_EFUNC:
	kfun.func = FETCH2U(pc);
	kf = &context->kfuns[kfun.func];
	if (kf->vargs != 0) {
	    kfun.nargs = FETCH1U(pc);
	} else {
	    kfun.nargs = kf->nargs;
	}
	kfun.type = kf->proto[0].type;
	if (kf->lval) {
	    kfun.lval = kf->nargs;
	    instruction = KFUNC_LVAL;
	} else {
	    kfun.lval = 0;
	    instruction = KFUNC;
	}
	break;

    case I_CALL_CKFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_CKFUNC:
	kfun.func = FETCH1U(pc);
	kfun.nargs = FETCH1U(pc);
	kf = &context->kfuns[kfun.func];
	kfun.type = kf->proto[0].type;
	instruction = KFUNC;
	break;

    case I_CALL_CEFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_CEFUNC:
	kfun.func = FETCH2U(pc);
	kfun.nargs = FETCH1U(pc);
	kf = &context->kfuns[kfun.func];
	kfun.type = kf->proto[0].type;
	instruction = KFUNC;
	break;

    case I_CALL_AFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_AFUNC:
	dfun.inherit = 0;
	dfun.func = FETCH1U(pc);
	dfun.nargs = FETCH1U(pc);
	dfun.type = function->object->funcType(&dfun);
	instruction = DFUNC;
	break;

    case I_CALL_DFUNC | I_POP_BIT:
	pop = true;
	/* fall through */
    case I_CALL_DFUNC:
	dfun.inherit = FETCHI(pc, context);
	dfun.func = FETCH1U(pc);
	dfun.nargs = FETCH1U(pc);
	dfun.type = function->object->funcType(&dfun);
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
	instruction = (FETCH1U(pc) != 0) ? RLIMITS : RLIMITS_CHECK;
	break;

    case I_RETURN:
	instruction = RETURN;
	break;

    default:
	fatal("retrieved unknown instruction 0x%02x", instr);
    }
    function->setPC(pc);
}

/*
 * remove retrieved code
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

void Code::evaluateTypes(class BlockContext *context)
{
}

void Code::evaluateFlow(class FlowContext *context)
{
}

void Code::emit(class GenContext *context)
{
}

/*
 * produce code
 */
Code *Code::create(CodeFunction *function)
{
    return new Code(function);
}

Code *(*Code::factory)(CodeFunction*) = create;

/*
 * set the factory which produces code
 */
void Code::producer(Code *(*factory)(CodeFunction*))
{
    Code::factory = factory;
}

/*
 * let the factory produce code
 */
Code *Code::produce(CodeFunction *function)
{
    return (*factory)(function);
}
