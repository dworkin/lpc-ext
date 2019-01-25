class DisCode : public TypedCode {
public:
    DisCode(CodeFunction *function);
    virtual ~DisCode();

    virtual CodeLine emit(CodeLine line);

    static Code *create(CodeFunction *function);
};

class DisBlock : public TypedBlock {
public:
    DisBlock(Code *fist, Code *last, CodeSize size);
    virtual ~DisBlock();

    virtual void emit(BlockContext *context);

    static Block *create(Code *first, Code *last, CodeSize size);
};
