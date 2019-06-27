# include <stdlib.h>
# include <stdint.h>
# include <new>
# include <math.h>
# include <stdio.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "instruction.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "flow.h"
# include "gentt.h"
# include "genclang.h"
# include "jitcomp.h"

# define Int	"i32"
# define Double	"double"

static const struct {
    const char *name;			/* function name */
    const char *ret;			/* return value */
    const char *args;			/* function arguments */
} functions[] = {
# define VM_INT				0
    { "vm_int", "void", "(i8*, " Int ")" },
# define VM_FLOAT			1
    { "vm_float", "void", "(i8*, " Double ")" },
# define VM_STRING			2
    { "vm_string", "void", "(i8*, i32, i32)" },
# define VM_PARAM			3
    { "vm_param", "void", "(i8*, i32)" },
# define VM_PARAM_INT			4
    { "vm_param_int", Int, "(i8*, i32)" },
# define VM_PARAM_FLOAT			5
    { "vm_param_float", Double, "(i8*, i32)" },
# define VM_LOCAL			6
    { "vm_local", "void", "(i8*, i32)" },
# define VM_GLOBAL			7
    { "vm_global", "void", "(i8*, i32, i32)" },
# define VM_GLOBAL_INT			8
    { "vm_global_int", Int, "(i8*, i32, i32)" },
# define VM_GLOBAL_FLOAT		9
    { "vm_global_float", Double, "(i8*, i32, i32)" },
# define VM_INDEX			10
    { "vm_index", "void", "(i8*, i1)" },
# define VM_INDEX_INT			11
    { "vm_index_int", Int, "(i8*, i1)" },
# define VM_INDEX2			12
    { "vm_index2", "void", "(i8*, i1)" },
# define VM_INDEX2_INT			13
    { "vm_index2_int", Int, "(i8*, i1)" },
# define VM_AGGREGATE			14
    { "vm_aggregate", "void", "(i8*, i32)" },
# define VM_MAP_AGGREGATE		15
    { "vm_map_aggregate", "void", "(i8*, i32)" },
# define VM_CAST			16
    { "vm_cast", "void", "(i8*, i32, i32, i32)" },
# define VM_CAST_INT			17
    { "vm_cast_int", Int, "(i8*)" },
# define VM_CAST_FLOAT			18
    { "vm_cast_float", Double, "(i8*)" },
# define VM_INSTANCEOF			19
    { "vm_instanceof", Int, "(i8*, i32, i32)" },
# define VM_RANGE			20
    { "vm_range", "void", "(i8*)" },
# define VM_RANGE_FROM			21
    { "vm_range_from", "void", "(i8*)" },
# define VM_RANGE_TO			22
    { "vm_range_to", "void", "(i8*)" },
# define VM_STORE_PARAM			23
    { "vm_store_param", "void", "(i8*, i32)" },
# define VM_STORE_PARAM_INT		24
    { "vm_store_param_int", "void", "(i8*, i32, " Int ")" },
# define VM_STORE_PARAM_FLOAT		25
    { "vm_store_param_float", "void", "(i8*, i32, " Double ")" },
# define VM_STORE_LOCAL			26
    { "vm_store_local", "void", "(i8*, i32)" },
# define VM_STORE_LOCAL_INT		27
    { "vm_store_local_int", "void", "(i8*, i32, " Int ")" },
# define VM_STORE_LOCAL_FLOAT		28
    { "vm_store_local_float", "void", "(i8*, i32, " Double ")" },
# define VM_STORE_GLOBAL		29
    { "vm_store_global", "void", "(i8*, i32, i32)" },
# define VM_STORE_GLOBAL_INT		30
    { "vm_store_global_int", "void", "(i8*, i32, i32, " Int ")" },
# define VM_STORE_GLOBAL_FLOAT		31
    { "vm_store_global_float", "void", "(i8*, i32, i32, " Double ")" },
# define VM_STORE_INDEX			32
    { "vm_store_index", "void", "(i8*)" },
# define VM_STORE_PARAM_INDEX		33
    { "vm_store_param_index", "void", "(i8*, i32)" },
# define VM_STORE_LOCAL_INDEX		34
    { "vm_store_local_index", "void", "(i8*, i32)" },
# define VM_STORE_GLOBAL_INDEX		35
    { "vm_store_global_index", "void", "(i8*, i32, i32)" },
# define VM_STORE_INDEX_INDEX		36
    { "vm_store_index_index", "void", "(i8*)" },
# define VM_STORES			37
    { "vm_stores", "void", "(i8*, i32)" },
# define VM_STORES_LVAL			38
    { "vm_stores_lval", "void", "(i8*, i32)" },
# define VM_STORES_SPREAD		39
    { "vm_stores_spread", "void", "(i8*, i32, i32, i32, i32)" },
# define VM_STORES_CAST			40
    { "vm_stores_cast", "void", "(i8*, i32, i32, i32)" },
# define VM_STORES_PARAM		41
    { "vm_stores_param", "void", "(i8*, i32)" },
# define VM_STORES_PARAM_INT		42
    { "vm_stores_param_int", Int, "(i8*, i32)" },
# define VM_STORES_PARAM_FLOAT		43
    { "vm_stores_param_float", Double, "(i8*, i32)" },
# define VM_STORES_LOCAL		44
    { "vm_stores_local", "void", "(i8*, i32)" },
# define VM_STORES_LOCAL_INT		45
    { "vm_stores_local_int", Int, "(i8*, i32)" },
# define VM_STORES_LOCAL_FLOAT		46
    { "vm_stores_local_float", Double, "(i8*, i32)" },
# define VM_STORES_GLOBAL		47
    { "vm_stores_global", "void", "(i8*, i32, i32)" },
# define VM_STORES_INDEX		48
    { "vm_stores_index", "void", "(i8*)" },
# define VM_STORES_PARAM_INDEX		49
    { "vm_stores_param_index", "void", "(i8*, i32)" },
# define VM_STORES_LOCAL_INDEX		50
    { "vm_stores_local_index", "void", "(i8*, i32)" },
# define VM_STORES_GLOBAL_INDEX		51
    { "vm_stores_global_index", "void", "(i8*, i32, i32)" },
# define VM_STORES_INDEX_INDEX		52
    { "vm_stores_index_index", "void", "(i8*)" },
# define VM_DIV_INT			53
    { "vm_div_int", Int, "(i8*, " Int ", " Int ")" },
# define VM_LSHIFT_INT			54
    { "vm_lshift_int", Int, "(i8*, " Int ", " Int ")" },
# define VM_MOD_INT			55
    { "vm_mod_int", Int, "(i8*, " Int ", " Int ")" },
# define VM_RSHIFT_INT			56
    { "vm_rshift_int", Int, "(i8*, " Int ", " Int ")" },
# define VM_TOFLOAT			57
    { "vm_tofloat", Double, "(i8*)" },
# define VM_TOINT			58
    { "vm_toint", Int, "(i8*)" },
# define VM_TOINT_FLOAT			59
    { "vm_toint_float", Int, "(i8*, " Double ")" },
# define VM_NIL				60
    { "vm_nil", Int, "(i8*)" },
# define VM_ADD_FLOAT			61
    { "vm_add_float", Double, "(i8*, " Double ", " Double ")" },
# define VM_DIV_FLOAT			62
    { "vm_div_float", Double, "(i8*, " Double ", " Double ")" },
# define VM_MULT_FLOAT			63
    { "vm_mult_float", Double, "(i8*, " Double ", " Double ")" },
# define VM_SUB_FLOAT			64
    { "vm_sub_float", Double, "(i8*, " Double ", " Double ")" },
# define VM_KFUNC			65
    { "vm_kfunc", "void", "(i8*, i32, i32)" },
# define VM_KFUNC_INT			66
    { "vm_kfunc_int", Int, "(i8*, i32, i32)" },
# define VM_KFUNC_FLOAT			67
    { "vm_kfunc_float", Double, "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD		68
    { "vm_kfunc_spread", "void", "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD_INT		69
    { "vm_kfunc_spread_int", Int, "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD_FLOAT		70
    { "vm_kfunc_spread_float", Double, "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD_LVAL		71
    { "vm_kfunc_spread_lval", "void", "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD_LVAL_INT	72
    { "vm_kfunc_spread_lval_int", Int, "(i8*, i32, i32)" },
# define VM_KFUNC_SPREAD_LVAL_FLOAT	73
    { "vm_kfunc_spread_lval_float", Double, "(i8*, i32, i32)" },
# define VM_DFUNC			74
    { "vm_dfunc", "void", "(i8*, i32, i32)" },
# define VM_DFUNC_INT			75
    { "vm_dfunc_int", Int, "(i8*, i32, i32)" },
# define VM_DFUNC_FLOAT			76
    { "vm_dfunc_float", Double, "(i8*, i32, i32)" },
# define VM_DFUNC_SPREAD		77
    { "vm_dfunc_spread", "void", "(i8*, i32, i32)" },
# define VM_DFUNC_SPREAD_INT		78
    { "vm_dfunc_spread_int", Int, "(i8*, i32, i32)" },
# define VM_DFUNC_SPREAD_FLOAT		79
    { "vm_dfunc_spread_float", Double, "(i8*, i32, i32)" },
# define VM_FUNC			80
    { "vm_func", "void", "(i8*, i32, i32)" },
# define VM_FUNC_INT			81
    { "vm_func_int", Int, "(i8*, i32, i32)" },
# define VM_FUNC_FLOAT			82
    { "vm_func_float", Double, "(i8*, i32, i32)" },
# define VM_FUNC_SPREAD			83
    { "vm_func_spread", "void", "(i8*, i32, i32)" },
# define VM_FUNC_SPREAD_INT		84
    { "vm_func_spread_int", Int, "(i8*, i32, i32)" },
# define VM_FUNC_SPREAD_FLOAT		85
    { "vm_func_spread_float", Double, "(i8*, i32, i32)" },
# define VM_POP				86
    { "vm_pop", "void", "(i8*)" },
# define VM_POP_BOOL			87
    { "vm_pop_bool", "i1", "(i8*)" },
# define VM_FUNCTIONS			88
};

class GenContext : public FlowContext {
public:
    GenContext(CodeFunction *func, StackSize size) : FlowContext(func, size) {
	stream = stderr;
	next = 0;
	count = 0;
    }

    virtual ~GenContext() { }

    /*
     * prepare context for block
     */
    void prepareGen(class Block *b) {
	if (b->nTo != 0) {
	    next = b->to[0]->first->addr;
	}
    }

    /*
     * generate reference
     */
    char *genRef() {
	static char buf[10];

	sprintf(buf, "%%c%d", ++count);
	return buf;
    }

    /*
     * copy integer value
     */
    void copyInt(char *to, char *from) {
	fprintf(stream, "\t%s = xor " Int " %s, 0\n", to, from);
    }

    /*
     * copy float value
     */
    void copyFloat(char *to, char *from) {
	fprintf(stream, "\t%s = fadd fast " Double " %s, 0.0\n", to, from);
    }

    /*
     * call VM function without arguments
     */
    void call(int func, char *ref) {
	fprintf(stream, "\t%s = call %s %s(i8* %%f)\n", ref,
		functions[func].ret, load(func));
    }

    /*
     * call VM function with arguments
     */
    void callArgs(int func, char *ref) {
	fprintf(stream, "\t%s = call %s %s(i8* %%f, ", ref,
		functions[func].ret, load(func));
    }

    /*
     * void call VM function without arguments
     */
    void voidCall(int func) {
	fprintf(stream, "\tcall %s %s(i8* %%f)\n", functions[func].ret,
		load(func));
    }

    /*
     * void call VM function with arguments
     */
    void voidCallArgs(int func) {
	fprintf(stream, "\tcall %s %s(i8* %%f, ", functions[func].ret,
		load(func));
    }

    FILE *stream;		/* output file */
    CodeSize next;		/* address of next block */

private:
    /*
     * load a function address
     */
    char *load(int func) {
	char *ref;

	ref = genRef();
	fprintf(stream, "\t%s = load %s %s*, %s %s** @%s, align %d\n",
		ref, functions[func].ret, functions[func].args,
		functions[func].ret, functions[func].args,
		functions[func].name, 8);
	return ref;
    }

    int count;			/* reference counter */
};


ClangCode::ClangCode(CodeFunction *function) :
    FlowCode(function)
{
}

ClangCode::~ClangCode()
{
}

/*
 * return true if the result of the current instruction should be pushed
 * on the stack
 */
bool ClangCode::onStack(GenContext *context, StackSize sp)
{
    Type type;
    Code *code;

    type = context->get(sp).type;
    if (type != LPC_TYPE_INT && type != LPC_TYPE_FLOAT) {
	return true;
    }

    code = context->consumer(sp, type);
    if (code == NULL) {
	return true;
    }

    switch (code->instruction) {
    case STORE_PARAM:
    case STORE_LOCAL:
    case STORE_GLOBAL:
    case RLIMITS:
    case RLIMITS_CHECK:
	return false;

    case JUMP_ZERO:
    case JUMP_NONZERO:
	return (type != LPC_TYPE_INT);

    case KFUNC:
	switch (code->kfun.func) {
	case KF_ADD_INT:
	case KF_ADD1_INT:
	case KF_AND_INT:
	case KF_DIV_INT:
	case KF_EQ_INT:
	case KF_GE_INT:
	case KF_GT_INT:
	case KF_LE_INT:
	case KF_LSHIFT_INT:
	case KF_LT_INT:
	case KF_MOD_INT:
	case KF_MULT_INT:
	case KF_NE_INT:
	case KF_NEG_INT:
	case KF_NOT_INT:
	case KF_OR_INT:
	case KF_RSHIFT_INT:
	case KF_SUB_INT:
	case KF_SUB1_INT:
	case KF_TOFLOAT:
	case KF_TOINT:
	case KF_TST_INT:
	case KF_UMIN_INT:
	case KF_XOR_INT:
	case KF_ADD_FLT:
	case KF_ADD1_FLT:
	case KF_DIV_FLT:
	case KF_EQ_FLT:
	case KF_GE_FLT:
	case KF_GT_FLT:
	case KF_LE_FLT:
	case KF_LT_FLT:
	case KF_MULT_FLT:
	case KF_NE_FLT:
	case KF_NOT_FLT:
	case KF_SUB_FLT:
	case KF_SUB1_FLT:
	case KF_TST_FLT:
	case KF_UMIN_FLT:
	    return false;

	default:
	    return true;
	}

    default:
	return true;
    }
}

/*
 * return a name for a reference to a temporary value
 */
char *ClangCode::tmpRef(StackSize sp)
{
    static char bufs[3][10];
    static int n = 0;

    n = (n + 1) % 3;
    sprintf(bufs[n], "%%t%d", sp);
    return bufs[n];
}

/*
 * return a name for a parameter reference
 */
char *ClangCode::paramRef(LPCParam param, int ref)
{
    static char buf[20];

    if (ref < 0) {
	/* merged */
	sprintf(buf, "%%p%dm%d", param, -ref);
    } else {
	if (ref == GenContext::INITIAL) {
	    ref = 0;
	}
	sprintf(buf, "%%p%dr%d", param, ref);
    }
    return buf;
}

/*
 * return a name for a local var reference
 */
char *ClangCode::localRef(LPCLocal local, int ref)
{
    static char buf[20];

    if (ref < 0) {
	/* merged */
	sprintf(buf, "%%l%dm%d", local, -ref);
    } else {
	if (ref == GenContext::INITIAL) {
	    ref = 0;
	}
	sprintf(buf, "%%l%dr%d", local, ref);
    }
    return buf;
}

/*
 * return a name for the parameter this Code refers to
 */
char *ClangCode::paramRef(GenContext *context, LPCParam param)
{
    int ref;

    ref = reference();
    if (ref == 0) {
	ref = context->inParams[param];
    }
    return paramRef(param, ref);
}

/*
 * return a name for the local var this Code refers to
 */
char *ClangCode::localRef(GenContext *context, LPCLocal local)
{
    int ref;

    ref = reference();
    if (ref == 0) {
	ref = context->inLocals[local];
    }
    return localRef(local, ref);
}

/*
 * push current result on the stack, if needed
 */
void ClangCode::result(GenContext *context)
{
    StackSize sp;

    sp = stackPointer();
    if (!pop && onStack(context, sp)) {
	if (context->get(sp).type == LPC_TYPE_INT) {
	    context->voidCallArgs(VM_INT);
	    fprintf(context->stream, Int " %s)\n", tmpRef(sp));
	} else {
	    context->voidCallArgs(VM_FLOAT);
	    fprintf(context->stream, Double " %s)\n", tmpRef(sp));
	}
    }
    context->sp = sp;
}

/*
 * emit pseudo instructions for this Code
 */
void ClangCode::emit(GenContext *context)
{
    StackSize sp;
    long double d;
    char *ref;

    if (line != context->line) {
	fprintf(context->stream, "; line %d\n", line);
	context->line = line;
    }

    sp = stackPointer();
    switch (instruction) {
    case INT:
	fprintf(context->stream, "\t%s = xor " Int " %lld, 0\n", tmpRef(sp),
		(long long) num);
	result(context);
	return;

    case FLOAT:
	if ((flt.high | flt.low) == 0) {
	    d = 0.0;
	} else {
	    d = ldexpl((long double) (0x10000 | (flt.high & 0xffff)), 64);
	    d = ldexpl(d + flt.low, ((flt.high >> 16) & 0x7fff) - 0x404f);
	    if (flt.high & 0x80000000) {
		d = -d;
	    }
	}
	fprintf(context->stream, "\t%s = fadd fast " Double " %.26LgL, 0.0\n",
		tmpRef(sp), d);
	result(context);
	return;

    case STRING:
	context->voidCallArgs(VM_STRING);
	fprintf(context->stream, "i32 %u, i32 %u)\n", str.inherit, str.index);
	break;

    case PARAM:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->copyInt(tmpRef(sp), paramRef(context, param));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->copyFloat(tmpRef(sp), paramRef(context, param));
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_PARAM);
	    fprintf(context->stream, "i32 %d)\n", param);
	    break;
	}
	break;

    case LOCAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->copyInt(tmpRef(sp), localRef(context, local));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->copyFloat(tmpRef(sp), localRef(context, local));
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_LOCAL);
	    fprintf(context->stream, "i32 %d)\n", local);
	    break;
	}
	break;

    case GLOBAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_GLOBAL_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit,
		    var.index);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_GLOBAL_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit,
		    var.index);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_GLOBAL);
	    fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit,
		    var.index);
	    break;
	}
	break;

    case INDEX:
	if (context->get(sp).type == LPC_TYPE_INT) {
	    context->call(VM_INDEX_INT, tmpRef(sp));
	    result(context);
	    return;
	}
	context->voidCall(VM_INDEX);
	break;

    case INDEX2:
	if (context->get(sp).type == LPC_TYPE_INT) {
	    context->call(VM_INDEX2_INT, tmpRef(sp));
	    result(context);
	    return;
	}
	context->voidCall(VM_INDEX2);
	break;

    case AGGREGATE:
	context->voidCallArgs(VM_AGGREGATE);
	fprintf(context->stream, "i32 %d)\n", size);
	break;

    case MAP_AGGREGATE:
	context->voidCallArgs(VM_MAP_AGGREGATE);
	fprintf(context->stream, "i32 %d)\n", size);
	break;

    case CAST:
	switch (type.type) {
	case LPC_TYPE_INT:
	    context->call(VM_CAST_INT, tmpRef(sp));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->call(VM_CAST_FLOAT, tmpRef(sp));
	    result(context);
	    return;

	case LPC_TYPE_CLASS:
	    context->voidCallArgs(VM_CAST);
	    fprintf(context->stream, "i32 %d, i32 %u, i32 %u)\n", type.type,
		    type.inherit, type.index);
	    break;

	default:
	    context->voidCallArgs(VM_CAST);
	    fprintf(context->stream, "i32 %d, i32 0, i32 0)\n", type.type);
	    break;
	}
	break;

    case INSTANCEOF:
	context->callArgs(VM_INSTANCEOF, tmpRef(sp));
	fprintf(context->stream, "i32 %d, i32 %d)\n", str.inherit, str.index);
	result(context);
	return;

    case CHECK_RANGE:
	context->voidCall(VM_RANGE);
	break;

    case CHECK_RANGE_FROM:
	context->voidCall(VM_RANGE_FROM);
	break;

    case CHECK_RANGE_TO:
	context->voidCall(VM_RANGE_TO);
	break;

    case STORE_PARAM:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    context->copyInt(paramRef(context, param), tmpRef(context->sp));
	    context->voidCallArgs(VM_STORE_PARAM_INT);
	    fprintf(context->stream, "i32 %d, " Int " %s)\n", param,
		    tmpRef(context->sp));
	    if (!pop) {
		context->copyInt(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->copyFloat(paramRef(context, param), tmpRef(context->sp));
	    context->voidCallArgs(VM_STORE_PARAM_FLOAT);
	    fprintf(context->stream, "i32 %d, " Double " %s)\n", param,
		    tmpRef(context->sp));
	    if (!pop) {
		context->copyFloat(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_STORE_PARAM);
	    fprintf(context->stream, "i32 %d)\n", param);
	    break;
	}
	break;

    case STORE_LOCAL:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    context->copyInt(localRef(context, local), tmpRef(context->sp));
	    if (context->level > 0) {
		context->voidCallArgs(VM_STORE_LOCAL_INT);
		fprintf(context->stream, "i32 %d, " Int " %s)\n", local,
			tmpRef(context->sp));
	    }
	    if (!pop) {
		context->copyInt(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->copyFloat(localRef(context, local), tmpRef(context->sp));
	    if (context->level > 0) {
		context->voidCallArgs(VM_STORE_LOCAL_FLOAT);
		fprintf(context->stream, "i32 %d, " Double " %s)\n", local,
			tmpRef(context->sp));
	    }
	    if (!pop) {
		context->copyFloat(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_STORE_LOCAL);
	    fprintf(context->stream, "i32 %d)\n", local);
	    break;
	}
	break;

    case STORE_GLOBAL:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    context->voidCallArgs(VM_STORE_GLOBAL_INT);
	    fprintf(context->stream, "i32 %u, i32 %u, " Int " %s)\n",
		    var.inherit, var.index, tmpRef(context->sp));
	    if (!pop) {
		context->copyInt(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->voidCallArgs(VM_STORE_GLOBAL_FLOAT);
	    fprintf(context->stream, "i32 %u, i32 %u, " Double " %s)\n",
		    var.inherit, var.index, tmpRef(context->sp));
	    if (!pop) {
		context->copyFloat(tmpRef(sp), tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_STORE_GLOBAL);
	    fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit,
		    var.index);
	    break;
	}
	break;

    case STORE_INDEX:
	context->voidCall(VM_STORE_INDEX);
	break;

    case STORE_PARAM_INDEX:
	context->voidCallArgs(VM_STORE_PARAM_INDEX);
	fprintf(context->stream, "i32 %d)\n", param);
	break;

    case STORE_LOCAL_INDEX:
	context->voidCallArgs(VM_STORE_LOCAL_INDEX);
	fprintf(context->stream, "i32 %d)\n", local);
	break;

    case STORE_GLOBAL_INDEX:
	context->voidCallArgs(VM_STORE_GLOBAL_INDEX);
	fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit, var.index);
	break;

    case STORE_INDEX_INDEX:
	context->voidCall(VM_STORE_INDEX_INDEX);
	break;

    case STORES:
	context->voidCallArgs(VM_STORES);
	fprintf(context->stream, "i32 %d)\n", size);
	if (!context->stores(size, NULL)) {
	    context->voidCall(VM_POP);
	}
	break;

    case STORES_LVAL:
	context->voidCallArgs(VM_STORES_LVAL);
	fprintf(context->stream, "i32 %d)\n", size);
	if (!context->stores(size, (pop) ? this : NULL)) {
	    context->voidCall(VM_POP);
	    if (pop) {
		context->voidCall(VM_POP);
	    }
	}
	context->sp = sp;
	return;

    case STORES_SPREAD:
	context->voidCallArgs(VM_STORES_SPREAD);
	if (type.type == LPC_TYPE_CLASS) {
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %u, i32 %u)\n",
		    spread, type.type, type.inherit, type.index);
	} else {
	    fprintf(context->stream, "i32 %d, i32 %d, i32 0, i32 0)\n", spread,
		    type.type);
	}
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_CAST:
	context->voidCallArgs(VM_STORES_CAST);
	if (type.type == LPC_TYPE_CLASS) {
	    fprintf(context->stream, "i32 %d, i32 %u, i32 %u)\n", type.type,
		    type.inherit, type.index);
	} else {
	    fprintf(context->stream, "i32 %d, i32 0, i32 0)\n", type.type);
	}
	context->castType = simplifiedType(type.type);
	break;

    case STORES_PARAM:
	switch (context->castType) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_STORES_PARAM_INT, paramRef(context, param));
	    fprintf(context->stream, "i32 %d)\n", param);
	    break;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_STORES_PARAM_FLOAT, paramRef(context, param));
	    fprintf(context->stream, "i32 %d)\n", param);
	    break;

	default:
	    context->voidCallArgs(VM_STORES_PARAM);
	    fprintf(context->stream, "i32 %d)\n", param);
	    break;
	}
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_LOCAL:
	switch (context->castType) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_STORES_LOCAL_INT, localRef(context, local));
	    fprintf(context->stream, "i32 %d)\n", local);
	    break;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_STORES_LOCAL_FLOAT, localRef(context, local));
	    fprintf(context->stream, "i32 %d)\n", local);
	    break;

	default:
	    context->voidCallArgs(VM_STORES_LOCAL);
	    fprintf(context->stream, "i32 %d)\n", local);
	    break;
	}
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_GLOBAL:
	context->voidCallArgs(VM_STORES_GLOBAL);
	fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit, var.index);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_INDEX:
	context->voidCall(VM_STORES_INDEX);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_PARAM_INDEX:
	context->voidCallArgs(VM_STORES_PARAM_INDEX);
	fprintf(context->stream, "i32 %d)\n", param);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_LOCAL_INDEX:
	context->voidCallArgs(VM_STORES_LOCAL_INDEX);
	fprintf(context->stream, "i32 %d)\n", local);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_GLOBAL_INDEX:
	context->voidCallArgs(VM_STORES_GLOBAL_INDEX);
	fprintf(context->stream, "i32 %u, i32 %u)\n", var.inherit, var.index);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case STORES_INDEX_INDEX:
	context->voidCall(VM_STORES_INDEX_INDEX);
	if (!context->storeN()) {
	    context->voidCall(VM_POP);
	    if (context->storePop() != NULL) {
		context->voidCall(VM_POP);
	    }
	}
	break;

    case JUMP:
	// XXX account ticks for backward jump
	break;

    case JUMP_ZERO:
	// XXX account ticks for backward jump
	ref = context->genRef();
	if (context->get(context->sp).type == LPC_TYPE_INT) {
	    fprintf(context->stream, "\t%s = icmp ne " Int " %s, 0\n", ref,
		    tmpRef(context->sp));
	} else {
	    context->call(VM_POP_BOOL, ref);
	}
	fprintf(context->stream, "\tbr i1 %s, %%L%04x, %%L%04x\n", ref,
		context->next, target);
	context->sp = sp;
	return;

    case JUMP_NONZERO:
	// XXX account ticks for backward jump
	ref = context->genRef();
	if (context->get(context->sp).type == LPC_TYPE_INT) {
	    fprintf(context->stream, "\t%s = icmp ne " Int " %s, 0\n", ref,
		    tmpRef(context->sp));
	} else {
	    context->call(VM_POP_BOOL, ref);
	}
	fprintf(context->stream, "\tbr i1 %s, %%L%04x, %%L%04x\n", ref, target,
		context->next);
	context->sp = sp;
	return;

    case SWITCH_INT:
	// XXX account ticks for backward jump
	fprintf(context->stream, "\tcall @vm_switch_int()\n");
	break;

    case SWITCH_RANGE:
	// XXX account ticks for backward jump
	fprintf(context->stream, "\tcall @vm_switch_range()\n");
	break;

    case SWITCH_STRING:
	// XXX account ticks for backward jump
	fprintf(context->stream, "\tcall @vm_switch_string()\n");
	break;

    case KFUNC:
    case KFUNC_LVAL:
	switch (kfun.func) {
	case KF_ADD_INT:
	    fprintf(context->stream, "\t%s = add nsw " Int " %s, %s\n",
		    tmpRef(sp), tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_ADD1_INT:
	    fprintf(context->stream, "\t%s = add nsw " Int " %s, 1\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_AND_INT:
	    fprintf(context->stream, "\t%s = and " Int " %s, %s\n",
		    tmpRef(sp), tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_DIV_INT:
	    context->callArgs(VM_DIV_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %s, i32 %s)\n",
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_EQ_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp eq " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_GE_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp sge " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_GT_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp sgt " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_LE_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp sle " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_LSHIFT_INT:
	    context->callArgs(VM_LSHIFT_INT, tmpRef(sp));
	    fprintf(context->stream, Int " %s, " Int " %s)\n",
		    tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LT_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp slt " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_MOD_INT:
	    context->callArgs(VM_MOD_INT, tmpRef(sp));
	    fprintf(context->stream, Int " %s, " Int " %s)\n",
		    tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_MULT_INT:
	    fprintf(context->stream, "\t%s = mul nsw " Int " %s, %s\n",
		    tmpRef(sp), tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NE_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp ne " Int " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_NEG_INT:
	    fprintf(context->stream, "\t%s = xor " Int " %s, -1\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NOT_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp eq " Int " %s, 0\n", ref,
		    tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_OR_INT:
	    fprintf(context->stream, "\t%s = or " Int " %s, %s\n",
		    tmpRef(sp), tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_RSHIFT_INT:
	    context->callArgs(VM_RSHIFT_INT, tmpRef(sp));
	    fprintf(context->stream, Int " %s, " Int " %s)\n",
		    tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB_INT:
	    fprintf(context->stream, "\t%s = sub nsw " Int "%s, %s\n",
		    tmpRef(sp), tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB1_INT:
	    fprintf(context->stream, "\t%s = sub nsw " Int " %s, 1\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_TOFLOAT:
	    switch (context->get(context->sp).type) {
	    case LPC_TYPE_INT:
		fprintf(context->stream,
			"\t%s = sitofp " Int "%s to " Double "\n", tmpRef(sp),
			tmpRef(context->sp));
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		context->copyFloat(tmpRef(sp), tmpRef(context->sp));
		result(context);
		return;

	    default:
		context->call(VM_TOFLOAT, tmpRef(sp));
		result(context);
		return;
	    }
	    break;

	case KF_TOINT:
	    switch (context->get(context->sp).type) {
	    case LPC_TYPE_INT:
		context->copyInt(tmpRef(sp), tmpRef(context->sp));
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		context->callArgs(VM_TOINT_FLOAT, tmpRef(sp));
		fprintf(context->stream, Double " %s)\n", tmpRef(context->sp));
		result(context);
		return;

	    default:
		context->call(VM_TOINT, tmpRef(sp));
		result(context);
		return;
	    }
	    break;

	case KF_TST_INT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = icmp ne " Int " %s, 0\n", ref,
		    tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_UMIN_INT:
	    fprintf(context->stream, "\t%s = sub nsw " Int " 0, %s\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_XOR_INT:
	    fprintf(context->stream, "\t%s = xor " Int " %s, %s\n", tmpRef(sp),
		    tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NIL:
	    context->voidCall(VM_NIL);
	    break;

	case KF_ADD_FLT:
	    context->callArgs(VM_ADD_FLOAT, tmpRef(sp));
	    fprintf(context->stream, Double " %s, " Double " %s)\n",
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_ADD1_FLT:
	    fprintf(context->stream, "\t%s = fadd " Double " %s, 1.0\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_DIV_FLT:
	    context->callArgs(VM_DIV_FLOAT, tmpRef(sp));
	    fprintf(context->stream, Double " %s, " Double " %s)\n",
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_EQ_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp oeq " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_GE_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp oge " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_GT_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp ogt " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_LE_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp ole " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_LT_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp olt " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_MULT_FLT:
	    context->callArgs(VM_MULT_FLOAT, tmpRef(sp));
	    fprintf(context->stream, Double " %s, " Double " %s)\n",
		    tmpRef(context->nextSp(context->sp)),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NE_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp one " Double " %s, %s\n", ref,
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_NOT_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp oeq " Double " %s, 0.0\n",
		    ref, tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_SUB_FLT:
	    context->callArgs(VM_SUB_FLOAT, tmpRef(sp));
	    fprintf(context->stream, Double " %s, " Double " %s)\n",
		    tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB1_FLT:
	    fprintf(context->stream, "\t%s = fsub " Double " %s, 1.0\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_TST_FLT:
	    ref = context->genRef();
	    fprintf(context->stream, "\t%s = fcmp one " Double " %s, 0.0\n",
		    ref, tmpRef(context->sp));
	    fprintf(context->stream, "\t%s = zext i1 %s to " Int "\n",
		    tmpRef(sp), ref);
	    result(context);
	    return;

	case KF_UMIN_FLT:
	    fprintf(context->stream, "\t%s = fsub " Double " 0.0, %s\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	default:
	    switch (context->get(sp).type) {
	    case LPC_TYPE_INT:
		context->callArgs(VM_KFUNC_INT, tmpRef(sp));
		fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
			kfun.nargs);
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		context->callArgs(VM_KFUNC_FLOAT, tmpRef(sp));
		fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
			kfun.nargs);
		result(context);
		return;

	    default:
		context->voidCallArgs(VM_KFUNC);
		fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
			kfun.nargs);
		break;
	    }
	}
	break;

    case KFUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_KFUNC_SPREAD_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_KFUNC_SPREAD_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_KFUNC_SPREAD);
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    break;
	}
	break;

    case KFUNC_SPREAD_LVAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_KFUNC_SPREAD_LVAL_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_KFUNC_SPREAD_LVAL_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_KFUNC_SPREAD_LVAL);
	    fprintf(context->stream, "i32 %d, i32 %d)\n", kfun.func,
		    kfun.nargs);
	    break;
	}
	break;

    case DFUNC:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_DFUNC_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_DFUNC_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_DFUNC);
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    break;
	}
	break;

    case DFUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_DFUNC_SPREAD_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_DFUNC_SPREAD_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_DFUNC_SPREAD);
	    fprintf(context->stream, "i32 %d, i32 %d, i32 %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    break;
	}
	break;

    case FUNC:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_FUNC_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_FUNC_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_FUNC);
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    break;
	}
	break;

    case FUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    context->callArgs(VM_FUNC_SPREAD_INT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    context->callArgs(VM_FUNC_SPREAD_FLOAT, tmpRef(sp));
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    result(context);
	    return;

	default:
	    context->voidCallArgs(VM_FUNC_SPREAD);
	    fprintf(context->stream, "i32 %d, i32 %d)\n", fun.call, fun.nargs);
	    break;
	}
	break;

    case CATCH:
	if (pop) {
	    fprintf(context->stream, "\tcatch_pop(f) %%L%04x, %%L%04x\n", context->next,
		    target);
	} else {
	    fprintf(context->stream, "\tcatch(f) %%L%04x, %%L%04x\n", context->next,
		    target);
	}
	context->sp = sp;
	return;

    case END_CATCH:
	fprintf(context->stream, "\tcatch_end\n");
	break;

    case RLIMITS:
	fprintf(context->stream, "\trlimits(f, %s, %s)\n",
		tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	break;

    case RLIMITS_CHECK:
	fprintf(context->stream, "\trlimits_check(f, %s, %s)\n",
		tmpRef(context->nextSp(context->sp)), tmpRef(context->sp));
	break;

    case END_RLIMITS:
	fprintf(context->stream, "\trlimits_end\n");
	break;

    case RETURN:
	fprintf(context->stream, "\tret void\n");
	break;

    default:
	break;
    }

    if (pop) {
	context->voidCall(VM_POP);
    }
    context->sp = sp;
}

/*
 * create a Clang code
 */
Code *ClangCode::create(CodeFunction *function)
{
    return new ClangCode(function);
}


ClangBlock::ClangBlock(Code *first, Code *last, CodeSize size) :
    FlowBlock(first, last, size)
{
}

ClangBlock::~ClangBlock()
{
}

/*
 * emit pseudo instructions for all blocks
 */
void ClangBlock::emit(CodeFunction *function, CodeSize size)
{
    GenContext context(function, size);
    Block *b;
    LPCParam nParams, n;
    bool initParam;
    CodeSize i;
    Type type;
    int ref;
    bool phi;
    StackSize sp;
    Code *code;

    FlowBlock::evaluate(&context);

    initParam = false;
    nParams = function->nargs + function->vargs;
    for (n = 1; n <= nParams; n++) {
	switch (function->proto[n].type) {
	case LPC_TYPE_INT:
	    if (!initParam) {
		fprintf(context.stream, "%%Lparam:\n");
	    }
	    context.callArgs(VM_PARAM_INT, ClangCode::paramRef(nParams - n, 0));
	    fprintf(context.stream, "int32 %d)\n", nParams - n);
	    break;

	case LPC_TYPE_FLOAT:
	    if (!initParam) {
		fprintf(context.stream, "%%Lparam:\n");
	    }
	    context.callArgs(VM_PARAM_FLOAT,
			     ClangCode::paramRef(nParams - n, 0));
	    fprintf(context.stream, "int32 %d)\n", nParams - n);
	    break;

	default:
	    continue;
	}
	initParam = true;
    }
    if (initParam) {
	fprintf(context.stream, "\tbr %%L0000\n");
    }

    for (b = this; b != NULL; b = b->next) {
	if (b->nFrom == 0 && b != this) {
	    continue;
	}
	fprintf(context.stream, "%%L%04x:\n", b->first->addr);
	b->prepareFlow(&context);
	context.prepareGen(b);

	if (b->nFrom > (b != this)) {
	    /*
	     * parameters
	     */
	    for (n = 0; n < context.nParams; n++) {
		if (context.inParams[n] == -(b->first->addr + 1)) {
		    i = 0;
		    if (b == this) {
			type = function->proto[nParams - n].type;
		    } else {
			type = LPC_TYPE_VOID;
			for (; i < b->nFrom; i++) {
			    if (b->from[i]->paramRef(n) != 0) {
				type = b->from[i]->paramType(n);
				break;
			    }
			}
		    }
		    for (; i < b->nFrom; i++) {
			if (b->from[i]->paramRef(n) != 0 &&
			    b->from[i]->paramType(n) != type) {
			    type = LPC_TYPE_VOID;
			    break;
			}
		    }

		    if (type == LPC_TYPE_INT) {
			fprintf(context.stream, "\t%s = phi " Int,
				ClangCode::paramRef(n, context.inParams[n]));
		    } else if (type == LPC_TYPE_FLOAT) {
			fprintf(context.stream, "\t%s = phi " Double,
				ClangCode::paramRef(n, context.inParams[n]));
		    } else {
			continue;
		    }
		    phi = true;
		    if (b == this) {
			fprintf(context.stream, " [ %s, %%Lparam ]",
				ClangCode::paramRef(n, 0));
			phi = false;
		    }
		    for (i = 0; i < b->nFrom; i++) {
			ref = b->from[i]->paramRef(n);
			if (ref != context.inParams[n]) {
			    if (!phi) {
				fprintf(context.stream, ",");
			    }
			    fprintf(context.stream, " [ %s, %%L%04x ]",
				    ClangCode::paramRef(n, ref),
				    b->from[i]->first->addr);
			    phi = false;
			}
		    }
		    fprintf(context.stream, "\n");
		}
	    }
	}

	if (b->nFrom > 1) {
	    /*
	     * locals
	     */
	    for (n = 0; n < context.nLocals; n++) {
		if (context.inLocals[n] == -(b->first->addr + 1)) {
		    type = LPC_TYPE_VOID;
		    for (i = 0; i < b->nFrom; i++) {
			if (b->from[i]->localRef(n) != 0) {
			    type = b->from[i]->localType(n);
			    break;
			}
		    }
		    for (; i < b->nFrom; i++) {
			if (b->from[i]->localRef(n) != 0 &&
			    b->from[i]->localType(n) != type) {
			    type = LPC_TYPE_VOID;
			    break;
			}
		    }

		    if (type == LPC_TYPE_INT) {
			fprintf(context.stream, "\t%s = phi " Int,
				ClangCode::localRef(n, context.inLocals[n]));
		    } else if (type == LPC_TYPE_FLOAT) {
			fprintf(context.stream, "\t%s = phi " Double,
				ClangCode::localRef(n, context.inLocals[n]));
		    } else {
			continue;
		    }
		    phi = true;
		    for (i = 0; i < b->nFrom; i++) {
			ref = b->from[i]->localRef(n);
			if (ref != context.inLocals[n]) {
			    if (!phi) {
				fprintf(context.stream, ",");
			    }
			    fprintf(context.stream, " [ %s, %%L%04x ]",
				    ClangCode::localRef(n, ref),
				    b->from[i]->first->addr);
			    phi = false;
			}
		    }
		    fprintf(context.stream, "\n");
		}
	    }

	    /*
	     * stack
	     */
	    for (n = 0, sp = b->sp; sp != STACK_EMPTY;
		 n++, sp = context.nextSp(sp)) {
		if (ClangCode::onStack(&context, sp)) {
		    continue;
		}
		switch (context.get(sp).type) {
		case LPC_TYPE_INT:
		    fprintf(context.stream, "\t%s = phi " Int,
			    ClangCode::tmpRef(sp));
		    break;

		case LPC_TYPE_FLOAT:
		    fprintf(context.stream, "\t%s = phi " Double,
			    ClangCode::tmpRef(sp));
		    break;

		default:
		    continue;
		}
		phi = true;

		for (i = 0; i < b->nFrom; i++) {
		    if (!phi) {
			fprintf(context.stream, ",");
		    }
		    fprintf(context.stream, " [ %s, %%L%04x ]",
			    ClangCode::tmpRef(context.nextSp(b->from[i]->endSp,
							     n)),
			    b->from[i]->first->addr);
		    phi = false;
		}
		fprintf(context.stream, "\n");
	    }
	}

	context.sp = b->sp;
	context.level = level;
	for (code = b->first; ; code = code->next) {
	    code->emit(&context);
	    if (code == b->last) {
		switch (code->instruction) {
		case Code::JUMP:
		    fprintf(context.stream, "\tbr %%L%04x\n", code->target);
		    break;

		case Code::JUMP_ZERO:
		case Code::JUMP_NONZERO:
		case Code::SWITCH_INT:
		case Code::SWITCH_RANGE:
		case Code::SWITCH_STRING:
		case Code::CATCH:
		case Code::RETURN:
		    break;

		case Code::END_CATCH:
		    context.level--;
		    break;

		default:
		    fprintf(context.stream, "\tbr %%L%04x\n", context.next);
		    break;
		}
		break;
	    }
	}
    }
}

/*
 * create a Clang block
 */
Block *ClangBlock::create(Code *first, Code *last, CodeSize size)
{
    return new ClangBlock(first, last, size);
}


ClangObject::ClangObject(CodeObject *object, uint8_t *prog, int nFunctions)
{
    this->object = object;
    this->prog = prog;
    this->nFunctions = nFunctions;
}

ClangObject::~ClangObject()
{
}

void ClangObject::header(FILE *stream)
{
    int i;

    fprintf(stream, "target triple = \"%s\"\n\n", TARGET_TRIPLE);
    for (i = 0; i < VM_FUNCTIONS; i++) {
	fprintf(stream, "@%s = external global %s %s*, align %d\n", 
		functions[i].name, functions[i].ret, functions[i].args, 8);
    }
}

void ClangObject::table(FILE *stream, int nFunctions)
{
    int i;

    fprintf(stream, "@functions = [%d x void (i8*, i32, i8*)*] [",
	    nFunctions + 1);
    for (i = 1; i <= nFunctions; i++) {
	fprintf(stream, "void (i8*, i32, i8*)* @func%d, ", i);
    }
    fprintf(stream, "null], align %d\n", 8);
}

/*
 * create a dynamically loadable object
 */
void ClangObject::emit(char *base)
{
    int i;

    /*
     * generate .ll file
     */
    header(stderr);

    table(stderr, nFunctions);

    for (i = 0; i < nFunctions; i++) {
	CodeFunction func(object, prog);
	Block *b = Block::function(&func);

	fprintf(stderr, "\ndefine internal void @func%d"
			"(i8* %%f, i32 %%nargs, i8* %%value) #0 {\n",
		i + 1);
	if (b != NULL) {
	    b->emit(&func, b->fragment());
	    b->clear();
	} else {
	    fprintf(stderr, "\tret void\n");
	}
	prog = func.endProg();
	fprintf(stderr, "}\n");
    }

    /*
     * compile .ll file to object
    system("clang -Os -fPIC -shared fname.ll -o fname.so");
     */
}
