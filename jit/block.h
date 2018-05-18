class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    ~Block();

    static Block *create(CodeFunction *func);
    static void analyze(Block *b);
    static void clear(Block *b);

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block **follow;			/* following blocks */
    uint16_t nfollow;			/* # following blocks */
    CodeSize size;			/* size of block */

private:
    struct Context {
	enum {
	    CATCH,
	    RLIMITS
	} type;
	int next;
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void push(Block **list, int offset);

    static const int UNLISTED = 0;
    static const int NONE = 1;
    union {
	struct {
	    Block *left, *right;	/* left and right child in tree */
	} t;
	struct {
	    int start, end;
	    Block *list;
	} f;
    };
};
