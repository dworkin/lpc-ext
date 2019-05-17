class ClangCode : public FlowCode {
public:
    ClangCode(CodeFunction *function);
    virtual ~ClangCode();

    virtual void emit(FlowContext *context);

    static bool onStack(FlowContext *context, StackSize sp);
    static char *tmpRef(StackSize sp);
    static char *paramRef(LPCParam param, int ref);
    static char *localRef(LPCLocal local, int ref);
    static Code *create(CodeFunction *function);

private:
    char *paramRef(FlowContext *context, LPCParam param);
    char *localRef(FlowContext *context, LPCLocal local);
    void result(FlowContext *context);
};

class ClangBlock : public FlowBlock {
public:
    ClangBlock(Code *fist, Code *last, CodeSize size);
    virtual ~ClangBlock();

    virtual void emit(CodeFunction *function, CodeSize size);

    static Block *create(Code *first, Code *last, CodeSize size);
};
