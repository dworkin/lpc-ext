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


BlockContext::BlockContext(StackSize size)
{
    stack = new Stack<TypeVal>(size);
    sp = STACK_EMPTY;
    storeCount = 0;
    storePop = false;
    spreadArgs = false;
}

BlockContext::~BlockContext()
{
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
}

void BlockContext::setLocal(LPCLocal local, TypeVal val)
{
}

TypeVal BlockContext::getParam(LPCParam param)
{
    return TypeVal(LPC_TYPE_INT, 0);
}

TypeVal BlockContext::getLocal(LPCLocal local)
{
    return TypeVal(LPC_TYPE_INT, 0);
}

# define KF_SUM		89
# define SUM_AGGREGATE	-6

Type BlockContext::kfun(LPCKFunc *kf)
{
    int i, j;
    TypeVal val;

    i = kf->nargs;
    if (kf->func == KF_SUM) {
	while (i != 0) {
	    val = pop();
	    if (val.val < 0) {
		if (val.val <= SUM_AGGREGATE) {
		    /* aggregate */
		    for (j = SUM_AGGREGATE - val.val; j != 0; --j) {
			pop();
		    }
		} else {
		    pop();
		}
	    } else {
		/* subrange */
		pop();
		pop();
	    }
	    --i;
	}
    } else {
	if (kf->lval != 0) {
	    /* pop non-lval arguments */
	    for (i = kf->lval; i != 0; --i) {
		pop();
	    }
	    push(LPC_TYPE_ARRAY);
	} else {
	    if (spreadArgs) {
		--i;
	    }
	    while (i != 0) {
		pop();
		--i;
	    }
	}
    }
    spreadArgs = false;

    return LPC_TYPE_INT;
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
    return type;
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
	context->push(context->getParam(param));
	break;

    case LOCAL:
	context->push(context->getLocal(local));
	break;

    case GLOBAL:
	context->push(simplifiedType(var.type));
	break;

    case INDEX:
	context->pop();
	context->pop();
	context->push(LPC_TYPE_MIXED);
	break;

    case INDEX2:
	context->push(LPC_TYPE_MIXED);
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
	context->pop();
	context->setParam(param, val);
	context->push(val);
	break;

    case STORE_LOCAL_INDEX:
	val = context->pop();
	context->pop();
	context->pop();
	context->setLocal(local, val);
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
	context->pop();
	context->setParam(param, context->typeX());
	context->storeN();
	break;

    case STOREX_LOCAL_INDEX:
	context->pop();
	context->pop();
	context->setLocal(local, context->typeX());
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

BlockContext *TypedBlock::evaluate(StackSize size)
{
    Block *list, *b, *to;
    StackSize sp;
    BlockContext *context;
    Code *code;
    CodeSize i;

    context = new BlockContext(size);
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
