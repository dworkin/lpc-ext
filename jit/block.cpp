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
 * track this block
 */
void Block::track(Block **list, StackSize offset)
{
    if (start == STACK_INVALID) {
	this->list = *list;
	*list = this;
	start = offset;
    } else if (start != offset) {
	fatal("catch/rlimits branch mismatch");
    }
}

/*
 * split function into blocks
 */
Block *Block::pass1()
{
    Code *code;
    Block *b, **follow;
    CodeSize i;

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
 * transform RETURN into END_CATCH or END_RLIMITS, as required
 */
void Block::pass2()
{
    Block *b, *list;
    CodeSize size;
    Code *code;
    CodeSize i;

    list = b = this;

    /* initially, no blocks are in the list */
    while (b != NULL) {
	b->start = b->end = STACK_INVALID;
	size = b->last->addr;
	b = b->next;
    }

    Stack<Context> context(size);	/* too large but we need some limit */
    list->start = STACK_EMPTY;
    list->list = NULL;

    /*
     * determine the catch/rlimits context at the start and end of each block
     */
    while (list != NULL) {
	b = list;
	list = b->list;

	if (b->end != STACK_INVALID) {
	    fatal("block list corrupted");
	}
	b->end = b->start;

	/*
	 * go through the code and make adjustments as needed
	 */
	for (code = b->first; ; code = code->next) {
	    switch (code->instruction) {
	    case Code::CATCH:
		b->end = context.push(b->end, Block::CATCH);
		break;

	    case Code::RLIMITS:
	    case Code::RLIMITS_CHECK:
		b->end = context.push(b->end, Block::RLIMITS);
		break;

	    case Code::RETURN:
		if (b->end != STACK_EMPTY) {
		    if (context.get(b->end) == Block::CATCH) {
			code->instruction = Code::END_CATCH;
		    } else {
			code->instruction = Code::END_RLIMITS;
		    }
		    b->end = context.pop(b->end);
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
		b->to[0]->track(&list, b->end);
		b->to[1]->track(&list, context.pop(b->end));
	    } else {
		for (i = 0; i < b->nTo; i++) {
		    b->to[i]->track(&list, b->end);
		}
	    }
	} else if (b->next != NULL && code->instruction != Code::RETURN) {
	    /* add the next block to the list */
	    b->next->track(&list, b->end);
	} else if (b->end != STACK_EMPTY) {
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

    if (function->first == NULL) {
	return NULL;
    }

    function->getPC(&size);
    first = produce(function->first, function->last, size);
    first->next = first->left = first->right = NULL;

    tree = first->pass1();
    if (tree == NULL) {
	first->clear();
	return NULL;
    }
    first->pass2();
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
