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


BlockContext::BlockContext(CodeFunction *func, StackSize size)
{
    LPCParam i;

    /*
     * construct BlockContext from function and expected stack space
     */
    nParams = func->nargs + func->vargs;
    if (nParams != 0) {
	params = new Type[nParams];
	origParams = new Type[nParams];

	for (i = 1; i <= nParams; i++) {
	    params[nParams - i] = func->proto[i].type;
	}
	if (func->fclass & CodeFunction::CLASS_ELLIPSIS) {
	    params[0] = LPC_TYPE_ARRAY;
	}
	memcpy(origParams, params, nParams);
    } else {
	origParams = params = NULL;
    }

    nLocals = func->locals;
    if (nLocals != 0) {
	locals = new Type[nLocals];
	origLocals = new Type[nLocals];

	for (i = 0; i < nLocals; i++) {
	    locals[i] = LPC_TYPE_NIL;
	}
	memcpy(origLocals, locals, nLocals);
    } else {
	origLocals = locals = NULL;
    }

    stack = new Stack<TVC>(size);
    altSp = sp = STACK_EMPTY;
    storeCount = 0;
    storeCode = NULL;
    spreadArgs = false;
    merging = false;
}

BlockContext::~BlockContext()
{
    delete[] params;
    delete[] origParams;
    delete[] locals;
    delete[] origLocals;
    delete stack;
}

/*
 * merge two types
 */
Type BlockContext::mergeType(Type type1, Type type2)
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
 * prepare before evaluating a block
 */
void BlockContext::prologue(Type *mergeParams, Type *mergeLocals,
			    StackSize mergeSp, Block *b)
{
    StackSize sp;

    memcpy(params, mergeParams, nParams);
    memcpy(origParams, mergeParams, nParams);
    memcpy(locals, mergeLocals, nLocals);
    memcpy(origLocals, mergeLocals, nLocals);

    if (b->sp == STACK_INVALID) {
	/*
	 * copy stack from mergeSp
	 */
	b->sp = STACK_EMPTY;
	for (sp = mergeSp; sp != STACK_EMPTY; sp = stack->pop(sp)) {
	    b->sp = stack->push(b->sp, TVC(LPC_TYPE_VOID, 0));
	}
	for (sp = b->sp; sp != STACK_EMPTY; sp = stack->pop(sp)) {
	    TVC val = stack->get(mergeSp);
	    val.merge = sp;
	    stack->set(mergeSp, val);
	    val.merge = STACK_EMPTY;
	    stack->set(sp, val);
	    mergeSp = stack->pop(mergeSp);
	}

	merging = false;
    } else {
	/*
	 * merge stacks before evaluating block
	 */
	for (sp = b->sp; sp != STACK_EMPTY; sp = stack->pop(sp)) {
	    TVC val = stack->get(mergeSp);
	    Type type = val.type;
	    val.merge = sp;
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
void BlockContext::push(TVC val)
{
    if (merging) {
	altStack[altSp++] = val;
    } else {
	sp = stack->push(sp, val);
    }
}

/*
 * return top of stack
 */
TVC BlockContext::top()
{
    if (altSp != STACK_EMPTY) {
	return altStack[altSp - 1];
    } else {
	return stack->get(sp);
    }
}

/*
 * pop type from stack
 */
TVC BlockContext::pop(Code *code)
{
    if (altSp != STACK_EMPTY) {
	return altStack[--altSp];
    } else if (sp == STACK_EMPTY) {
	fatal("pop empty stack");
    }

    TVC val = stack->get(sp);
    val.code = code;
    stack->set(sp, val);
    sp = stack->pop(sp);
    return val;
}

/*
 * prepare for N stores
 */
void BlockContext::stores(int count, Code *popCode)
{
    storeCount = count;
    storeCode = popCode;
    if (count == 0 && popCode != NULL) {
	this->pop(popCode);
    }
}

/*
 * handle store N
 */
void BlockContext::storeN()
{
    if (--storeCount == 0 && storeCode != NULL) {
	pop(storeCode);
    }
}

/*
 * return indexed type on the stack, skipping index
 */
TVC BlockContext::indexed()
{
    return stack->get(stack->pop(sp));
}

/*
 * handle a kfun call and return the resulting type
 */
Type BlockContext::kfun(LPCKFunCall *kf, Code *code)
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
		    for (i = SUM_AGGREGATE - val; i != 0; --i) {
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
	if (kf->lval != 0) {
	    /* pop non-lval arguments */
	    for (nargs = kf->lval; nargs != 0; --nargs) {
		pop(code);
	    }
	    push(LPC_TYPE_ARRAY);
	} else {
	    if (spreadArgs) {
		--nargs;
	    }
	    while (nargs != 0) {
		pop(code);
		--nargs;
	    }
	}

	type = kf->type;
    }

    spreadArgs = false;
    return type;
}

/*
 * pop function arguments
 */
void BlockContext::args(int nargs, Code *code)
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
 * merge stacks after evaluating code
 */
StackSize BlockContext::merge(StackSize codeSp)
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
bool BlockContext::changed()
{
    return (sp != STACK_EMPTY ||
	    memcmp(origParams, params, nParams) != 0 ||
	    memcmp(origLocals, locals, nLocals) != 0);
}

/*
 * find the Code that consumes a type/value
 */
Code *BlockContext::consumer(StackSize stackPointer)
{
    while (stackPointer != STACK_EMPTY) {
	TVC val = stack->get(stackPointer);
	if (val.code != NULL) {
	    return val.code;
	}
	stackPointer = val.merge;
    }

    return NULL;
}

/*
 * calculate stack depth
 */
StackSize BlockContext::depth(StackSize stackPointer)
{
    StackSize depth;

    depth = 0;
    while (stackPointer != STACK_EMPTY) {
	depth++;
	stackPointer = stack->pop(stackPointer);
    }

    return depth;
}

/*
 * return the top type of a particular stack pointer
 */
Type BlockContext::topType(StackSize stackPointer)
{
    return stack->get(stackPointer).type;
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
 * evaluate type changes
 */
void TypedCode::evaluate(BlockContext *context)
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
    case SPREAD_STORES:
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
	context->params[param] = val.type;
	context->push(val);
	break;

    case STORE_LOCAL:
	val = context->pop(this);
	context->locals[local] = val.type;
	context->push(val);
	break;

    case STORE_GLOBAL:
	context->push(context->pop(this));
	break;

    case STORE_INDEX:
	val = context->pop(this);
	context->pop(this);
	context->pop(this);
	context->push(val);
	break;

    case STORE_PARAM_INDEX:
	val = context->pop(this);
	context->pop(this);
	context->params[param] = context->pop(this).type;
	context->push(val);
	break;

    case STORE_LOCAL_INDEX:
	val = context->pop(this);
	context->pop(this);
	context->locals[local] = context->pop(this).type;
	context->push(val);
	break;

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
	context->stores(size, (pop) ? this : NULL);
	return;

    case SPREADX:
	sp = context->merge(sp);
	context->storeN();
	return;

    case CASTX:
	context->castType = simplifiedType(type.type);
	break;

    case STOREX_PARAM:
	context->params[param] = context->castType;
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_LOCAL:
	context->locals[local] = context->castType;
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_GLOBAL:
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_INDEX:
	context->pop(this);
	context->pop(this);
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_PARAM_INDEX:
	context->pop(this);
	context->params[param] = context->pop(this).type;
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_LOCAL_INDEX:
	context->pop(this);
	context->locals[local] = context->pop(this).type;
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_GLOBAL_INDEX:
	context->pop(this);
	context->pop(this);
	sp = context->merge(sp);
	context->storeN();
	return;

    case STOREX_INDEX_INDEX:
	context->pop(this);
	context->pop(this);
	context->pop(this);
	context->pop(this);
	sp = context->merge(sp);
	context->storeN();
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
    case KFUNC_STORES:
	context->push(simplifiedType(context->kfun(&kfun, this)));
	break;

    case DFUNC:
	context->args(dfun.nargs, this);
	context->push(simplifiedType(dfun.type));
	break;

    case FUNC:
	context->args(fun.nargs, this);
	context->push(LPC_TYPE_MIXED);
	break;

    case CATCH:
	context->push(LPC_TYPE_STRING);
	break;

    case END_CATCH:
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

/*
 * create a typed code
 */
Code *TypedCode::create(CodeFunction *function)
{
    return new TypedCode(function);
}


TypedBlock::TypedBlock(Code *first, Code *last, CodeSize size) :
    Block(first, last, size)
{
    params = NULL;
    locals = NULL;
    endSp = STACK_INVALID;
}

TypedBlock::~TypedBlock()
{
    delete[] params;
    delete[] locals;
}

/*
 * prepare a context with params, locals and stack from this block
 */
void TypedBlock::setContext(BlockContext *context, Block *b)
{
    context->prologue(params, locals, endSp, b);
}

/*
 * evaluate code in a block
 */
void TypedBlock::evaluate(BlockContext *context, Block **list)
{
    Code *code;
    CodeSize i, j;
    Block *b;

    context->sp = sp;

    for (code = first; ; code = code->next) {
	code->evaluate(context);
	if (code == last) {
	    break;
	}
    }

    if (endSp == STACK_INVALID || context->changed()) {
	/*
	 * save state
	 */
	if (endSp == STACK_INVALID) {
	    endSp = context->sp;
	    if (context->nParams != 0) {
		params = new Type[context->nParams];
	    }
	    if (context->nLocals != 0) {
		locals = new Type[context->nLocals];
	    }
	}
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
}

/*
 * evaluate all blocks
 */
BlockContext *TypedBlock::evaluate(CodeFunction *func, StackSize size)
{
    Block *list, *b, *to;
    StackSize sp;
    BlockContext *context;
    Code *code;
    CodeSize i;

    context = new BlockContext(func, size);
    startVisits(&list);
    sp = STACK_EMPTY;
    for (b = next; b != NULL; b = b->next) {
	b->sp = STACK_INVALID;
    }

    /* eval this block */
    evaluate(context, &list);

    for (b = this; b != NULL; ) {
	for (i = 0; ; i++) {
	    if (i == b->nFrom) {
		b = b->nextVisit(&list);
		break;
	    }
	    if (b->fromVisit[i]) {
		b->fromVisit[i] = false;
		b->from[i]->setContext(context, b);
		b->evaluate(context, &list);
		break;
	    }
	}
    }

    return context;
}

/*
 * create a typed block
 */
Block *TypedBlock::create(Code *first, Code *last, CodeSize size)
{
    return new TypedBlock(first, last, size);
}
