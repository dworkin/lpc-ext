# include <stdlib.h>
# include <stdint.h>
# include <new>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "jitcomp.h"


BlockContext::BlockContext(CodeFunction *func, StackSize size)
{
    int i;

    nParams = func->nargs + func->vargs;
    nLocals = func->locals;
    params = (nParams != 0) ? new Type[nParams] : NULL;
    locals = (nLocals != 0) ? new Type[nLocals] : NULL;
    for (i = 1; i <= nParams; i++) {
	params[nParams - i] = func->proto[i].type;
    }
    if (func->fclass & CodeFunction::CLASS_ELLIPSIS) {
	params[0] = LPC_TYPE_ARRAY;
    }
    for (i = 0; i < nLocals; i++) {
	locals[i] = LPC_TYPE_NIL;
    }

    stack = new Stack<TypeVal>(size);
    sp = STACK_EMPTY;
    storeCount = 0;
    storePop = false;
    spreadArgs = false;
}

BlockContext::~BlockContext()
{
    delete params;
    delete locals;
    delete stack;
}

void BlockContext::push(TypeVal val)
{
    sp = stack->push(sp, val);
}

TypeVal BlockContext::top()
{
    return stack->get(sp);
}

TypeVal BlockContext::pop()
{
    if (sp == STACK_EMPTY) {
	fatal("pop empty stack");
    }
    TypeVal val = stack->get(sp);
    sp = stack->pop(sp);
    return val;
}

void BlockContext::stores(int count, bool pop)
{
    storeCount = count;
    storePop = pop;
}

void BlockContext::storeN()
{
    if (--storeCount == 0 && storePop) {
	pop();
    }
}

void BlockContext::setParam(LPCParam param, TypeVal val)
{
    params[param] = val.type;
}

void BlockContext::setLocal(LPCLocal local, TypeVal val)
{
    locals[local] = val.type;
}

TypeVal BlockContext::getParam(LPCParam param)
{
    return TypeVal(params[param], 0);
}

TypeVal BlockContext::getLocal(LPCLocal local)
{
    return TypeVal(locals[local], 0);
}

TypeVal BlockContext::indexed()
{
    return stack->get(stack->pop(sp));
}

# define KF_SUM		89
# define SUM_SIMPLE	-2
# define SUM_AGGREGATE	-6

Type BlockContext::kfun(LPCKFunc *kf)
{
    int nargs, i;
    Type type, summand;
    LPCInt val;

    nargs = kf->nargs;
    if (kf->func == KF_SUM) {
	type = LPC_TYPE_VOID;
	while (nargs != 0) {
	    val = pop().val;
	    if (val < 0) {
		if (val <= SUM_AGGREGATE) {
		    /* aggregate */
		    for (i = SUM_AGGREGATE - val; i != 0; --i) {
			pop();
		    }
		    summand = LPC_TYPE_ARRAY;
		} else if (val == SUM_SIMPLE) {
		    /* simple argument */
		    summand = pop().type;
		} else {
		    /* allocate array */
		    pop();
		    summand = LPC_TYPE_ARRAY;
		}
	    } else {
		/* subrange */
		pop();
		summand = pop().type;
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
		pop();
	    }
	    push(LPC_TYPE_ARRAY);
	} else {
	    if (spreadArgs) {
		--nargs;
	    }
	    while (nargs != 0) {
		pop();
		--nargs;
	    }
	}

	type = kf->type;
    }

    spreadArgs = false;
    return type;
}

void BlockContext::args(int nargs)
{
    if (spreadArgs) {
	--nargs;
    }
    while (nargs != 0) {
	pop();
	--nargs;
    }
    spreadArgs = false;
}

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

Type TypedCode::simplifiedType(Type type)
{
    return (LPC_TYPE_REF(type) != 0) ? LPC_TYPE_ARRAY : type;
}

void TypedCode::evaluate(BlockContext *context)
{
    TypeVal val;
    int i;

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
	context->push(simplifiedType(context->getParam(param).type));
	break;

    case LOCAL:
	context->push(context->getLocal(local));
	break;

    case GLOBAL:
	context->push(simplifiedType(var.type));
	break;

    case INDEX:
	val = context->indexed();
	context->pop();
	context->pop();
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
	context->pop();
	context->spread();
	break;

    case AGGREGATE:
	for (i = size; i != 0; --i) {
	    context->pop();
	}
	context->push(LPC_TYPE_ARRAY);
	break;

    case MAP_AGGREGATE:
	for (i = size; i != 0; --i) {
	    context->pop();
	}
	context->push(LPC_TYPE_MAPPING);
	break;

    case CAST:
	context->pop();
	context->push(simplifiedType(type.type));
	break;

    case INSTANCEOF:
	context->pop();
	context->push(LPC_TYPE_INT);
	break;

    case CHECK_RANGE:
	break;

    case CHECK_RANGE_FROM:
	context->push(LPC_TYPE_INT);
	break;

    case CHECK_RANGE_TO:
	val = context->pop();
	context->push(LPC_TYPE_INT);
	context->push(val);
	break;

    case STORE_PARAM:
	context->setParam(param, context->top());
	break;

    case STORE_LOCAL:
	context->setLocal(local, context->top());
	break;

    case STORE_GLOBAL:
	break;

    case STORE_INDEX:
	val = context->pop();
	context->pop();
	context->pop();
	context->push(val);
	break;

    case STORE_PARAM_INDEX:
	val = context->pop();
	context->pop();
	context->setParam(param, context->pop());
	context->push(val);
	break;

    case STORE_LOCAL_INDEX:
	val = context->pop();
	context->pop();
	context->setLocal(local, context->pop());
	context->push(val);
	break;

    case STORE_GLOBAL_INDEX:
	val = context->pop();
	context->pop();
	context->pop();
	context->push(val);
	break;

    case STORE_INDEX_INDEX:
	val = context->pop();
	context->pop();
	context->pop();
	context->pop();
	context->pop();
	context->push(val);
	break;

    case STORES:
	context->pop();
	context->stores(size, pop);
	sp = context->stackPointer();
	return;

    case SPREADX:
	context->storeN();
	break;

    case CASTX:
	context->castX(simplifiedType(type.type));
	break;

    case STOREX_PARAM:
	context->setParam(param, context->typeX());
	context->storeN();
	break;

    case STOREX_LOCAL:
	context->setLocal(local, context->typeX());
	context->storeN();
	break;

    case STOREX_GLOBAL:
	context->storeN();
	break;

    case STOREX_INDEX:
	context->pop();
	context->pop();
	context->storeN();
	break;

    case STOREX_PARAM_INDEX:
	context->pop();
	context->setParam(param, context->pop());
	context->storeN();
	break;

    case STOREX_LOCAL_INDEX:
	context->pop();
	context->setLocal(local, context->pop());
	context->storeN();
	break;

    case STOREX_GLOBAL_INDEX:
	context->pop();
	context->pop();
	context->storeN();
	break;

    case STOREX_INDEX_INDEX:
	context->pop();
	context->pop();
	context->pop();
	context->pop();
	context->storeN();
	break;

    case JUMP:
    case JUMP_ZERO:
    case JUMP_NONZERO:
    case SWITCH_INT:
    case SWITCH_RANGE:
    case SWITCH_STRING:
	break;

    case KFUNC:
    case KFUNC_STORES:
	context->push(simplifiedType(context->kfun(&kfun)));
	break;

    case DFUNC:
	context->args(dfun.nargs);
	context->push(simplifiedType(dfun.type));
	break;

    case FUNC:
	context->args(fun.nargs);
	context->push(LPC_TYPE_MIXED);
	break;

    case CATCH:
	context->push(LPC_TYPE_STRING);
	break;

    case END_CATCH:
	break;

    case RLIMITS:
    case RLIMITS_CHECK:
	context->pop();
	context->pop();
	break;

    case END_RLIMITS:
	break;

    case RETURN:
	context->pop();
	if (context->stackPointer() != STACK_EMPTY) {
	    fatal("stack not empty");
	}
	break;

    default:
	fatal("unknown instruction");
    }

    if (pop) {
	context->pop();
    }
    sp = context->stackPointer();
}

Code *TypedCode::create(CodeFunction *function)
{
    return new TypedCode(function);
}


TypedBlock::TypedBlock(Code *first, Code *last, CodeSize size) :
    Block(first, last, size)
{
}

TypedBlock::~TypedBlock()
{
}

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

    for (b = this; b != NULL; b = b->nextVisit(&list)) {
	context->setStackPointer(b->sp);
	for (code = b->first; ; code = code->next) {
	    code->evaluate(context);
	    if (code == b->last) {
		break;
	    }
	}
	sp = context->stackPointer();

	for (i = 0; i < b->nTo; i++) {
	    to = b->to[i];
	    if (to->sp == STACK_INVALID) {
		to->sp = sp;
		to->toVisit(&list);
	    }
	}
    }

    return context;
}

Block *TypedBlock::create(Code *first, Code *last, CodeSize size)
{
    return new TypedBlock(first, last, size);
}
