typedef struct Block {
    union {
	struct {
	    struct Block *left, *right;	/* left and right child in tree */
	} t;
	struct {
	    struct FlowData *flow;	/* flow data */
	} d;
    } u;
    Code *first, *last;			/* first and last code in block */
    struct Block *next;			/* next block */
    struct Block **follow;		/* following blocks */
    uint16_t nfollow;			/* # following blocks */
    CodeSize size;			/* size of block */
} Block;

extern Block *block_function	(CodeFunction*);
