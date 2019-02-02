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
# include "disasm.h"
# include "jitcomp.h"


DisCode::DisCode(CodeFunction *function) : TypedCode(function) { }

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
CodeLine DisCode::emit(CodeLine line)
{
    int i;

    fprintf(stderr, " %04x", addr);
    if (this->line != line) {
	fprintf(stderr, "%5d", this->line);
	line = this->line;
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
	fprintf(stderr, "INT %lld\t\t\t\t", num);
	break;

    case FLOAT:
	fprintf(stderr, "FLOAT <%08X, %016llX>\t", flt.high, flt.low);
	break;

    case STRING:
	fprintf(stderr, "STRING <%d, %d>\t\t\t", str.inherit, str.index);
	break;

    case PARAM:
	fprintf(stderr, "PARAM %d\t\t\t\t", param);
	break;

    case LOCAL:
	fprintf(stderr, "LOCAL %d\t\t\t\t", local);
	break;

    case GLOBAL:
	fprintf(stderr, "GLOBAL <%d, %d>\t\t\t", var.inherit, var.index);
	break;

    case INDEX:
	fprintf(stderr, "INDEX\t\t\t\t");
	break;

    case INDEX2:
	fprintf(stderr, "INDEX2\t\t\t\t");
	break;

    case SPREAD:
	fprintf(stderr, "SPREAD\t\t\t\t");
	break;

    case SPREAD_STORES:
	fprintf(stderr, "SPREAD_STORES %d\t\t\t", spread);
	break;

    case SPREADX:
	fprintf(stderr, "SPREADX %d ", spread);
	dis_casttype(&type);
	fprintf(stderr, "\t\t\t");
	break;

    case AGGREGATE:
	fprintf(stderr, "AGGREGATE %d\t\t\t", size);
	break;

    case MAP_AGGREGATE:
	fprintf(stderr, "MAP_AGGREGATE %d\t\t\t", size);
	break;

    case CAST:
	fprintf(stderr, "CAST ");
	dis_casttype(&type);
	fprintf(stderr, "\t\t\t");
	break;

    case CASTX:
	fprintf(stderr, "CASTX ");
	dis_casttype(&type);
	fprintf(stderr, "\t\t\t");
	break;

    case INSTANCEOF:
	fprintf(stderr, "INSTANCEOF <%d, %d>\t\t\t", str.inherit, str.index);
	break;

    case CHECK_RANGE:
	fprintf(stderr, "CHECK_RANGE\t\t\t");
	break;

    case CHECK_RANGE_FROM:
	fprintf(stderr, "CHECK_RANGE_FROM\t\t\t");
	break;

    case CHECK_RANGE_TO:
	fprintf(stderr, "CHECK_RANGE_TO\t\t\t");
	break;

    case STORES:
	fprintf(stderr, "STORES %d\t\t\t\t", size);
	break;

    case STORE_PARAM:
	fprintf(stderr, "STORE_PARAM %d\t\t\t", param);
	break;

    case STORE_LOCAL:
	fprintf(stderr, "STORE_LOCAL %d\t\t\t", local);
	break;

    case STORE_GLOBAL:
	fprintf(stderr, "STORE_GLOBAL <%d, %d>\t\t", var.inherit, var.index);
	break;

    case STORE_INDEX:
	fprintf(stderr, "STORE_INDEX\t\t\t");
	break;

    case STORE_PARAM_INDEX:
	fprintf(stderr, "STORE_PARAM_INDEX %d\t\t", param);
	break;

    case STORE_LOCAL_INDEX:
	fprintf(stderr, "STORE_LOCAL_INDEX %d\t\t", local);
	break;

    case STORE_GLOBAL_INDEX:
	fprintf(stderr, "STORE_GLOBAL_INDEX <%d, %d>\t\t", var.inherit,
		var.index);
	break;

    case STORE_INDEX_INDEX:
	fprintf(stderr, "STORE_INDEX_INDEX\t\t\t");
	break;

    case STOREX_PARAM:
	fprintf(stderr, "STOREX_PARAM %d\t\t\t", param);
	break;

    case STOREX_LOCAL:
	fprintf(stderr, "STOREX_LOCAL %d\t\t\t", local);
	break;

    case STOREX_GLOBAL:
	fprintf(stderr, "STOREX_GLOBAL <%d, %d>\t\t", var.inherit, var.index);
	break;

    case STOREX_INDEX:
	fprintf(stderr, "STOREX_INDEX\t\t\t");
	break;

    case STOREX_PARAM_INDEX:
	fprintf(stderr, "STOREX_PARAM_INDEX %d\t\t", param);
	break;

    case STOREX_LOCAL_INDEX:
	fprintf(stderr, "STOREX_LOCAL_INDEX %d\t\t", local);
	break;

    case STOREX_GLOBAL_INDEX:
	fprintf(stderr, "STOREX_GLOBAL_INDEX <%d, %d>\t\t", var.inherit,
		var.index);
	break;

    case STOREX_INDEX_INDEX:
	fprintf(stderr, "STOREX_INDEX_INDEX\t\t\t");
	break;

    case JUMP:
	fprintf(stderr, "JUMP %04x\t\t\t\t", target);
	break;

    case JUMP_ZERO:
	fprintf(stderr, "JUMP_ZERO %04x\t\t\t", target);
	break;

    case JUMP_NONZERO:
	fprintf(stderr, "JUMP_NONZERO %04x\t\t\t", target);
	break;

    case SWITCH_INT:
	fprintf(stderr, "SWITCH_INT %04x\n", caseInt->addr);
	for (i = 1; i < size; i++) {
	    fprintf(stderr, "              case %lld: %04x\n", caseInt[i].num,
		    caseInt[i].addr);
	}
	break;

    case SWITCH_RANGE:
	fprintf(stderr, "SWITCH_RANGE %04x\n", caseRange->addr);
	for (i = 1; i < size; i++) {
	    fprintf(stderr, "              case %lld..%lld: %04x\n",
		    caseRange[i].from, caseRange[i].to, caseRange[i].addr);
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
	fprintf(stderr, "KFUNC %d (%d)\t\t\t", kfun.func, kfun.nargs);
	break;

    case KFUNC_STORES:
	fprintf(stderr, "KFUNC_STORES %d (%d)\t\t", kfun.func, kfun.nargs);
	break;

    case DFUNC:
	fprintf(stderr, "DFUNC <%d, %d> (%d)\t\t\t", dfun.inherit, dfun.func,
		dfun.nargs);
	break;

    case FUNC:
	fprintf(stderr, "FUNC %d (%d)\t\t\t\t", fun.call, fun.nargs);
	break;

    case CATCH:
	fprintf(stderr, "CATCH %04x\t\t\t\t", target);
	break;

    case END_CATCH:
	fprintf(stderr, "END_CATCH\t\t\t\t");
	break;

    case RLIMITS:
	fprintf(stderr, "RLIMITS\t\t\t\t");
	break;

    case RLIMITS_CHECK:
	fprintf(stderr, "RLIMITS_CHECK\t\t\t");
	break;

    case END_RLIMITS:
	fprintf(stderr, "END_RLIMITS\t\t\t");
	break;

    case RETURN:
	fprintf(stderr, "RETURN\t\t\t\t");
	break;

    default:
	fatal("unknown instruction %d", instruction);
    }

    return line;
}


DisBlock::DisBlock(Code *first, Code *last, CodeSize size) :
    TypedBlock(first, last, size)
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
void DisBlock::emit(BlockContext *context)
{
    CodeLine line;
    Block *b;
    Code *code;
    CodeSize i, sp;

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
	fprintf(stderr, "\n");
	for (code = b->first; ; code = code->next) {
	    line = code->emit(line);
	    switch (code->instruction) {
	    case Code::SWITCH_INT:
	    case Code::SWITCH_RANGE:
	    case Code::SWITCH_STRING:
		break;

	    default:
		sp = ((TypedCode *) code)->stackPointer();
		i = context->depth(sp);
		fprintf(stderr, "; %d ", i);
		if (i != 0) {
		    dis_type(context->topType(sp));
		}
		fprintf(stderr, "\n");
	    }
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
