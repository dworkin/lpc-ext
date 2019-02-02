class TypeVal {
public:
    TypeVal() { }
    TypeVal(Type type, LPCInt val) : type(type), val(val) { }

    Type type;
    LPCInt val;
};

class BlockContext {
public:
    BlockContext(CodeFunction *func, StackSize size);
    virtual ~BlockContext();

    void push(TypeVal val);
    void push(Type type, LPCInt val = 0) {
	push(TypeVal(type, val));
    }
    TypeVal pop();
    TypeVal top();
    void setParam(LPCParam param, TypeVal val);
    void setParam(LPCParam param, Type type, LPCInt val = 0) {
	setParam(param, TypeVal(type, val));
    }
    void setLocal(LPCLocal local, TypeVal val);
    void setLocal(LPCLocal local, Type type, LPCInt val = 0) {
	setLocal(local, TypeVal(type, val));
    }
    TypeVal getParam(LPCParam param);
    TypeVal getLocal(LPCLocal local);
    TypeVal indexed();
    void stores(int count, bool pop);
    void storeN();
    void castX(Type type) {
	castType = type;
    }
    Type typeX() {
	return castType;
    }
    void spread() {
	spreadArgs = true;
    }
    Type kfun(LPCKFunc *kf);
    void args(int nargs);
    void setStackPointer(StackSize stackPointer) {
	sp = stackPointer;
    }
    StackSize stackPointer() {
	return sp;
    }
    StackSize depth(StackSize stackPointer);
    Type topType(StackSize stackPointer);

private:
    Type *params;
    Type *locals;
    int nParams;
    int nLocals;
    Stack<TypeVal> *stack;
    StackSize sp;
    Type castType;
    int storeCount;
    bool storePop;
    bool spreadArgs;
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

    StackSize sp;
};

class TypedBlock : public Block {
public:
    TypedBlock(Code *first, Code *last, CodeSize size);
    virtual ~TypedBlock();

    virtual BlockContext *evaluate(CodeFunction *func, StackSize size);

    static Block *create(Code *first, Code *last, CodeSize size);
};
