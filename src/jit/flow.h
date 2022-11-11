class FlowContext : public TypedContext {
public:
    FlowContext(CodeFunction *func, StackSize size);
    virtual ~FlowContext();

    int paramRef(LPCParam param);
    int localRef(LPCLocal local);

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

    int inputRef() {
	return in;
    }
    int outputRef() {
	return out;
    }

private:
    int in;			/* variable input reference */
    int out;			/* variable output reference */
};

class FlowBlock : public TypedBlock {
public:
    FlowBlock(Code *fist, Code *last, CodeSize size);
    virtual ~FlowBlock();

    virtual int paramIn(LPCParam param);
    virtual int paramOut(LPCParam param);
    virtual bool paramMerged(LPCParam param);
    virtual int localIn(LPCLocal local);
    virtual int localOut(LPCLocal local);
    virtual bool localMerged(LPCLocal local);
    virtual Type mergedParamType(LPCParam param, Type type);
    virtual Type mergedLocalType(LPCLocal local);
    virtual void prepareFlow(FlowContext *context);
    virtual void evaluateFlow(FlowContext *context, Block **list);
    virtual void evaluateInputs(FlowContext *context, Block **list);
    virtual void evaluateOutputs(FlowContext *context, Block **list);
    void evaluate(FlowContext *context);

private:
    int *inParams;		/* parameter input references */
    int *inLocals;		/* local input references */
    int *outParams;		/* parameter output references */
    int *outLocals;		/* local output references */
};
