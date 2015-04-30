# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
# include "lpc_ext.h"
# include "data.h"
# include "code.h"
# include "block.h"
# include "jit.h"


/*
 * NAME:	Block->new()
 * DESCRIPTION:	allocate new block
 */
static Block *block_new(Code *first, Code *last, CodeSize size)
{
    Block *b;

    b = alloc(Block, 1);
    b->first = first;
    b->last = last;
    b->size = size;
    b->nfollow = 0;

    return b;
}

/*
 * NAME:	Block->find()
 * DESCRIPTION:	find a block
 */
static Block *block_find(Block *n, CodeSize addr)
{
    Block b, *t, *l, *r;

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
 * NAME:	Block->split()
 * DESCRIPTION:	find and possibly split a block
 */
static Block *block_split(Block *b, CodeSize addr)
{
    b = block_find(b, addr);
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
	n = block_new(last->next, b->last, b->size - size);
	n->next = b->next;
	n->follow = b->follow;
	n->nfollow = b->nfollow;
	b->next = n;
	b->last = last;
	b->size = size;
	b->follow = NULL;
	b->nfollow = 0;

	/* insert at root of tree */
	n->left = b;
	n->right = b->right;
	b->right = NULL;
	b = n;
    }

    return b;
}

/*
 * NAME:	Block->clear()
 * DESCRIPTION:	clear a list of blocks
 */
void block_clear(Block *b)
{
    Block *next;

    while (b != NULL) {
	next = b->next;
	if (b->follow != NULL) {
	    free(b->follow);
	}
	free(b);
	b = next;
    }
}

/*
 * NAME:	Block->function()
 * DESCRIPTION:	break up a function into code blocks
 */
Block *block_function(CodeFunction *function)
{
    Code *code;
    Block *first, *b, **follow;
    int i;

    code = function->list;
    first = block_new(code, function->last, function->pc);

    for (b = first; code != NULL; code = code->next) {
	switch (code->instruction) {
	case CODE_JUMP:
	    /* split at jump target */
	    b = block_split(b, code->u.addr);
	    if (b == NULL) {
		block_clear(first);
		return NULL;
	    }
	    /* one followup */
	    follow = alloc(Block*, 1);
	    follow[0] = b;
	    b = block_find(b, code->addr);
	    b->follow = follow;
	    b->nfollow = 1;

	    if (code->next != NULL) {
		/* new block starts after jump */
		b = block_split(b, code->next->addr);
		if (b == NULL) {
		    block_clear(first);
		    return NULL;
		}
	    }
	    break;

	case CODE_JUMP_ZERO:
	case CODE_JUMP_NONZERO:
	case CODE_CATCH:
	    /* split at jump target */
	    b = block_split(b, code->u.addr);
	    if (b == NULL) {
		block_clear(first);
		return NULL;
	    }
	    /* two followups */
	    follow = alloc(Block*, 2);
	    follow[1] = b;
	    b = block_find(b, code->addr);
	    b->follow = follow;
	    b->nfollow = 2;
	    b = block_split(b, code->next->addr);
	    if (b == NULL) {
		block_clear(first);
		return NULL;
	    }
	    follow[0] = b;
	    break;

	case CODE_SWITCH_INT:
	    /* create jump table with block entry points */
	    b = block_find(b, code->addr);
	    b->follow = follow = alloc(Block*, code->size);
	    b->nfollow = code->size;
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = block_split(b, code->u.caseInt[i].addr);
		if (b == NULL) {
		    block_clear(first);
		    return NULL;
		}
	    }
	    break;

	case CODE_SWITCH_RANGE:
	    /* create jump table with block entry points */
	    b = block_find(b, code->addr);
	    b->follow = follow = alloc(Block*, code->size);
	    b->nfollow = code->size;
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = block_split(b, code->u.caseRange[i].addr);
		if (b == NULL) {
		    block_clear(first);
		    return NULL;
		}
	    }
	    break;

	case CODE_SWITCH_STRING:
	    /* create jump table with block entry points */
	    b = block_find(b, code->addr);
	    b->follow = follow = alloc(Block*, code->size);
	    b->nfollow = code->size;
	    for (i = 0; i < code->size; i++) {
		follow[i] = b = block_split(b, code->u.caseString[i].addr);
		if (b == NULL) {
		    block_clear(first);
		    return NULL;
		}
	    }
	    break;

	default:
	    break;
	}
    }

    return first;
}
