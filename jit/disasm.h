class DisCode : public FlowCode {
public:
    DisCode(CodeFunction *function);
    virtual ~DisCode();

    virtual void emit(FlowContext *context);

    static Code *create(CodeFunction *function);
};

class DisBlock : public FlowBlock {
public:
    DisBlock(Code *first, Code *last, CodeSize size);
    virtual ~DisBlock();

    virtual void emit(CodeFunction *function, CodeSize size);

    static Block *create(Code *first, Code *last, CodeSize size);
};
