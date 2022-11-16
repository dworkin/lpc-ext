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
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "jitcomp.h"


TypedContext::TypedContext(CodeFunction *func, StackSize size) :
    BlockContext()
{
    LPCParam i;

    /*
     * construct TypedContext from function and expected stack space
     */
    nParams = func->nargs + func->vargs;
    if (nParams != 0) {
	params = new Type[nParams];

	for (i = 1; i <= nParams; i++) {
	    params[nParams - i] = func->proto[i].type;
	}
	if (func->fclass & CodeFunction::CLASS_ELLIPSIS) {
	    params[0] = LPC_TYPE_ARRAY;
	}
    } else {
	params = NULL;
    }

    nLocals = func->locals;
    if (nLocals != 0) {
	locals = new Type[nLocals];

	for (i = 0; i < nLocals; i++) {
	    locals[i] = LPC_TYPE_NIL;
	}
    } else {
	locals = NULL;
    }

    stack = new Stack<TVC>((size * 3) / 2);
    altSp = sp = STACK_EMPTY;
    castType = LPC_TYPE_MIXED;
    spreadArgs = false;
    merging = false;
}

TypedContext::~TypedContext()
{
    delete[] params;
    delete[] locals;
    delete stack;
}

/*
 * merge two types
 */
Type TypedContext::mergeType(Type type1, Type type2)
{
    if (type1 != LPC_TYPE_MIXED && type1 != type2) {
	if (type1 == LPC_TYPE_NIL && type2 >= LPC_TYPE_STRING) {
	    return type2;
	}
	if (type2 == LPC_TYPE_NIL && type1 >= LPC_TYPE_STRING) {
	    return type1;
	}
	return LPC_TYPE_MIXED;
    }

    return type1;
}

/*
 * copy stack
 */
StackSize TypedContext::copyStack(StackSize copy, StackSize from, StackSize to)
{
    StackSize sp;

    for (sp = from; sp != to; sp = stack->pop(sp)) {
	copy = stack->push(copy, TVC(LPC_TYPE_VOID, 0));
    }
    for (sp = copy; from != to; from = stack->pop(from)) {
	TVC val = stack->get(from);
	val.ref = sp;
	stack->set(from, val);
	val.ref = STACK_EMPTY;
	stack->set(sp, val);
	sp = stack->pop(sp);
    }

    return copy;
}

/*
 * prepare before evaluating a block
 */
void TypedContext::prologue(Type *mergeParams, Type *mergeLocals,
			    StackSize mergeSp, Block *b)
{
    StackSize sp;

    memcpy(params, mergeParams, nParams);
    memcpy(locals, mergeLocals, nLocals);

    if (b->sp == STACK_INVALID) {
	if (b->nFrom > 1) {
	    /*
	     * copy stack from mergeSp
	     */
	    b->sp = copyStack(STACK_EMPTY, mergeSp, STACK_EMPTY);
	} else {
	    b->sp = mergeSp;
	}

	merging = false;
    } else if (b->nFrom > 1) {
	/*
	 * merge stacks before evaluating block
	 */
	for (sp = b->sp; sp != STACK_EMPTY; sp = stack->pop(sp)) {
	    TVC val = stack->get(mergeSp);
	    Type type = val.type;
	    val.ref = sp;
	    stack->set(mergeSp, val);
	    val = stack->get(sp);
	    val.type = mergeType(val.type, type);
	    stack->set(sp, val);

	    mergeSp = stack->pop(mergeSp);
	}

	merging = true;
    }

    this->sp = b->sp;
}

/*
 * push type on stack
 */
void TypedContext::push(TVC val)
{
    if (merging) {
	altStack[altSp++] = val;
    } else {
	sp = stack->push(sp, val);
    }
}

/*
 * pop type from stack
 */
TVC TypedContext::pop(Code *code)
{
    if (altSp != STACK_EMPTY) {
	return altStack[--altSp];
    } else if (sp == STACK_EMPTY) {
	fatal("pop empty stack");
    }

    TVC val = stack->get(sp);
    val.code = code;
    stack->set(sp, val);
    val.code = NULL;
    val.ref = STACK_EMPTY;
    sp = stack->pop(sp);
    return val;
}

/*
 * store a parameter
 */
void TypedContext::storeParam(LPCParam param, Type type)
{
    params[param] = type;
    if (caught != NULL && !caught->mod[param]) {
	caught->mod[param] = true;
	caught->toVisit(list);
    }
}

/*
 * store a local variable
 */
void TypedContext::storeLocal(LPCParam local, Type type)
{
    locals[local] = type;
    if (caught != NULL && !caught->mod[nParams + local]) {
	caught->mod[nParams + local] = true;
	caught->toVisit(list);
    }
}

/*
 * clean up after stores
 */
void TypedContext::endStores()
{
    castType = LPC_TYPE_MIXED;
    if (!storeN()) {
	if (storePop() != NULL) {
	    if (lval()) {
		pop(storePop());
	    }
	} else if (!lval()) {
	    push(LPC_TYPE_ARRAY);
	}
    }
}

/*
 * return indexed type on the stack, skipping index
 */
TVC TypedContext::indexed()
{
    return stack->get(stack->pop(sp));
}

/*
 * handle a kfun call and return the resulting type
 */
Type TypedContext::kfun(LPCKFunCall *kf, Code *code)
{
    int nargs, i;
    Type type, summand;
    LPCInt val;

    nargs = kf->nargs;
    if (kf->func == KF_SUM) {
	/*
	 * summand: check values on the stack to determine actual number
	 * of arguments to pop
	 */
	type = LPC_TYPE_VOID;
	while (nargs != 0) {
	    val = pop(code).val;
	    if (val < 0) {
		if (val <= SUM_AGGREGATE) {
		    /* aggregate */
		    for (i = SUM_AGGREGATE - (int) val; i != 0; --i) {
			pop(code);
		    }
		    summand = LPC_TYPE_ARRAY;
		} else if (val == SUM_SIMPLE) {
		    /* simple argument */
		    summand = pop(code).type;
		} else {
		    /* allocate array */
		    pop(code);
		    summand = LPC_TYPE_ARRAY;
		}
	    } else {
		/* subrange */
		pop(code);
		summand = pop(code).type;
	    }

	    if (type == LPC_TYPE_VOID) {
		type = summand;
	    } else if (type != summand) {
		if (type == LPC_TYPE_STRING || summand == LPC_TYPE_STRING) {
		    type = LPC_TYPE_STRING;
		} else if (type == LPC_TYPE_ARRAY || summand == LPC_TYPE_ARRAY)
		{
		    type = LPC_TYPE_ARRAY;
		} else {
		    type = LPC_TYPE_MIXED;
		}
	    }

	    --nargs;
	}
    } else {
	if (spreadArgs) {
	    --nargs;
	}
	if (kf->lval != 0) {
	    if (!merging) {
		StackSize from, to;

		/* skip lvalue arguments */
		from = sp;
		while (nargs > kf->lval) {
		    sp = stack->pop(sp);
		    --nargs;
		}
		to = sp;

		/* pop non-lvalue arguments */
		while (nargs > 0) {
		    pop(code);
		    --nargs;
		}

		/* push return value */
		push(TypedCode::simplifiedType(kf->type));

		/* copy lvalue arguments */
		sp = copyStack(sp, from, to);
	    }

	    type = LPC_TYPE_ARRAY;
	} else {
	    while (nargs != 0) {
		pop(code);
		--nargs;
	    }

	    type = kf->type;
	}
    }

    spreadArgs = false;
    return type;
}

/*
 * pop function arguments
 */
void TypedContext::args(int nargs, Code *code)
{
    if (spreadArgs) {
	--nargs;
    }
    while (nargs != 0) {
	pop(code);
	--nargs;
    }
    spreadArgs = false;
}

/*
 * prepare modification flags for caught context
 */
void TypedContext::startCatch()
{
    if (block->next->mod == NULL && nParams + nLocals != 0) {
	block->next->initMod(nParams + nLocals);
    }
}

/*
 * Handle caught by setting modified parameter/variable types to LPC_TYPE_MIXED.
 */
void TypedContext::modCaught()
{
    LPCParam i, j;

    for (i = 0; i < nParams; i++) {
	if (block->mod[i]) {
	    params[i] = LPC_TYPE_MIXED;
	}
    }
    for (j = 0; j < nLocals; i++, j++) {
	if (block->mod[i]) {
	    locals[j] = LPC_TYPE_MIXED;
	}
    }
}

/*
 * copy modifications to enclosing caught context
 */
void TypedContext::endCatch()
{
    LPCParam i;

    if (caught->caught != NULL) {
	for (i = 0; i < nParams + nLocals; i++) {
	    if (caught->mod[i] && !caught->caught->mod[i]) {
		caught->caught->mod[i] = true;
		caught->caught->toVisit(list);
	    }
	}
    }
    caught = caught->caught;
}

/*
 * merge stacks after evaluating code
 */
StackSize TypedContext::merge(StackSize codeSp)
{
    if (codeSp != STACK_INVALID) {
	sp = codeSp;

	while (altSp != STACK_EMPTY) {
	    TVC val = stack->get(codeSp);
	    val.type = mergeType(val.type, altStack[altSp - 1].type);
	    stack->set(codeSp, val);

	    codeSp = stack->pop(codeSp);
	    --altSp;
	}
    }

    return sp;
}

/*
 * propagate changes to following blocks?
 */
bool TypedContext::changed(Type *params, Type *locals)
{
    LPCParam i;

    for (i = 0; i < nParams; i++) {
	this->params[i] = mergeType(params[i], this->params[i]);
    }
    for (i = 0; i < nLocals; i++) {
	this->locals[i] = mergeType(locals[i], this->locals[i]);
    }

    return (sp != STACK_EMPTY ||
	    memcmp(params, this->params, nParams) != 0 ||
	    memcmp(locals, this->locals, nLocals) != 0);
}

/*
 * retrieve a TVC
 */
TVC TypedContext::get(StackSize stackPointer)
{
    return stack->get(stackPointer);
}

/*
 * find the Code that consumes a type/value
 */
Code *TypedContext::consumer(StackSize stackPointer, Type *type)
{
    while (stackPointer != STACK_EMPTY) {
	TVC val = stack->get(stackPointer);
	if (val.code != NULL) {
	    *type = val.type;
	    return val.code;
	}
	stackPointer = val.ref;
    }

    return NULL;
}

/*
 * find the next item on the stack
 */
StackSize TypedContext::nextSp(StackSize stackPointer, int depth)
{
    while (depth != 0) {
	stackPointer = stack->pop(stackPointer);
	--depth;
    }

    return stackPointer;
}


TypedCode::TypedCode(CodeFunction *function) :
    Code(function)
{
    sp = STACK_INVALID;
}

TypedCode::~TypedCode()
{
}

/*
 * simplify type
 */
Type TypedCode::simplifiedType(Type type)
{
    return (LPC_TYPE_REF(type) != 0) ? LPC_TYPE_ARRAY : type;
}

/*
 * return a type if a value should be kept off the stack, LPC_TYPE_NIL otherwise
 */
Type TypedCode::offStack(TypedContext *context, StackSize stackPointer)
{
    Code *code;
    Type type;

    code = context->consumer(stackPointer, &type);
    if (code == NULL) {
	return LPC_TYPE_NIL;
    }

    switch (code->instruction) {
    case STORE_PARAM:
    case STORE_LOCAL:
    case STORE_GLOBAL:
	if (type == LPC_TYPE_INT || type == LPC_TYPE_FLOAT) {
	    return type;
	}
	break;

    case JUMP_ZERO:
    case JUMP_NONZERO:
    case SWITCH_INT:
    case SWITCH_RANGE:
	if (type == LPC_TYPE_INT) {
	    return type;
	}
	break;

    case KFUNC:
	switch (code->kfun.func) {
	case KF_ADD_INT:
	case KF_ADD1_INT:
	case KF_AND_INT:
	case KF_DIV_INT:
	case KF_EQ_INT:
	case KF_GE_INT:
	case KF_GT_INT:
	case KF_LE_INT:
	case KF_LSHIFT_INT:
	case KF_LT_INT:
	case KF_MOD_INT:
	case KF_MULT_INT:
	case KF_NE_INT:
	case KF_NEG_INT:
	case KF_NOT_INT:
	case KF_OR_INT:
	case KF_RSHIFT_INT:
	case KF_SUB_INT:
	case KF_SUB1_INT:
	case KF_TST_INT:
	case KF_UMIN_INT:
	case KF_XOR_INT:
	    return LPC_TYPE_INT;

	case KF_ADD_FLT:
	case KF_ADD1_FLT:
	case KF_DIV_FLT:
	case KF_EQ_FLT:
	case KF_GE_FLT:
	case KF_GT_FLT:
	case KF_LE_FLT:
	case KF_LT_FLT:
	case KF_MULT_FLT:
	case KF_NE_FLT:
	case KF_NOT_FLT:
	case KF_SUB_FLT:
	case KF_SUB1_FLT:
	case KF_TST_FLT:
	case KF_UMIN_FLT:
	case KF_FABS:
	case KF_FLOOR:
	case KF_CEIL:
	case KF_FMOD:
	case KF_EXP:
	case KF_LOG:
	case KF_LOG10:
	case KF_POW:
	case KF_SQRT:
	case KF_COS:
	case KF_SIN:
	case KF_TAN:
	case KF_ACOS:
	case KF_ASIN:
	case KF_ATAN:
	case KF_ATAN2:
	case KF_COSH:
	case KF_SINH:
	case KF_TANH:
	    return LPC_TYPE_FLOAT;

	case KF_TOFLOAT:
	case KF_TOINT:
	case KF_LDEXP:
	    if (type == LPC_TYPE_INT || type == LPC_TYPE_FLOAT) {
		return type;
	    }
	    break;
	}
	break;

    default:
	break;
    }

    return LPC_TYPE_NIL;
}

/*
 * evaluate type changes
 */
void TypedCode::evaluateTypes(TypedContext *context)
{
    TVC val;
    CodeSize i;

    switch (instruction) {
    case INT:
	context->push(LPC_TYPE_INT, num);
	break;

    case FLOAT:
	context->push(LPC_TYPE_FLOAT);
	break;

    case STRING:
	context->push(LPC_TYPE_STRING);
	break;

    case PARAM:
	context->push(simplifiedType(context->params[param]));
	break;

    case LOCAL:
	context->push(context->locals[local]);
	break;

    case GLOBAL:
	context->push(simplifiedType(var.type));
	break;

    case INDEX:
	val = context->indexed();
	context->pop(this);
	context->pop(this);
	context->push((val.type == LPC_TYPE_STRING) ?
		       LPC_TYPE_INT : LPC_TYPE_MIXED);
	break;

    case INDEX2:
	context->push((context->indexed().type == LPC_TYPE_STRING) ?
		       LPC_TYPE_INT : LPC_TYPE_MIXED);
	break;

    case SPREAD:
    case SPREAD_LVAL:
	/* assume array of size 0 spread */
	context->pop(this);
	context->spread();
	break;

    case AGGREGATE:
	for (i = size; i != 0; --i) {
	    context->pop(this);
	}
	context->push(LPC_TYPE_ARRAY);
	break;

    case MAP_AGGREGATE:
	for (i = size; i != 0; --i) {
	    context->pop(this);
	}
	context->push(LPC_TYPE_MAPPING);
	break;

    case CAST:
	context->pop(this);
	context->push(simplifiedType(type.type));
	break;

    case INSTANCEOF:
	context->pop(this);
	context->push(LPC_TYPE_INT);
	break;

    case CHECK_RANGE:
	break;

    case CHECK_RANGE_FROM:
	context->push(LPC_TYPE_INT);
	break;

    case CHECK_RANGE_TO:
	val = context->pop(this);
	context->push(LPC_TYPE_INT);
	context->push(val);
	break;

    case STORE_PARAM:
	val = context->pop(this);
	context->storeParam(param, val.type);
	context->push(val);
	break;

    case STORE_LOCAL:
	val = context->pop(this);
	context->storeLocal(local, val.type);
	context->push(val);
	break;

    case STORE_GLOBAL:
	context->push(context->pop(this));
	break;

    case STORE_PARAM_INDEX:
    case STORE_LOCAL_INDEX:
    case STORE_INDEX:
    case STORE_GLOBAL_INDEX:
	val = context->pop(this);
	context->pop(this);
	context->pop(this);
	context->push(val);
	break;

    case STORE_INDEX_INDEX:
	val = context->pop(this);
	context->pop(this);
	context->pop(this);
	context->pop(this);
	context->pop(this);
	context->push(val);
	break;

    case STORES:
	context->pop(this);
	sp = context->merge(sp);
	if (!context->stores(size, (pop) ? this : NULL, false) && !pop) {
	    context->push(LPC_TYPE_ARRAY);
	}
	return;

    case STORES_LVAL:
	context->pop(this);
	sp = context->merge(sp);
	if (!context->stores(size, (pop) ? this : NULL, true) && pop) {
	    context->pop(this);
	}
	return;

    case STORES_SPREAD:
	sp = context->merge(sp);
	context->endStores();
	return;

    case STORES_CAST:
	context->castType = simplifiedType(type.type);
	break;

    case STORES_PARAM:
	varType = (context->lval() &&
				context->params[param] != context->castType) ?
		   LPC_TYPE_MIXED : context->castType;
	context->storeParam(param, varType);
	sp = context->merge(sp);
	context->endStores();
	return;

    case STORES_LOCAL:
	varType = (context->lval() &&
				context->locals[local] != context->castType) ?
		   LPC_TYPE_MIXED : context->castType;
	context->storeLocal(local, varType);
	sp = context->merge(sp);
	context->endStores();
	return;

    case STORES_GLOBAL:
	sp = context->merge(sp);
	context->endStores();
	return;

    case STORES_PARAM_INDEX:
    case STORES_LOCAL_INDEX:
    case STORES_INDEX:
    case STORES_GLOBAL_INDEX:
	context->pop(this);
	context->pop(this);
	sp = context->merge(sp);
	context->endStores();
	return;

    case STORES_INDEX_INDEX:
	context->pop(this);
	context->pop(this);
	context->pop(this);
	context->pop(this);
	sp = context->merge(sp);
	context->endStores();
	return;

    case JUMP:
	break;

    case JUMP_ZERO:
    case JUMP_NONZERO:
    case SWITCH_INT:
    case SWITCH_RANGE:
    case SWITCH_STRING:
	sp = context->merge(sp);
	context->pop(this);
	return;

    case KFUNC:
    case KFUNC_LVAL:
    case KFUNC_SPREAD:
    case KFUNC_SPREAD_LVAL:
	context->push(simplifiedType(context->kfun(&kfun, this)));
	break;

    case DFUNC:
    case DFUNC_SPREAD:
	context->args(dfun.nargs, this);
	context->push(simplifiedType(dfun.type));
	break;

    case FUNC:
    case FUNC_SPREAD:
	context->args(fun.nargs, this);
	context->push(LPC_TYPE_MIXED);
	break;

    case CATCH:
	context->startCatch();
	break;

    case CAUGHT:
	context->modCaught();
	context->push(LPC_TYPE_STRING);
	break;

    case END_CATCH:
	context->endCatch();
	break;

    case RLIMITS:
    case RLIMITS_CHECK:
	context->pop(this);
	context->pop(this);
	break;

    case END_RLIMITS:
	break;

    case RETURN:
	context->pop(this);
	if (context->sp != STACK_EMPTY) {
	    fatal("stack not empty");
	}
	break;

    default:
	fatal("unknown instruction");
    }

    sp = context->merge(sp);

    if (pop) {
	context->pop(NULL);
    }
}


TypedBlock::TypedBlock(Code *first, Code *last, CodeSize size) :
    Block(first, last, size)
{
    params = locals = NULL;
}

TypedBlock::~TypedBlock()
{
    delete[] params;
    delete[] locals;
}

/*
 * type of a parameter at the end of the block
 */
Type TypedBlock::paramType(LPCParam param)
{
    return params[param];
}

/*
 * type of a local var at the end of the block
 */
Type TypedBlock::localType(LPCLocal local)
{
    return locals[local];
}

/*
 * prepare a context with params, locals and stack from this block
 */
void TypedBlock::setContext(TypedContext *context, Block *b)
{
    context->prologue(params, locals, endSp, b);
}

/*
 * evaluate code in a block
 */
void TypedBlock::evaluateTypes(TypedContext *context, Block **list)
{
    Code *code;
    CodeSize i, j;
    Block *b;

    context->sp = sp;
    context->block = this;
    context->caught = caught;
    context->list = list;

    for (code = first; ; code = code->next) {
	code->evaluateTypes(context);
	if (code == last) {
	    break;
	}
    }

    if (endSp == STACK_INVALID) {
	/*
	 * initialize params & locals
	 */
	if (context->nParams != 0) {
	    params = new Type[context->nParams];
	}
	if (context->nLocals != 0) {
	    locals = new Type[context->nLocals];
	}
    } else if (!context->changed(params, locals)) {
	return;
    }

    /*
     * save state
     */
    endSp = context->sp;
    memcpy(params, context->params, context->nParams);
    memcpy(locals, context->locals, context->nLocals);

    /* followups */
    for (i = 0; i < nTo; i++) {
	b = to[i];

	/* find proper from */
	for (j = 0; b->from[j] != this; j++) ;

	if (!b->fromVisit[j]) {
	    b->fromVisit[j] = true;
	    if (b != this) {
		b->toVisit(list);
	    }
	}
    }
}

/*
 * evaluate all blocks
 */
void TypedBlock::evaluate(TypedContext *context)
{
    Block *list, *b;
    StackSize sp;
    CodeSize i;

    /*
     * evaluate types
     */
    startVisits(&list);
    sp = STACK_EMPTY;
    for (b = next; b != NULL; b = b->next) {
	b->sp = STACK_INVALID;
    }

    /* eval this block */
    evaluateTypes(context, &list);

    for (b = this; b != NULL; ) {
	for (i = 0; ; i++) {
	    if (i == b->nFrom) {
		b = nextVisit(&list);
		break;
	    }
	    if (b->fromVisit[i]) {
		b->fromVisit[i] = false;
		b->from[i]->setContext(context, b);
		b->evaluateTypes(context, &list);
		break;
	    }
	}
    }
}
