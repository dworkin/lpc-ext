class ClangCode : public FlowCode {
public:
    ClangCode(CodeFunction *function);
    virtual ~ClangCode();

    virtual void emit(class GenContext *context);
    void emitRangeTable(class GenContext *context);
    void emitStringTable(class GenContext *context);

    static void intArg(class GenContext *context, StackSize sp);
    static void floatArg(class GenContext *context, StackSize sp);
    static char *tmpRef(StackSize sp);
    static char *paramRef(LPCParam param, int ref);
    static char *localRef(LPCLocal local, int ref);
    static Code *create(CodeFunction *function);

    ClangCode *list;		/* next in switch list */

private:
    char *paramRef(class GenContext *context, LPCParam param);
    char *localRef(class GenContext *context, LPCLocal local);
    char *localPre(class GenContext *context, LPCLocal local);
    void pushResult(class GenContext *context);
    void popResult(class GenContext *context);
    void popStores(class GenContext *context, StackSize sp);
    void switchInt(class GenContext *context);
    void genTable(class GenContext *context, const char *type);
};

class ClangBlock : public FlowBlock {
public:
    ClangBlock(Code *fist, Code *last, CodeSize size);
    virtual ~ClangBlock();

    virtual void emit(class GenContext *context, CodeFunction *function);

    static Block *create(Code *first, Code *last, CodeSize size);
};

class ClangObject {
public:
    ClangObject(CodeObject *object, CodeByte *prog, int nFunctions);
    virtual ~ClangObject();

    bool emit(char *base, int flags);

private:
    void header(FILE *stream);
    void table(FILE *stream, int nFunctions);

    CodeObject *object;		/* object being compiled */
    CodeByte *prog;		/* LPC bytecode */
    int nFunctions;		/* # functions in object */
};
