class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    virtual ~Block();

    void startVisits(Block **list);
    bool visited();
    void toVisit(Block **list, StackSize stackPointer);
    Block *nextVisit(Block **List);
    StackSize stackPointer() {
	return sp;
    }

    CodeSize fragment();
    void clear();
    virtual class BlockContext *evaluate(StackSize size);
    virtual void emit(BlockContext *context);

    static Block *function(CodeFunction *function);

    static Block *create(Code *first, Code *last, CodeSize size);
    static Block *produce(Code *first, Code *last, CodeSize size);
    static void producer(Block *(*factory)(Code*, Code*, CodeSize));

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block **from;			/* entrance blocks */
    Block **to;				/* following blocks */
    CodeSize nFrom;			/* # entrance blocks */
    CodeSize nTo;			/* # following blocks */
    CodeSize size;			/* size of block */

private:
    enum Context {
	CATCH,
	RLIMITS
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void toVisitOnce(Block **list, StackSize stackPointer);
    Block *pass1();
    Block *pass2(Block *tree, StackSize size);
    void pass3(Block *b);
    void pass4();

    Block *left, *right;		/* left and right child in tree */
    StackSize sp;			/* start stack pointer */
    Block *visit;			/* next in visit list */

    static Block *(*factory)(Code*, Code*, CodeSize);
};
