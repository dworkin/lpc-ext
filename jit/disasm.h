class DisCode : public FlowCode {
public:
    DisCode(CodeFunction *function);
    virtual ~DisCode();

    virtual void emit(class GenContext *context);

    static Code *create(CodeFunction *function);

private:
    void emitType(GenContext *context, LPCType *type);
};

class DisBlock : public FlowBlock {
public:
    DisBlock(Code *first, Code *last, CodeSize size);
    virtual ~DisBlock();

    virtual void emit(FILE *stream, CodeFunction *function, CodeSize size);

    static Block *create(Code *first, Code *last, CodeSize size);
};
