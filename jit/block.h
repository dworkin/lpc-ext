class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    virtual ~Block();

    void startVisits(Block **list);
    bool visited();
    void toVisit(Block **list, StackSize offset);
    Block *nextVisit(Block **List, StackSize offset);
    StackSize offset() {
	return start;
    }

    void clear();
    virtual void evaluate(class BlockContext *context);
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
    enum Context {
	CATCH,
	RLIMITS
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void toVisitOnce(Block **list, StackSize offset);
    static Block *pass0(CodeFunction *function);
    Block *pass1();
    void pass2(StackSize size);
    void pass3(Block *b);
    void pass4();

    Block *left, *right;		/* left and right child in tree */
    StackSize start, end;		/* start and end offset */
    Block *visit;			/* next in visit list */

    static Block *(*factory)(Code*, Code*, CodeSize);
};
