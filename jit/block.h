class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    virtual ~Block();

    void clear();
    virtual void emit();

    static Block *blocks(CodeFunction *func);

    static Block *create(Code *first, Code *last, CodeSize size);
    static Block *produce(Code *first, Code *last, CodeSize size);
    static void producer(Block *(*factory)(Code*, Code*, CodeSize));

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block **from;			/* entrance blocks */
    Block **to;				/* following blocks */
    uint16_t nFrom;			/* # entrance blocks */
    uint16_t nTo;			/* # following blocks */
    CodeSize size;			/* size of block */

private:
    struct Context {
	enum {
	    CATCH,
	    RLIMITS
	} type;
	CodeSize next;
    };
    enum {
	UNLISTED,
	NONE
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void track(Block **list, CodeSize offset);
    Block *pass1();
    void pass2();
    void pass3(Block *b);
    void pass4();

    Block *left, *right;		/* left and right child in tree */
    CodeSize start, end;		/* start and end context */
    Block *list;

    static Block *(*factory)(Code*, Code*, CodeSize);
};
