class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    virtual ~Block();

    void startVisits(Block **list);
    void toVisit(Block **list);
    Block *nextVisit(Block **List);

    CodeSize fragment();
    void clear();
    virtual void setContext(class BlockContext *context, Block *b);
    virtual void evaluate(class BlockContext *context, Block **list);
    virtual class BlockContext *evaluate(CodeFunction *func, StackSize size);
    virtual void emit(BlockContext *context);

    static Block *function(CodeFunction *function);

    static Block *create(Code *first, Code *last, CodeSize size);
    static Block *produce(Code *first, Code *last, CodeSize size);
    static void producer(Block *(*factory)(Code*, Code*, CodeSize));

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block **from;			/* entrance blocks */
    bool *fromVisit;			/* entrance flags */
    Block **to;				/* following blocks */
    CodeSize nFrom;			/* # entrance blocks */
    CodeSize nTo;			/* # following blocks */
    CodeSize size;			/* size of block */
    StackSize sp;			/* stack pointer */
    StackSize level;			/* catch level */

private:
    enum Context {
	CATCH,
	RLIMITS
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void toVisitOnce(Block **list, StackSize stackPointer,
		     StackSize catchLevel);
    Block *pass1();
    Block *pass2(Block *tree, StackSize size);
    void pass3(Block *b);
    void pass4();

    Block *left, *right;		/* left and right child in tree */
    Block *visit;			/* next in visit list */

    static Block *(*factory)(Code*, Code*, CodeSize);
};
