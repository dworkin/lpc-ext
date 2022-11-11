class BlockContext {
public:
    BlockContext();
    virtual ~BlockContext();

    bool stores(int count, Code *popCode, bool flag);
    bool lval() {
	return lvalue;
    }
    bool storeN();
    Code *storePop() {
	return storeCode;
    }

private:
    int storeCount;		/* number of STOREX instructions left */
    Code *storeCode;		/* pop at end of STORES? */
    bool lvalue;		/* lvalue stores? */
};

class Block {
public:
    Block(Code *first, Code *last, CodeSize size);
    virtual ~Block();

    void startVisits(Block **list);
    void startAllVisits(Block **list);
    void toVisit(Block **list);

    void setRelay();
    bool relay();
    bool relayToDefault(Block *to);
    CodeSize fragment();
    void clear();
    virtual void setContext(class TypedContext *context, Block *b);
    virtual Type paramType(LPCParam param);
    virtual int paramIn(LPCParam param);
    virtual int paramOut(LPCParam param);
    virtual bool paramMerged(LPCParam param);
    virtual Type localType(LPCLocal local);
    virtual int localIn(LPCLocal local);
    virtual int localOut(LPCLocal local);
    virtual bool localMerged(LPCLocal local);
    virtual Type mergedParamType(LPCParam param, Type type);
    virtual Type mergedLocalType(LPCLocal local);
    virtual void prepareFlow(class FlowContext *context);
    virtual void evaluateTypes(class TypedContext *context, Block **list);
    virtual void evaluateFlow(class FlowContext *context, Block **list);
    virtual void evaluateInputs(class FlowContext *context, Block **list);
    virtual void evaluateOutputs(class FlowContext *context, Block **list);
    virtual void emit(class GenContext *context, CodeFunction *function);

    static Block *function(CodeFunction *function);
    static Block *nextVisit(Block **List);

    static Block *produce(Code *first, Code *last, CodeSize size);
    static void producer(Block *(*factory)(Code*, Code*, CodeSize));

    Code *first, *last;			/* first and last code in block */
    Block *next;			/* next block */
    Block *caught;			/* catch context at start of block */
    Block **from;			/* entrance blocks */
    bool *fromVisit;			/* entrance flags */
    Block **to;				/* following blocks */
    CodeSize nFrom;			/* # entrance blocks */
    CodeSize nTo;			/* # following blocks */
    CodeSize size;			/* size of block */
    StackSize sp;			/* stack pointer */
    StackSize endSp;			/* final stack pointer */

private:
    enum Context {
	CATCH,
	RLIMITS
    };

    Block *find(CodeSize addr);
    Block *split(CodeSize addr);
    void toVisitOnce(Block **list, StackSize stackPointer, Block *catchContext);
    Block *pass1();
    Block *pass2(Block *tree, StackSize size);
    void pass3(Block *b);
    void pass4();

    Block *left, *right;		/* left and right child in tree */
    Block *visit;			/* next in visit list */
    uint16_t flags;			/* flag bits */

    static Block *(*factory)(Code*, Code*, CodeSize);
};

# define BLOCK_DEFAULT			0x0001
# define BLOCK_RELAY			0x0002
