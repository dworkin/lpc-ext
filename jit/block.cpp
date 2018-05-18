# include <stdlib.h>
# include <stdint.h>
# include <new>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "block.h"
# include "jitcomp.h"


Block::Block(Code *first, Code *last, CodeSize size) :
    first(first), last(last), size(size)
{
    follow = NULL;
    nfollow = 0;
}

Block::~Block()
{
    if (follow != NULL) {
	delete[] follow;
    }
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

	    t = n->t.right;
	    if (t == NULL) {
		return NULL;		/* out of range */
	    }
	    if (t->first->addr + t->size > addr) {
		l->t.right = n;
		l = n;
		n = t;
		if (t->first->addr <= addr) {
		    break;		/* found */
		}
	    } else {
		/* rotate */
		n->t.right = t->t.left;
		t->t.left = n;
		l->t.right = t;
		l = t;
		n = t->t.right;
		if (n == NULL) {
		    return NULL;	/* out of range */
		}
	    }
	} else {
	    t = n->t.left;
	    if (t->first->addr <= addr) {
		r->t.left = n;
		r = n;
		n = t;
	    } else {
		/* rotate */
		n->t.left = t->t.right;
		t->t.right = n;
		r->t.left = t;
		r = t;
		n = t->t.left;
	    }
	}
    }

    l->t.right = n->t.left;
    r->t.left = n->t.right;
    n->t.right = b.t.left;
    n->t.left = b.t.right;
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
	n = new Block(last->next, b->last, b->size - size);
	n->next = b->next;
	n->follow = b->follow;
	n->nfollow = b->nfollow;
	b->next = n;
	b->last = last;
	b->size = size;
	b->follow = NULL;
	b->nfollow = 0;

	/* insert at root of tree */
	n->t.left = b;
	n->t.right = b->t.right;
	b->t.right = NULL;
	b = n;
    }

    return b;
}

/*
 * delete a list of blocks
 */
void Block::clear(Block *b)
{
    Block *next;

    while (b != NULL) {
	next = b->next;
	delete b;
	b = next;
    }
}

/*
 * break up a function into code blocks
 */
Block *Block::create(CodeFunction *function)
{
    Code *code;
    Block *first, *b, **follow;
    int i;

    code = function->first;
    if (code == NULL) {
	return NULL;
    }

    first = new Block(code, function->last, function->pc);
    first->next = first->t.left = first->t.right = NULL;
    first->follow = NULL;
    first->nfollow = 0;

    for (b = first; code != NULL; code = code->next) {
	switch (code->instruction) {
	case Code::JUMP:
	    /* split at jump target */
	    b = b->split(code->target);
	    if (b == NULL) {
		clear(first);
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
		    clear(first);
		    return NULL;
		}
	    }

	    /* set followups */
	    b = b->find(code->addr);
	    b->follow = follow;
	    b->nfollow = 1;
	    break;

	case Code::JUMP_ZERO:
	case Code::JUMP_NONZERO:
	case Code::CATCH:
	    /* split at jump target */
	    b = b->split(code->target);
	    if (b == NULL) {
		clear(first);
		return NULL;
	    }
	    /* two followups */
	    follow = new Block*[2];
	    follow[1] = b;

	    /* start new block */
	    if (code->next == NULL ||
		(b=b->split(code->next->addr)) == NULL) {
		delete[] follow;
		clear(first);
		return NULL;
	    }
	    follow[0] = b;

	    /* set followups */
	    b = b->find(code->addr);
	    b->follow = follow;
	    b->nfollow = 2;
	    break;

	case Code::SWITCH_INT:
	    /* create jump table with block entry points */
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		b = b->split(code->caseInt[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
		follow[i] = b;
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
	    }

	    /* set followups */
	    b = b->find(code->addr);
	    b->follow = follow;
	    b->nfollow = code->size;
	    break;

	case Code::SWITCH_RANGE:
	    /* create jump table with block entry points */
	    b = b->find(code->addr);
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = b->split(code->caseRange[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
	    }

	    /* set followups */
	    b = b->find(code->addr);
	    b->follow = follow;
	    b->nfollow = code->size;
	    break;

	case Code::SWITCH_STRING:
	    /* create jump table with block entry points */
	    b = b->find(code->addr);
	    follow = new Block*[code->size];
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = b->split(code->caseString[i].addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
	    }

	    /* start new block */
	    if (code->next != NULL) {
		b = b->split(code->next->addr);
		if (b == NULL) {
		    delete[] follow;
		    clear(first);
		    return NULL;
		}
	    }

	    /* set followups */
	    b = b->find(code->addr);
	    b->follow = follow;
	    b->nfollow = code->size;
	    break;

	default:
	    break;
	}
    }

    return first;
}

/*
 * add block to the list
 */
void Block::push(Block **list, int offset)
{
    if (f.start == UNLISTED) {
	f.list = *list;
	*list = this;
	f.start = offset;
    } else if (f.start != offset) {
	fatal("catch/rlimits branch mismatch");
    }
}

/*
 * transform RETURN into END_CATCH or END_RLIMITS, as required
 */
void Block::analyze(Block *b)
{
    Block *list;
    CodeSize size;
    Code *code;
    int c, i;

    list = b;

    /* initially, no blocks are in the list */
    while (b != NULL) {
	b->f.start = b->f.end = UNLISTED;
	size = b->last->addr;
	b = b->next;
    }

    Context context[size];	/* too large but we need some limit */
    c = NONE;

    list->f.start = c;
    list->f.list = NULL;

    /*
     * determine the catch/rlimits context at the start and end of each block
     */
    while (list != NULL) {
	b = list;
	list = b->f.list;

	if (b->f.end != UNLISTED) {
	    fatal("block list corrupted");
	}
	b->f.end = b->f.start;

	/*
	 * go through the code and make adjustments as needed
	 */
	for (code = b->first; ; code = code->next) {
	    switch (code->instruction) {
	    case Code::CATCH:
		context[++c].type = Context::CATCH;
		context[c].next = b->f.end;
		b->f.end = c;
		break;

	    case Code::RLIMITS:
	    case Code::RLIMITS_CHECK:
		context[++c].type = Context::RLIMITS;
		context[c].next = b->f.end;
		b->f.end = c;
		break;

	    case Code::RETURN:
		if (b->f.end != NONE) {
		    if (context[b->f.end].type == Context::CATCH) {
			code->instruction = Code::END_CATCH;
		    } else {
			code->instruction = Code::END_RLIMITS;
		    }
		    b->f.end = context[b->f.end].next;
		}

	    default:
		break;
	    }

	    if (code == b->last) {
		break;
	    }
	}

	if (b->nfollow != 0) {
	    /* add followups to the list */
	    if (code->instruction == Code::CATCH) {
		/*
		 * special case for CATCH: the context will be gone after an
		 * exception occurs
		 */
		b->follow[0]->push(&list, b->f.end);
		b->follow[1]->push(&list, context[b->f.end].next);
	    } else {
		for (i = 0; i < b->nfollow; i++) {
		    b->follow[i]->push(&list, b->f.end);
		}
	    }
	} else if (b->next != NULL && code->instruction != Code::RETURN) {
	    /* add the next block to the list */
	    b->next->push(&list, b->f.end);
	} else if (b->f.end != NONE) {
	    /* function ended within a catch/rlimits context */
	    fatal("catch/rlimits return mismatch");
	}
    }
}
