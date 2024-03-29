# include <stdlib.h>
# include <stdint.h>
# include <new>
# include <string.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "jitcomp.h"


BlockContext::BlockContext()
{
    storeCount = 0;
    storeCode = NULL;
    lvalue = false;
}

BlockContext::~BlockContext()
{
}

/*
 * prepare for N stores
 */
bool BlockContext::stores(int count, Code *popCode, bool flag)
{
    storeCount = count;
    storeCode = popCode;
    lvalue = flag;
    return (count != 0);
}

/*
 * handle store N
 */
bool BlockContext::storeN()
{
    return (--storeCount != 0);
}


Block::Block(Code *first, Code *last, CodeSize size) :
    first(first), last(last), size(size)
{
    caught = NULL;
    from = to = NULL;
    fromVisit = NULL;
    nFrom = nTo = 0;
    flags = 0;
    endSp = STACK_INVALID;
    mod = NULL;
}

Block::~Block()
{
    Code *code, *next;

    if (first != NULL) {
	for (code = first; ; code = next) {
	    next = code->next;
	    delete code;
	    if (code == last) {
		break;
	    }
	}
    }

    delete[] from;
    delete[] fromVisit;
    delete[] to;
    delete[] mod;
}

/*
 * find a block
 */
Block *Block::find(CodeSize addr)
{
    Block *n, *t, *l, *r, b(NULL, NULL, 0);

    n = this;
    l = r = &b;

    for (;;) {
	if (n->first->addr <= addr) {
	    if (n->first->addr + n->size > addr) {
		break;			/* found */
	    }

	    t = n->right;
	    if (t == NULL) {
		return NULL;		/* out of range */
	    }
	    if (t->first->addr + t->size > addr) {
		l->right = n;
		l = n;
		n = t;
		if (t->first->addr <= addr) {
		    break;		/* found */
		}
	    } else {
		/* rotate */
		n->right = t->left;
		t->left = n;
		l->right = t;
		l = t;
		n = t->right;
		if (n == NULL) {
		    return NULL;	/* out of range */
		}
	    }
	} else {
	    t = n->left;
	    if (t->first->addr <= addr) {
		r->left = n;
		r = n;
		n = t;
	    } else {
		/* rotate */
		n->left = t->right;
		t->right = n;
		r->left = t;
		r = t;
		n = t->left;
	    }
	}
    }

    l->right = n->left;
    r->left = n->right;
    n->right = b.left;
    n->left = b.right;
    return n;
}

/*
 * find and possibly split a block
 */
Block *Block::split(CodeSize addr)
{
    Block *b;

    b = find(addr);
    if (b->first->addr < addr) {
	Code *code, *last;
	CodeSize size;
	Block *n;

	/* find last address */
	code = b->first;
	do {
	    last = code;
	    code = code->next;
	} while (code->addr < addr);
	if (code->addr != addr) {
	    return NULL;	/* invalid address */
	}

	/* split block */
	size = last->next->addr - b->first->addr;
	n = produce(last->next, b->last, b->size - size);
	n->next = b->next;
	n->to = b->to;
	n->nTo = b->nTo;
	b->next = n;
	b->last = last;
	b->size = size;
	b->to = NULL;
	b->nTo = 0;

	/* insert at root of tree */
	n->left = b;
	n->right = b->right;
	b->right = NULL;
	b = n;
    }

    return b;
}

/*
 * create single block for function
 */
Block *Block::function(CodeFunction *function)
{
    Code *code, *first, *last;
    CodeByte *program;
    CodeSize addr, stores;
    bool lval, spread;

    first = last = NULL;
    program = function->getPC(&addr);
    stores = 0;
    lval = spread = false;
    while (function->endProg() == NULL) {
	/* add new code */
	code = Code::produce(function);
	if (first == NULL) {
	    first = last = code;
	} else {
	    last->next = code;
	    last = code;
	}
	if (stores != 0) {
	    switch (code->instruction) {
	    case Code::SPREAD:
		code->instruction = Code::STORES_SPREAD;
		break;

	    case Code::CAST:
		code->instruction = Code::STORES_CAST;
		continue;

	    case Code::STORE_PARAM:
		code->instruction = Code::STORES_PARAM;
		break;

	    case Code::STORE_LOCAL:
		code->instruction = Code::STORES_LOCAL;
		break;

	    case Code::STORE_GLOBAL:
		code->instruction = Code::STORES_GLOBAL;
		break;

	    case Code::STORE_INDEX:
		code->instruction = Code::STORES_INDEX;
		break;

	    case Code::STORE_PARAM_INDEX:
		code->instruction = Code::STORES_PARAM_INDEX;
		break;

	    case Code::STORE_LOCAL_INDEX:
		code->instruction = Code::STORES_LOCAL_INDEX;
		break;

	    case Code::STORE_GLOBAL_INDEX:
		code->instruction = Code::STORES_GLOBAL_INDEX;
		break;

	    case Code::STORE_INDEX_INDEX:
		code->instruction = Code::STORES_INDEX_INDEX;
		break;

	    default:
		fatal("unexpected code %d", code->instruction);
		break;
	    }
	    --stores;
	} else {
	    switch (code->instruction) {
	    case Code::SPREAD:
	    case Code::SPREAD_LVAL:
		spread = true;
		break;

	    case Code::KFUNC:
		if (spread) {
		    code->instruction = Code::KFUNC_SPREAD;
		    spread = false;
		}
		break;

	    case Code::KFUNC_LVAL:
		if (spread) {
		    code->instruction = Code::KFUNC_SPREAD_LVAL;
		    spread = false;
		}
		lval = true;
		break;

	    case Code::DFUNC:
		if (spread) {
		    code->instruction = Code::DFUNC_SPREAD;
		    spread = false;
		}
		break;

	    case Code::FUNC:
		if (spread) {
		    code->instruction = Code::FUNC_SPREAD;
		    spread = false;
		}
		break;

	    case Code::CATCH:
		last = code->next = Code::produce(NULL);
		last->instruction = Code::CAUGHT;
		last->addr = code->addr + 1;
		last->line = code->line;
		last->target = code->target;
		last->pop = code->pop;
		code->pop = false;
		code->target = code->addr + 3;
		code = last;
		break;

	    case Code::STORES:
		if (lval) {
		    code->instruction = Code::STORES_LVAL;
		    lval = false;
		}
		stores = code->size;
		break;

	    default:
		break;
	    }
	}
	code->next = NULL;
    }

    return (first != NULL) ?
	    produce(first, last, function->getPC(&addr) - program) : NULL;
}

/*
 * split into multiple blocks
 */
Block *Block::pass1()
{
    Code *code;
    Block *b, **follow;
    CodeSize i;

    left = right = next = NULL;
    for (b = this, code = this->first; code != NULL; code = code->next) {
	switch (code->instruction) {
	case Code::JUMP:
	case Code::CAUGHT:
	    /* split at jump target */
	    b = b->split(code->target);
	    if (b == NULL) {
		return NULL;
	    }
	    /* one followup */
	    follow = new Block*[1];
	    follow[0] = b;

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* set followups */
	    b = b->find(code->addr);
	    b->to = follow;
	    b->nTo = 1;
	    break;

	case Code::JUMP_ZERO:
	case Code::JUMP_NONZERO:
	case Code::CATCH:
	    /* split at jump target */
	    b = b->split(code->target);
	    if (b == NULL) {
		return NULL;
	    }
	    /* two followups */
	    follow = new Block*[2];
	    follow[1] = b;

	    /* start new block */
	    if (code->next == NULL || (b=b->split(code->next->addr)) == NULL) {
		delete[] follow;
		return NULL;
	    }
	    follow[0] = b;

	    /* set followups */
	    b = b->find(code->addr);
	    b->to = follow;
	    b->nTo = 2;
	    break;

	case Code::SWITCH_INT:
	    /* create jump table with block entry points */
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = b->split(code->caseInt[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* set followups */
	    follow[0]->flags |= BLOCK_DEFAULT;
	    b = b->find(code->addr);
	    b->to = follow;
	    b->nTo = code->size;
	    break;

	case Code::SWITCH_RANGE:
	    /* create jump table with block entry points */
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = b->split(code->caseRange[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* set followups */
	    follow[0]->flags |= BLOCK_DEFAULT;
	    b = b->find(code->addr);
	    b->to = follow;
	    b->nTo = code->size;
	    break;

	case Code::SWITCH_STRING:
	    /* create jump table with block entry points */
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = b->split(code->caseString[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    return NULL;
		}
	    }

	    /* set followups */
	    follow[0]->flags |= BLOCK_DEFAULT;
	    b = b->find(code->addr);
	    b->to = follow;
	    b->nTo = code->size;
	    break;

	default:
	    break;
	}
    }

    return b;
}

/*
 * prepare to visit blocks, excluding the first one
 */
void Block::startVisits(Block **list)
{
    Block *b;

    *list = visit = NULL;
    for (b = next; b != NULL; b = b->next) {
	b->visit = b;
    }
}

/*
 * prepare to visit blocks, including the first one
 */
void Block::startAllVisits(Block **list)
{
    startVisits(list);
    visit = this;
}

/*
 * add block to visitation list
 */
void Block::toVisit(Block **list)
{
    if (visit == this) {
	visit = *list;
	*list = this;
    }
}

/*
 * visit next block in the list
 */
Block *Block::nextVisit(Block **list)
{
    Block *b;

    b = *list;
    if (b != NULL) {
	*list = b->visit;
	b->visit = b;
    }
    return b;
}

/*
 * prepare to visit, but only once
 */
void Block::toVisitOnce(Block **list, StackSize stackPointer,
			Block *caughtBlock)
{
    if (sp == STACK_INVALID) {
	sp = stackPointer;
	caught = caughtBlock;
	toVisit(list);
    }
}

/*
 * initialize modification flags
 */
void Block::initMod(LPCParam size)
{
    mod = new bool[size];
    memset(mod, false, size);
}

/*
 * mark as in need of relaying
 */
void Block::setRelay()
{
    flags |= BLOCK_RELAY;
}

/*
 * needs relaying?
 */
bool Block::relay()
{
    return (flags & BLOCK_RELAY);
}

/*
 * relay to default block?
 */
bool Block::relayToDefault(Block *to)
{
    return (flags & BLOCK_RELAY) && (to->flags & BLOCK_DEFAULT);
}

/*
 * transform RETURN into END_CATCH or END_RLIMITS, as required
 */
Block *Block::pass2(Block *tree, StackSize size)
{
    Stack<Context> context(size);	/* too large but we need some limit */
    Block *b, *list, *caughtBlock;
    Code *code;
    CodeSize stackPointer, i;

    /*
     * determine the catch/rlimits context at the start and end of each block
     */
    startVisits(&list);
    sp = STACK_EMPTY;
    for (b = next; b != NULL; b = b->next) {
	b->sp = STACK_INVALID;
    }
    for (b = this; b != NULL; b = nextVisit(&list)) {
	stackPointer = b->sp;
	caughtBlock = b->caught;

	/*
	 * go through the code and make adjustments as needed
	 */
	for (code = b->first; ; code = code->next) {
	    switch (code->instruction) {
	    case Code::CATCH:
		stackPointer = context.push(stackPointer, Block::CATCH);
		break;

	    case Code::RLIMITS:
	    case Code::RLIMITS_CHECK:
		stackPointer = context.push(stackPointer, Block::RLIMITS);
		break;

	    case Code::RETURN:
		if (stackPointer != STACK_EMPTY) {
		    switch (context.get(stackPointer)) {
		    case Block::CATCH:
			code->instruction = Code::END_CATCH;
			caughtBlock = caughtBlock->caught;
			break;

		    case Block::RLIMITS:
			code->instruction = Code::END_RLIMITS;
			break;
		    }
		    stackPointer = context.pop(stackPointer);
		} else if (code != b->last) {
		    tree = tree->split(code->next->addr);
		}
		break;

	    default:
		break;
	    }

	    if (code == b->last) {
		break;
	    }
	}

	if (b->nTo != 0) {
	    /* add followups to the list */
	    if (code->instruction == Code::CATCH) {
		/*
		 * special case for CATCH: the context will be gone after an
		 * exception occurs
		 */
		b->to[0]->caught = caughtBlock;
		b->to[0]->toVisitOnce(&list, context.pop(stackPointer),
				      caughtBlock);
		b->to[1]->toVisitOnce(&list, stackPointer, b->to[0]);
	    } else {
		for (i = 0; i < b->nTo; i++) {
		    b->to[i]->toVisitOnce(&list, stackPointer, caughtBlock);
		}
	    }
	} else if (b->next != NULL && code->instruction != Code::RETURN) {
	    /* add the next block to the list */
	    b->next->toVisitOnce(&list, stackPointer, caughtBlock);
	} else if (stackPointer != STACK_EMPTY) {
	    /* function ended within a catch/rlimits context */
	    fatal("catch/rlimits return mismatch");
	}
    }

    return tree;
}

/*
 * determine nFroms
 */
void Block::pass3(Block *b)
{
    Block *list, *f;
    Code *code;
    CodeSize i;

    nFrom++;
    startVisits(&list);
    for (f = this; f != NULL; f = nextVisit(&list)) {
	code = f->last;
	switch (code->instruction) {
	case Code::JUMP_ZERO:
	case Code::JUMP_NONZERO:
	    code = code->next;
	    if (code->instruction == Code::JUMP) {
		f->to[0] = b = b->find(code->target);
	    }
	    /* fall through */
	case Code::JUMP:
	case Code::CAUGHT:
	case Code::SWITCH_INT:
	case Code::SWITCH_RANGE:
	case Code::SWITCH_STRING:
	    for (i = 0; i < f->nTo; i++) {
		if (f->to[i]->nFrom++ == 0) {
		    f->to[i]->toVisit(&list);
		}
	    }
	    break;

	case Code::CATCH:
	    code = f->to[1]->first;
	    if (code->instruction == Code::JUMP) {
		f->to[1] = b = b->find(code->target);
	    }
	    for (i = 0; i < f->nTo; i++) {
		if (f->to[i]->nFrom++ == 0) {
		    f->to[i]->toVisit(&list);
		}
	    }
	    break;

	case Code::RETURN:
	    break;

	default:
	    /* make transition explicit */
	    f->nTo = 1;
	    f->to = new Block*[1];
	    code = code->next;
	    b = b->find((code->instruction == Code::JUMP) ?
			 code->target : code->addr);
	    f->to[0] = b;
	    if (b->nFrom++ == 0) {
		b->toVisit(&list);
	    }
	    break;
	}
    }
    --nFrom;
}


/*
 * determine froms
 */
void Block::pass4()
{
    Block *list, *f, *b;
    CodeSize i;

    startVisits(&list);
    for (f = this; f != NULL; f = nextVisit(&list)) {
	for (i = 0; i < f->nTo; i++) {
	    b = f->to[i];
	    if (b->from == NULL) {
		b->from = new Block*[b->nFrom];
		b->fromVisit = new bool[b->nFrom];
		memset(b->fromVisit, false, b->nFrom);
		b->nFrom = 0;
		b->toVisit(&list);
	    }
	    b->from[b->nFrom++] = f;
	}
    }
}

void Block::setContext(class TypedContext *context, Block *b)
{
}

void Block::prepareFlow(class FlowContext *context)
{
}

void Block::evaluateTypes(class TypedContext *context, Block **list)
{
}

void Block::evaluateFlow(class FlowContext *context, Block **list)
{
}

void Block::evaluateInputs(class FlowContext *context, Block **list)
{
}

void Block::evaluateOutputs(class FlowContext *context, Block **list)
{
}

Type Block::paramType(LPCParam param)
{
    return LPC_TYPE_VOID;
}

int Block::paramIn(LPCParam param)
{
    return 0;
}

int Block::paramOut(LPCParam param)
{
    return 0;
}

bool Block::paramMerged(LPCParam param)
{
    return false;
}

Type Block::localType(LPCLocal local)
{
    return LPC_TYPE_VOID;
}

int Block::localIn(LPCLocal local)
{
    return 0;
}

int Block::localOut(LPCLocal local)
{
    return 0;
}

bool Block::localMerged(LPCLocal local)
{
    return false;
}

Type Block::mergedParamType(LPCParam param, Type type)
{
    return LPC_TYPE_VOID;
}

Type Block::mergedLocalType(LPCLocal local)
{
    return LPC_TYPE_VOID;
}

void Block::emit(class GenContext *context, CodeFunction *function)
{
}


/*
 * delete a list of blocks
 */
void Block::clear()
{
    Block *b, *next;

    b = this;
    while (b != NULL) {
	next = b->next;
	delete b;
	b = next;
    }
}

/*
 * break up a function block
 */
CodeSize Block::fragment()
{
    Block *tree;
    CodeSize funcSize;

    funcSize = size;

    tree = pass1();
    if (tree == NULL) {
	return 0;
    }

    tree = pass2(tree, funcSize);
    pass3(tree);
    pass4();

    return funcSize;
}

Block *(*Block::factory)(Code*, Code*, CodeSize);

/*
 * set the factory which produces blocks
 */
void Block::producer(Block *(*factory)(Code*, Code*, CodeSize))
{
    Block::factory = factory;
}

/*
 * let the factory produce a block
 */
Block *Block::produce(Code *first, Code *last, CodeSize size)
{
    return (*factory)(first, last, size);
}
