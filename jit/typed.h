class TypeVal {
public:
    TypeVal() { }
    TypeVal(Type type, LPCInt val) : type(type), val(val) { }

    Type type;		/* type */
    LPCInt val;		/* value, only relevant for integer on the stack */
};

class BlockContext {
public:
    BlockContext(CodeFunction *func, StackSize size);
    virtual ~BlockContext();

    void prologue(Type *mergeParams, Type *mergeLocals, StackSize mergeSp,
		  Block *b);
    void push(TypeVal val);
    void push(Type type, LPCInt val = 0) {
	push(TypeVal(type, val));
    }
    TypeVal pop();
    TypeVal top();
    TypeVal indexed();
    void stores(int count, bool pop);
    void storeN();
    void spread() {
	spreadArgs = true;
    }
    Type kfun(LPCKFunc *kf);
    void args(int nargs);
    StackSize merge(StackSize codeSp);
    bool changed();
    StackSize depth(StackSize stackPointer);
    Type topType(StackSize stackPointer);

    Type *params;		/* function parameter types */
    Type *locals;		/* function local variable types */
    LPCParam nParams;		/* # parameters */
    LPCLocal nLocals;		/* # local variables */
    StackSize sp;		/* stack pointer */
    Type castType;		/* CASTX argument */

private:
    static Type mergeType(Type type1, Type type2);

    Type *origParams;		/* original parameter types */
    Type *origLocals;		/* original local variable types */
    Stack<TypeVal> *stack;	/* type stack */
    TypeVal altStack[3];	/* alternative stack */
    StackSize altSp;		/* alternative stack pointer */
    int storeCount;		/* number of STOREX instructions left */
    bool storePop;		/* pop at end of STORES? */
    bool spreadArgs;		/* SPREAD before call? */
    bool merging;		/* merging stack values? */
};

class TypedCode : public Code {
public:
    TypedCode(CodeFunction *function);
    virtual ~TypedCode();

    virtual void evaluate(BlockContext *context);
    StackSize stackPointer() { return sp; }

    static Code *create(CodeFunction *function);

private:
    static Type simplifiedType(Type type);

    StackSize sp;		/* stack pointer */
};

class TypedBlock : public Block {
public:
    TypedBlock(Code *first, Code *last, CodeSize size);
    virtual ~TypedBlock();

    virtual void setContext(BlockContext *context, Block *b);
    virtual void evaluate(BlockContext *context, Block **list);
    virtual BlockContext *evaluate(CodeFunction *func, StackSize size);

    static Block *create(Code *first, Code *last, CodeSize size);

private:
    Type *params;		/* parameter types at end of block */
    Type *locals;		/* local variable types at end of block */
};
