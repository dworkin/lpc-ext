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
    GenContext(FILE *stream, CodeFunction *func, StackSize size) :
	FlowContext(func, size), stream(stream) {
	line = 0;
    }
    virtual ~GenContext() { }

    FILE *stream;
    CodeLine line;
};


DisCode::DisCode(CodeFunction *function) : FlowCode(function) { }

DisCode::~DisCode() { }

Code *DisCode::create(CodeFunction *function)
{
    return new DisCode(function);
}

/*
 * emit type
 */
void DisCode::emitType(GenContext *context, LPCType *type)
{
    int i;

    switch (type->type & LPC_TYPE_MASK) {
    case LPC_TYPE_NIL:
	fprintf(context->stream, "nil\t");
	break;

    case LPC_TYPE_INT:
	fprintf(context->stream, "int\t");
	break;

    case LPC_TYPE_FLOAT:
	fprintf(context->stream, "float\t");
	break;

    case LPC_TYPE_STRING:
	fprintf(context->stream, "string");
	break;

    case LPC_TYPE_OBJECT:
	fprintf(context->stream, "object");
	break;

    case LPC_TYPE_ARRAY:
	fprintf(context->stream, "mixed *");
	break;

    case LPC_TYPE_MAPPING:
	fprintf(context->stream, "mapping");
	break;

    case LPC_TYPE_CLASS:
	fprintf(context->stream, "class <%d, %d>", type->inherit, type->index);
	break;

    case LPC_TYPE_MIXED:
	fprintf(context->stream, "mixed\t");
	break;

    case LPC_TYPE_VOID:
	fprintf(context->stream, "void\t");
	break;

    default:
	fprintf(context->stream, "unknown");
	break;
    }

    for (i = LPC_TYPE_REF(type->type); i != 0; --i) {
	fprintf(context->stream, "*");
    }
}

/*
 * emit instruction disassembly
 */
void DisCode::emit(GenContext *context)
{
    int i;

    fprintf(context->stream, " %04x", addr);
    if (line != context->line) {
	fprintf(context->stream, "%5d", line);
	context->line = line;
    } else {
	fprintf(context->stream, "     ");
    }
    if (pop) {
	fprintf(context->stream, " P ");
    } else {
	fprintf(context->stream, "   ");
    }

    switch (instruction) {
    case INT:
	fprintf(context->stream, "INT %lld\n", (long long) num);
	break;

    case FLOAT:
	fprintf(context->stream, "FLOAT <%08X, %016llX>\n", flt.high,
		(unsigned long long) flt.low);
	break;

    case STRING:
	fprintf(context->stream, "STRING <%d, %d>\n", str.inherit, str.index);
	break;

    case PARAM:
	fprintf(context->stream, "PARAM %d\n", param);
	break;

    case LOCAL:
	fprintf(context->stream, "LOCAL %d\n", local);
	break;

    case GLOBAL:
	fprintf(context->stream, "GLOBAL <%d, %d>\n", var.inherit, var.index);
	break;

    case INDEX:
	fprintf(context->stream, "INDEX\n");
	break;

    case INDEX2:
	fprintf(context->stream, "INDEX2\n");
	break;

    case SPREAD:
	fprintf(context->stream, "SPREAD\n");
	break;

    case SPREAD_LVAL:
	fprintf(context->stream, "SPREAD_LVAL %d\n", spread);
	break;

    case STORES_SPREAD:
	fprintf(context->stream, "STORES_SPREAD %d ", spread);
	emitType(context, &type);
	fprintf(context->stream, "\n");
	break;

    case AGGREGATE:
	fprintf(context->stream, "AGGREGATE %d\n", size);
	break;

    case MAP_AGGREGATE:
	fprintf(context->stream, "MAP_AGGREGATE %d\n", size);
	break;

    case CAST:
	fprintf(context->stream, "CAST ");
	emitType(context, &type);
	fprintf(context->stream, "\n");
	break;

    case STORES_CAST:
	fprintf(context->stream, "STORES_CAST ");
	emitType(context, &type);
	fprintf(context->stream, "\n");
	break;

    case INSTANCEOF:
	fprintf(context->stream, "INSTANCEOF <%d, %d>\n", str.inherit,
		str.index);
	break;

    case CHECK_RANGE:
	fprintf(context->stream, "CHECK_RANGE\n");
	break;

    case CHECK_RANGE_FROM:
	fprintf(context->stream, "CHECK_RANGE_FROM\n");
	break;

    case CHECK_RANGE_TO:
	fprintf(context->stream, "CHECK_RANGE_TO\n");
	break;

    case STORES:
	fprintf(context->stream, "STORES %d\n", size);
	break;

    case STORES_LVAL:
	fprintf(context->stream, "STORES_LVAL %d\n", size);
	break;

    case STORE_PARAM:
	fprintf(context->stream, "STORE_PARAM %d\n", param);
	break;

    case STORE_LOCAL:
	fprintf(context->stream, "STORE_LOCAL %d\n", local);
	break;

    case STORE_GLOBAL:
	fprintf(context->stream, "STORE_GLOBAL <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORE_INDEX:
	fprintf(context->stream, "STORE_INDEX\n");
	break;

    case STORE_PARAM_INDEX:
	fprintf(context->stream, "STORE_PARAM_INDEX %d\n", param);
	break;

    case STORE_LOCAL_INDEX:
	fprintf(context->stream, "STORE_LOCAL_INDEX %d\n", local);
	break;

    case STORE_GLOBAL_INDEX:
	fprintf(context->stream, "STORE_GLOBAL_INDEX <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORE_INDEX_INDEX:
	fprintf(context->stream, "STORE_INDEX_INDEX\n");
	break;

    case STORES_PARAM:
	fprintf(context->stream, "STORES_PARAM %d\n", param);
	break;

    case STORES_LOCAL:
	fprintf(context->stream, "STORES_LOCAL %d\n", local);
	break;

    case STORES_GLOBAL:
	fprintf(context->stream, "STORES_GLOBAL <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORES_INDEX:
	fprintf(context->stream, "STORES_INDEX\n");
	break;

    case STORES_PARAM_INDEX:
	fprintf(context->stream, "STORES_PARAM_INDEX %d\n", param);
	break;

    case STORES_LOCAL_INDEX:
	fprintf(context->stream, "STORES_LOCAL_INDEX %d\n", local);
	break;

    case STORES_GLOBAL_INDEX:
	fprintf(context->stream, "STORES_GLOBAL_INDEX <%d, %d>\n", var.inherit,
		var.index);
	break;

    case STORES_INDEX_INDEX:
	fprintf(context->stream, "STORES_INDEX_INDEX\n");
	break;

    case JUMP:
	fprintf(context->stream, "JUMP %04x\n", target);
	break;

    case JUMP_ZERO:
	fprintf(context->stream, "JUMP_ZERO %04x\n", target);
	break;

    case JUMP_NONZERO:
	fprintf(context->stream, "JUMP_NONZERO %04x\n", target);
	break;

    case SWITCH_INT:
	fprintf(context->stream, "SWITCH_INT %04x\n", caseInt->addr);
	for (i = 1; i < size; i++) {
	    fprintf(context->stream, "              case %lld: %04x\n",
		    (long long) caseInt[i].num, caseInt[i].addr);
	}
	break;

    case SWITCH_RANGE:
	fprintf(context->stream, "SWITCH_RANGE %04x\n", caseRange->addr);
	for (i = 1; i < size; i++) {
	    fprintf(context->stream, "              case %lld..%lld: %04x\n",
		    (long long) caseRange[i].from, (long long) caseRange[i].to,
		    caseRange[i].addr);
	}
	break;

    case SWITCH_STRING:
	fprintf(context->stream, "SWITCH_STRING %04x\n", caseString->addr);
	for (i = 1; i < size; i++) {
	    if (caseString[i].str.inherit == 0 &&
		caseString[i].str.index == 0xffff) {
		fprintf(context->stream, "              case nil: %04x\n",
			caseString[i].addr);
	    } else {
		fprintf(context->stream, "              case <%d, %d>: %04x\n",
			caseString[i].str.inherit, caseString[i].str.index,
			caseString[i].addr);
	    }
	}
	break;

    case KFUNC:
    case KFUNC_SPREAD:
	fprintf(context->stream, "KFUNC %d (%d)\n", kfun.func, kfun.nargs);
	break;

    case KFUNC_LVAL:
    case KFUNC_SPREAD_LVAL:
	fprintf(context->stream, "KFUNC_LVAL %d (%d)\n", kfun.func, kfun.nargs);
	break;

    case DFUNC:
    case DFUNC_SPREAD:
	fprintf(context->stream, "DFUNC <%d, %d> (%d)\n", dfun.inherit,
		dfun.func, dfun.nargs);
	break;

    case FUNC:
    case FUNC_SPREAD:
	fprintf(context->stream, "FUNC %d (%d)\n", fun.call, fun.nargs);
	break;

    case CATCH:
	fprintf(context->stream, "CATCH %04x\n", target);
	break;

    case CAUGHT:
	fprintf(context->stream, "CAUGHT %04x\n", target);
	break;

    case END_CATCH:
	fprintf(context->stream, "END_CATCH\n");
	break;

    case RLIMITS:
	fprintf(context->stream, "RLIMITS\n");
	break;

    case RLIMITS_CHECK:
	fprintf(context->stream, "RLIMITS_CHECK\n");
	break;

    case END_RLIMITS:
	fprintf(context->stream, "END_RLIMITS\n");
	break;

    case RETURN:
	fprintf(context->stream, "RETURN\n");
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
void DisBlock::emit(GenContext *context, CodeFunction *function)
{
    CodeLine line;
    Block *b;
    Code *code;
    CodeSize i, sp;

    FlowBlock::evaluate(context);

    line = 0;
    for (b = this; b != NULL; b = b->next) {
	if (b->nFrom == 0 && b != this) {
	    continue;
	}
	fprintf(context->stream, "{");
	if (b->nFrom != 0) {
	    fprintf(context->stream, " [");
	    for (i = 0; i < b->nFrom; i++) {
		fprintf(context->stream, "%04x", b->from[i]->first->addr);
		if (i != b->nFrom - 1) {
		    fprintf(context->stream, ", ");
		}
	    }
	    fprintf(context->stream, "]");
	}
	if (b->level != 0) {
	    fprintf(context->stream, " <%d>", b->level);
	}
	fprintf(context->stream, "\n");
	for (code = b->first; ; code = code->next) {
	    code->emit(context);
	    if (code == b->last) {
		break;
	    }
	}

	fprintf(context->stream, "}");
	if (b->nTo != 0) {
	    fprintf(context->stream, " [");
	    for (i = 0; i < b->nTo; i++) {
		fprintf(context->stream, "%04x", b->to[i]->first->addr);
		if (i != b->nTo - 1) {
		    fprintf(context->stream, ", ");
		}
	    }
	    fprintf(context->stream, "]");
	}
	fprintf(context->stream, "\n");
    }
    fprintf(context->stream, "\n");
}

/*
 * emit function disassembly
 */
void DisFunction::emit(FILE *stream, CodeFunction *function)
{
    Block *b = Block::function(function);
    CodeSize size;

    if (b != NULL) {
	size = b->fragment();
	if (size != 0) {
	    GenContext context(stream, function, size);
	    b->emit(&context, function);
	}
	b->clear();
    }
}
