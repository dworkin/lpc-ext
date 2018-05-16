class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    ~Block();

    static Block *create(CodeFunction *func);

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block **follow;			/* following blocks */
    uint16_t nfollow;			/* # following blocks */
    CodeSize size;			/* size of block */

private:
    Block *find(CodeSize addr);
    Block *split(CodeSize addr);

    static void clear(Block *b);

    union {
	struct {
	    Block *left, *right;	/* left and right child in tree */
	} t;
	struct {
	    struct FlowData *flow;	/* flow data */
	} d;
    };
};
