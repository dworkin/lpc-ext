class TVC {
public:
    TVC() { }
    TVC(Type type, LPCInt val) :
	type(type), val(val), code(NULL), ref(STACK_EMPTY) { }

    Type type;		/* type */
    LPCInt val;		/* value, only relevant for integer on the stack */
    Code *code;		/* instruction that consumes this TVC */
    StackSize ref;	/* replacement reference */
};

class TypedContext : public BlockContext {
public:
    TypedContext(CodeFunction *func, StackSize size);
    virtual ~TypedContext();

    void prologue(Type *mergeParams, Type *mergeLocals, StackSize mergeSp,
		  Block *b);
    void push(TVC val);
    void push(Type type, LPCInt val = 0) {
	push(TVC(type, val));
    }
    TVC pop(Code *code);
    TVC indexed();
    void storeParam(LPCParam param, Type type);
    void storeLocal(LPCParam local, Type type);
    void endStores();
    void spread() {
	spreadArgs = true;
    }
    Type kfun(LPCKFunCall *kf, Code *code);
    void args(int nargs, Code *code);
    void startCatch();
    void modCaught();
    void endCatch();
    StackSize merge(StackSize codeSp);
    bool changed(Type *params, Type *locals);
    TVC get(StackSize stackPointer);
    Code *consumer(StackSize stackPointer, Type *type);
    StackSize nextSp(StackSize stackPointer, int depth = 1);

    Type *params;		/* function parameter types */
    Type *locals;		/* function local variable types */
    LPCParam nParams;		/* # parameters */
    LPCLocal nLocals;		/* # local variables */
    StackSize sp;		/* stack pointer */
    Type castType;		/* CASTX argument */
    Block *block;		/* current block */
    Block *caught;		/* catch context */
    Block **list;		/* visitation list */

private:
    StackSize copyStack(StackSize copy, StackSize from, StackSize to);

    static Type mergeType(Type type1, Type type2);

    Stack<TVC> *stack;		/* type/val/code stack */
    TVC altStack[3];		/* alternative stack */
    StackSize altSp;		/* alternative stack pointer */
    bool spreadArgs;		/* SPREAD before call? */
    bool merging;		/* merging stack values? */
};

class TypedCode : public Code {
public:
    TypedCode(CodeFunction *function);
    virtual ~TypedCode();

    virtual void evaluateTypes(TypedContext *context);
    StackSize stackPointer() { return sp; }

    static Type simplifiedType(Type type);
    static Type offStack(TypedContext *context, StackSize stackPointer);

    Type varType;		/* STORES param/local type */

private:
    StackSize sp;		/* stack pointer */
};

class TypedBlock : public Block {
public:
    TypedBlock(Code *first, Code *last, CodeSize size);
    virtual ~TypedBlock();

    virtual Type paramType(LPCParam param);
    virtual Type localType(LPCLocal local);
    virtual void setContext(TypedContext *context, Block *b);
    virtual void evaluateTypes(TypedContext *context, Block **list);
    void evaluate(TypedContext *context);

private:
    Type *params;		/* parameter types at end of block */
    Type *locals;		/* local variable types at end of block */
};
