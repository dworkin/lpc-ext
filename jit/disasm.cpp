# include <stdlib.h>
# include <stdint.h>
# include <new>
# include <stdio.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "flow.h"
# include "disasm.h"
# include "jitcomp.h"


class GenContext : public FlowContext {
public:
    GenContext(CodeFunction *func, StackSize size) : FlowContext(func, size) { }
    virtual ~GenContext() { }
};


DisCode::DisCode(CodeFunction *function) : FlowCode(function) { }

DisCode::~DisCode() { }

Code *DisCode::create(CodeFunction *function)
{
    return new DisCode(function);
}

/*
 * disassemble LPC type
 */
static void dis_type(Type type)
{
    int i;

    switch (type & LPC_TYPE_MASK) {
    case LPC_TYPE_NIL:
	fprintf(stderr, "nil");
	break;

    case LPC_TYPE_INT:
	fprintf(stderr, "int");
	break;

    case LPC_TYPE_FLOAT:
	fprintf(stderr, "float");
	break;

    case LPC_TYPE_STRING:
	fprintf(stderr, "string");
	break;

    case LPC_TYPE_OBJECT:
    case LPC_TYPE_CLASS:
	fprintf(stderr, "object");
	break;

    case LPC_TYPE_ARRAY:
	fprintf(stderr, "mixed *");
	break;

    case LPC_TYPE_MAPPING:
	fprintf(stderr, "mapping");
	break;

    case LPC_TYPE_MIXED:
	fprintf(stderr, "mixed");
	break;

    case LPC_TYPE_VOID:
	fprintf(stderr, "void");
	break;

    default:
	fprintf(stderr, "unknown");
	break;
    }

    for (i = LPC_TYPE_REF(type); i != 0; --i) {
	fprintf(stderr, "*");
    }
}

/*
 * disassemble cast type
 */
static void dis_casttype(LPCType *type)
{
    int i;

    switch (type->type & LPC_TYPE_MASK) {
    case LPC_TYPE_NIL:
	fprintf(stderr, "nil\t");
	break;

    case LPC_TYPE_INT:
	fprintf(stderr, "int\t");
	break;

    case LPC_TYPE_FLOAT:
	fprintf(stderr, "float\t");
	break;

    case LPC_TYPE_STRING:
	fprintf(stderr, "string");
	break;

    case LPC_TYPE_OBJECT:
	fprintf(stderr, "object");
	break;

    case LPC_TYPE_ARRAY:
	fprintf(stderr, "mixed *");
	break;

    case LPC_TYPE_MAPPING:
	fprintf(stderr, "mapping");
	break;

    case LPC_TYPE_CLASS:
	fprintf(stderr, "class <%d, %d>", type->inherit, type->index);
	break;

    case LPC_TYPE_MIXED:
	fprintf(stderr, "mixed\t");
	break;

    case LPC_TYPE_VOID:
	fprintf(stderr, "void\t");
	break;

    default:
	fprintf(stderr, "unknown");
	break;
    }

    for (i = LPC_TYPE_REF(type->type); i != 0; --i) {
	fprintf(stderr, "*");
    }
}

/*
 * emit instruction disassembly
 */
void DisCode::emit(GenContext *context)
{
    int i;

    fprintf(stderr, " %04x", addr);
    if (line != context->line) {
	fprintf(stderr, "%5d", line);
	context->line = line;
    } else {
	fprintf(stderr, "     ");
    }
    if (pop) {
	fprintf(stderr, " P ");
    } else {
	fprintf(stderr, "   ");
    }

    switch (instruction) {
    case INT:
	fprintf(stderr, "INT %lld\n", (long long) num);
	break;

    case FLOAT:
	fprintf(stderr, "FLOAT <%08X, %016llX>\n", flt.high,
		(unsigned long long) flt.low);
	break;

    case STRING:
	fprintf(stderr, "STRING <%d, %d>\n", str.inherit, str.index);
	break;

    case PARAM:
	fprintf(stderr, "PARAM %d\n", param);
	break;

    case LOCAL:
	fprintf(stderr, "LOCAL %d\n", local);
	break;

    case GLOBAL:
	fprintf(stderr, "GLOBAL <%d, %d>\n", var.inherit, var.index);
	break;

    case INDEX:
	fprintf(stderr, "INDEX\n");
	break;

    case INDEX2:
	fprintf(stderr, "INDEX2\n");
	break;

    case SPREAD:
	fprintf(stderr, "SPREAD\n");
	break;

    case SPREAD_LVAL:
	fprintf(stderr, "SPREAD_LVAL %d\n", spread);
	break;

    case STORES_SPREAD:
	fprintf(stderr, "STORES_SPREAD %d ", spread);
	dis_casttype(&type);
	fprintf(stderr, "\n");
	break;

    case AGGREGATE:
	fprintf(stderr, "AGGREGATE %d\n", size);
	break;

    case MAP_AGGREGATE:
	fprintf(stderr, "MAP_AGGREGATE %d\n", size);
	break;

    case CAST:
	fprintf(stderr, "CAST ");
	dis_casttype(&type);
	fprintf(stderr, "\n");
	break;

    case STORES_CAST:
	fprintf(stderr, "STORES_CAST ");
	dis_casttype(&type);
	fprintf(stderr, "\n");
	break;

    case INSTANCEOF:
	fprintf(stderr, "INSTANCEOF <%d, %d>\n", str.inherit, str.index);
	break;

    case CHECK_RANGE:
	fprintf(stderr, "CHECK_RANGE\n");
	break;

    case CHECK_RANGE_FROM:
	fprintf(stderr, "CHECK_RANGE_FROM\n");
	break;

    case CHECK_RANGE_TO:
	fprintf(stderr, "CHECK_RANGE_TO\n");
	break;

    case STORES:
	fprintf(stderr, "STORES %d\n", size);
	break;

    case STORES_LVAL:
	fprintf(stderr, "STORES_LVAL %d\n", size);
	break;

    case STORE_PARAM:
	fprintf(stderr, "STORE_PARAM %d\n", param);
	break;

    case STORE_LOCAL:
	fprintf(stderr, "STORE_LOCAL %d\n", local);
	break;

    case STORE_GLOBAL:
	fprintf(stderr, "STORE_GLOBAL <%d, %d>\n", var.inherit, var.index);
	break;

    case STORE_INDEX:
	fprintf(stderr, "STORE_INDEX\n");
	break;

    case STORE_PARAM_INDEX:
	fprintf(stderr, "STORE_PARAM_INDEX %d\n", param);
	break;

    case STORE_LOCAL_INDEX:
	fprintf(stderr, "STORE_LOCAL_INDEX %d\n", local);
	break;

    case STORE_GLOBAL_INDEX:
	fprintf(stderr, "STORE_GLOBAL_INDEX <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORE_INDEX_INDEX:
	fprintf(stderr, "STORE_INDEX_INDEX\n");
	break;

    case STORES_PARAM:
	fprintf(stderr, "STORES_PARAM %d\n", param);
	break;

    case STORES_LOCAL:
	fprintf(stderr, "STORES_LOCAL %d\n", local);
	break;

    case STORES_GLOBAL:
	fprintf(stderr, "STORES_GLOBAL <%d, %d>\n", var.inherit, var.index);
	break;

    case STORES_INDEX:
	fprintf(stderr, "STORES_INDEX\n");
	break;

    case STORES_PARAM_INDEX:
	fprintf(stderr, "STORES_PARAM_INDEX %d\n", param);
	break;

    case STORES_LOCAL_INDEX:
	fprintf(stderr, "STORES_LOCAL_INDEX %d\n", local);
	break;

    case STORES_GLOBAL_INDEX:
	fprintf(stderr, "STORES_GLOBAL_INDEX <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORES_INDEX_INDEX:
	fprintf(stderr, "STORES_INDEX_INDEX\n");
	break;

    case JUMP:
	fprintf(stderr, "JUMP %04x\n", target);
	break;

    case JUMP_ZERO:
	fprintf(stderr, "JUMP_ZERO %04x\n", target);
	break;

    case JUMP_NONZERO:
	fprintf(stderr, "JUMP_NONZERO %04x\n", target);
	break;

    case SWITCH_INT:
	fprintf(stderr, "SWITCH_INT %04x\n", caseInt->addr);
	for (i = 1; i < size; i++) {
	    fprintf(stderr, "              case %lld: %04x\n",
		    (long long) caseInt[i].num, caseInt[i].addr);
	}
	break;

    case SWITCH_RANGE:
	fprintf(stderr, "SWITCH_RANGE %04x\n", caseRange->addr);
	for (i = 1; i < size; i++) {
	    fprintf(stderr, "              case %lld..%lld: %04x\n",
		    (long long) caseRange[i].from, (long long) caseRange[i].to,
		    caseRange[i].addr);
	}
	break;

    case SWITCH_STRING:
	fprintf(stderr, "SWITCH_STRING %04x\n", caseString->addr);
	for (i = 1; i < size; i++) {
	    if (caseString[i].str.inherit == 0 &&
		caseString[i].str.index == 0xffff) {
		fprintf(stderr, "              case nil: %04x\n",
			caseString[i].addr);
	    } else {
		fprintf(stderr, "              case <%d, %d>: %04x\n",
			caseString[i].str.inherit, caseString[i].str.index,
			caseString[i].addr);
	    }
	}
	break;

    case KFUNC:
    case KFUNC_SPREAD:
	fprintf(stderr, "KFUNC %d (%d)\n", kfun.func, kfun.nargs);
	break;

    case KFUNC_LVAL:
    case KFUNC_SPREAD_LVAL:
	fprintf(stderr, "KFUNC_LVAL %d (%d)\n", kfun.func, kfun.nargs);
	break;

    case DFUNC:
    case DFUNC_SPREAD:
	fprintf(stderr, "DFUNC <%d, %d> (%d)\n", dfun.inherit, dfun.func,
		dfun.nargs);
	break;

    case FUNC:
    case FUNC_SPREAD:
	fprintf(stderr, "FUNC %d (%d)\n", fun.call, fun.nargs);
	break;

    case CATCH:
	fprintf(stderr, "CATCH %04x\n", target);
	break;

    case END_CATCH:
	fprintf(stderr, "END_CATCH\n");
	break;

    case RLIMITS:
	fprintf(stderr, "RLIMITS\n");
	break;

    case RLIMITS_CHECK:
	fprintf(stderr, "RLIMITS_CHECK\n");
	break;

    case END_RLIMITS:
	fprintf(stderr, "END_RLIMITS\n");
	break;

    case RETURN:
	fprintf(stderr, "RETURN\n");
	break;

    default:
	fatal("unknown instruction %d", instruction);
    }
}


DisBlock::DisBlock(Code *first, Code *last, CodeSize size) :
    FlowBlock(first, last, size)
{
}

DisBlock::~DisBlock() { }

Block *DisBlock::create(Code *first, Code *last, CodeSize size)
{
    return new DisBlock(first, last, size);
}

/*
 * emit program disassembly
 */
void DisBlock::emit(CodeFunction *function, CodeSize size)
{
    GenContext context(function, size);
    CodeLine line;
    Block *b;
    Code *code;
    CodeSize i, sp;

    FlowBlock::evaluate(&context);

    line = 0;
    for (b = this; b != NULL; b = b->next) {
	if (b->nFrom == 0 && b != this) {
	    continue;
	}
	fprintf(stderr, "{");
	if (b->nFrom != 0) {
	    fprintf(stderr, " [");
	    for (i = 0; i < b->nFrom; i++) {
		fprintf(stderr, "%04x", b->from[i]->first->addr);
		if (i != b->nFrom - 1) {
		    fprintf(stderr, ", ");
		}
	    }
	    fprintf(stderr, "]");
	}
	if (b->level != 0) {
	    fprintf(stderr, " <%d>", b->level);
	}
	fprintf(stderr, "\n");
	for (code = b->first; ; code = code->next) {
	    code->emit(&context);
	    if (code == b->last) {
		break;
	    }
	}

	fprintf(stderr, "}");
	if (b->nTo != 0) {
	    fprintf(stderr, " [");
	    for (i = 0; i < b->nTo; i++) {
		fprintf(stderr, "%04x", b->to[i]->first->addr);
		if (i != b->nTo - 1) {
		    fprintf(stderr, ", ");
		}
	    }
	    fprintf(stderr, "]");
	}
	fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
}
