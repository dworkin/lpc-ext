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
# include "jitcomp.h"


Block::Block(Code *first, Code *last, CodeSize size) :
    first(first), last(last), size(size)
{
    from = to = NULL;
    nFrom = nTo = 0;
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
    delete[] to;
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
 * obtain single block for function
 */
Block *Block::pass0(CodeFunction *function)
{
    Code *code, *first, *last;
    CodeByte *program, *end;
    CodeSize addr, stores;

    first = NULL;
    program = function->getPC(&addr);
    stores = 0;
    while ((end=function->endProg()) == NULL) {
	/* add new code */
	code = Code::produce(function);
	code->next = NULL;
	if (first == NULL) {
	    first = last = code;
	} else {
	    last->next = code;
	    last = code;
	}
	if (code->instruction == Code::STORES) {
	    stores = code->size;
	} else if (stores != 0) {
	    switch (code->instruction) {
	    case Code::SPREAD:
		code->instruction = Code::SPREADX;
		break;

	    case Code::CAST:
		code->instruction = Code::CASTX;
		continue;

	    case Code::STORE_PARAM:
		code->instruction = Code::STOREX_PARAM;
		break;

	    case Code::STORE_LOCAL:
		code->instruction = Code::STOREX_LOCAL;
		break;

	    case Code::STORE_GLOBAL:
		code->instruction = Code::STOREX_GLOBAL;
		break;

	    case Code::STORE_INDEX:
		code->instruction = Code::STOREX_INDEX;
		break;

	    case Code::STORE_PARAM_INDEX:
		code->instruction = Code::STOREX_PARAM_INDEX;
		break;

	    case Code::STORE_LOCAL_INDEX:
		code->instruction = Code::STOREX_LOCAL_INDEX;
		break;

	    case Code::STORE_GLOBAL_INDEX:
		code->instruction = Code::STOREX_GLOBAL_INDEX;
		break;

	    case Code::STORE_INDEX_INDEX:
		code->instruction = Code::STOREX_INDEX_INDEX;
		break;

	    default:
		fatal("unexpected code %d", code->instruction);
		break;
	    }
	    --stores;
	}
    }

    return (first != NULL) ?
	    Block::produce(first, last, function->getPC(&addr) - program) :
	    NULL;
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
 * prepare a round of visits
 */
void Block::startVisits(Block **list)
{
    Block *b;

    start = STACK_EMPTY;
    end = STACK_INVALID;
    *list = visit = NULL;
    for (b = next; b != NULL; b = b->next) {
	b->start = b->end = STACK_INVALID;
	b->visit = b;
    }
}

/*
 * block already visited this round?
 */
bool Block::visited()
{
    return (start != STACK_INVALID);
}

/*
 * add block to visitation list
 */
void Block::toVisit(Block **list, StackSize offset)
{
    if (start == STACK_INVALID) {
	start = offset;
    } else if (start != offset) {
	fatal("start stack mismatch");
    }

    if (visit == this) {
	visit = *list;
	*list = this;
    }
}

/*
 * visit next block in the list
 */
Block *Block::nextVisit(Block **list, StackSize offset)
{
    Block *b;

    if (end == STACK_INVALID) {
	end = offset;
    } else if (end != offset) {
	fatal("end stack mismatch");
    }

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
void Block::toVisitOnce(Block **list, StackSize offset)
{
    if (!visited()) {
	toVisit(list, offset);
    }
}

/*
 * transform RETURN into END_CATCH or END_RLIMITS, as required
 */
void Block::pass2(CodeSize size)
{
    Stack<Context> context(size);	/* too large but we need some limit */
    Block *b, *list;
    Code *code;
    CodeSize offset, i;

    /*
     * determine the catch/rlimits context at the start and end of each block
     */
    startVisits(&list);
    for (b = this; b != NULL; b = b->nextVisit(&list, offset)) {
	offset = b->offset();

	/*
	 * go through the code and make adjustments as needed
	 */
	for (code = b->first; ; code = code->next) {
	    switch (code->instruction) {
	    case Code::CATCH:
		offset = context.push(offset, Block::CATCH);
		break;

	    case Code::RLIMITS:
	    case Code::RLIMITS_CHECK:
		offset = context.push(offset, Block::RLIMITS);
		break;

	    case Code::RETURN:
		if (offset != STACK_EMPTY) {
		    if (context.get(offset) == Block::CATCH) {
			code->instruction = Code::END_CATCH;
		    } else {
			code->instruction = Code::END_RLIMITS;
		    }
		    offset = context.pop(offset);
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
		b->to[0]->toVisitOnce(&list, offset);
		b->to[1]->toVisitOnce(&list, context.pop(offset));
	    } else {
		for (i = 0; i < b->nTo; i++) {
		    b->to[i]->toVisitOnce(&list, offset);
		}
	    }
	} else if (b->next != NULL && code->instruction != Code::RETURN) {
	    /* add the next block to the list */
	    b->next->toVisitOnce(&list, offset);
	} else if (offset != STACK_EMPTY) {
	    /* function ended within a catch/rlimits context */
	    fatal("catch/rlimits return mismatch");
	}
    }
}

/*
 * determine nFroms
 */
void Block::pass3(Block *b)
{
    Block *f;
    Code *code;
    CodeSize i;

    for (f = this; f != NULL; f = f->next) {
	code = f->last;
	switch (code->instruction) {
	case Code::JUMP:
	case Code::JUMP_ZERO:
	case Code::JUMP_NONZERO:
	case Code::CATCH:
	case Code::SWITCH_INT:
	case Code::SWITCH_RANGE:
	case Code::SWITCH_STRING:
	    for (i = 0; i < f->nTo; i++) {
		f->to[i]->nFrom++;
	    }
	    break;

	case Code::RETURN:
	    break;

	default:
	    /* make transition explicit */
	    f->nTo = 1;
	    f->to = new Block*[1];
	    f->to[0] = b = b->find(code->next->addr);
	    b->nFrom++;
	    break;
	}
    }
}


/*
 * determine froms
 */
void Block::pass4()
{
    Block *f, *b;
    CodeSize i;

    for (f = this; f != NULL; f = f->next) {
	for (i = 0; i < f->nTo; i++) {
	    b = f->to[i];
	    if (b->from == NULL) {
		b->from = new Block*[b->nFrom];
		b->nFrom = 0;
	    }
	    b->from[b->nFrom++] = f;
	}
    }
}

void Block::evaluate(class BlockContext *context)
{
}

void Block::emit()
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
 * break up a function into code blocks
 */
Block *Block::blocks(CodeFunction *function)
{
    Block *first, *tree;
    CodeSize size;

    first = pass0(function);
    if (first == NULL) {
	return NULL;
    }
    size = first->size;

    tree = first->pass1();
    if (tree == NULL) {
	first->clear();
	return NULL;
    }
    first->pass2(size);
    first->pass3(tree);
    first->pass4();

    return first;
}

/*
 * produce a block
 */
Block *Block::create(Code *first, Code *last, CodeSize size)
{
    return new Block(first, last, size);
}

Block *(*Block::factory)(Code*, Code*, CodeSize) = &create;

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
