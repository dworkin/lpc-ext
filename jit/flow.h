class FlowContext : public BlockContext {
public:
    FlowContext(CodeFunction *func, StackSize size);
    virtual ~FlowContext();

    static const int INITIAL =	0x20000;
    static const int NEEDED =	0x20001;

    int *inParams, *outParams;	/* parameter references */
    int *inLocals, *outLocals;	/* local references */
};

class FlowCode : public TypedCode {
public:
    FlowCode(CodeFunction *function);
    virtual ~FlowCode();

    virtual void evaluateFlow(FlowContext *context);

    static Code *create(CodeFunction *function);

    int reference() {
	return ref;
    }

private:
    int ref;			/* variable reference */
};

class FlowBlock : public TypedBlock {
public:
    FlowBlock(Code *fist, Code *last, CodeSize size);
    virtual ~FlowBlock();

    virtual int paramRef(LPCParam param);
    virtual int localRef(LPCLocal local);
    virtual void prepareFlow(FlowContext *context);
    virtual void evaluateFlow(FlowContext *context, Block **list);
    virtual void evaluateInputs(FlowContext *context, Block **list);
    virtual void evaluateOutputs(FlowContext *context, Block **list);
    void evaluate(FlowContext *context);

    static Block *create(Code *first, Code *last, CodeSize size);

private:
    int *inParams;		/* parameter input references */
    int *inLocals;		/* local input references */
    int *outParams;		/* parameter output references */
    int *outLocals;		/* local output references */
};
