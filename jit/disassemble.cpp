# include <stdlib.h>
# include <stdint.h>
# include <string.h>
# include <stdio.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "block.h"
# include "disassemble.h"
# include "jitcomp.h"


/*
 * NAME:	disasm->type()
 * DESCRIPTION:	disassemble LPC type
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
 * NAME:	disasm->casttype()
 * DESCRIPTION:	disassemble cast type
 */
static void dis_casttype(LPCType *type)
{
    int i;

    switch (type->type & LPC_TYPE_MASK) {
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
	fprintf(stderr, "mixed");
	break;

    case LPC_TYPE_VOID:
	fprintf(stderr, "void");
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
 * NAME:	disasm->program()
 * DESCRIPTION:	disassemble program
 */
void dis_program(CodeFunction *func, Block *b)
{
    CodeLine line;
    Code *code;
    int i;

    line = 0;
    for (; b != NULL; b = b->next) {
	fprintf(stderr, "{\n");
	for (code = b->first; ; code = code->next) {
	    fprintf(stderr, " %04x", code->addr);
	    if (code->line != line) {
		fprintf(stderr, "%5d", code->line);
		line = code->line;
	    } else {
		fprintf(stderr, "     ");
	    }
	    if (code->pop) {
		fprintf(stderr, " P ");
	    } else {
		fprintf(stderr, "   ");
	    }

	    switch (code->instruction) {
	    case Code::INT:
		fprintf(stderr, "INT %lld\n", code->num);
		break;

	    case Code::FLOAT:
		fprintf(stderr, "FLOAT <%08X, %016llX>\n", code->flt.high,
		       code->flt.low);
		break;

	    case Code::STRING:
		fprintf(stderr, "STRING <%d, %d>\n", code->str.inherit,
			code->str.index);
		break;

	    case Code::PARAM:
		fprintf(stderr, "PARAM %d\n", code->param);
		break;

	    case Code::LOCAL:
		fprintf(stderr, "LOCAL %d\n", code->local);
		break;

	    case Code::GLOBAL:
		fprintf(stderr, "GLOBAL <%d, %d> ", code->var.inherit,
			code->var.index);
		dis_type(code->var.type);
		fprintf(stderr, "\n");
		break;

	    case Code::INDEX:
		fprintf(stderr, "INDEX\n");
		break;

	    case Code::INDEX2:
		fprintf(stderr, "INDEX2\n");
		break;

	    case Code::SPREAD:
		fprintf(stderr, "SPREAD %d\n", code->spread);
		break;

	    case Code::SPREAD_STORES:
		fprintf(stderr, "SPREAD_STORES %d\n", code->spread);
		break;

	    case Code::AGGREGATE:
		fprintf(stderr, "AGGREGATE %d\n", code->size);
		break;

	    case Code::MAP_AGGREGATE:
		fprintf(stderr, "MAP_AGGREGATE %d\n", code->size);
		break;

	    case Code::CAST:
		fprintf(stderr, "CAST ");
		dis_casttype(&code->type);
		fprintf(stderr, "\n");
		break;

	    case Code::INSTANCEOF:
		fprintf(stderr, "INSTANCEOF <%d, %d>\n", code->str.inherit,
		       code->str.index);
		break;

	    case Code::CHECK_RANGE:
		fprintf(stderr, "CHECK_RANGE\n");
		break;

	    case Code::CHECK_RANGE_FROM:
		fprintf(stderr, "CHECK_RANGE_FROM\n");
		break;

	    case Code::CHECK_RANGE_TO:
		fprintf(stderr, "CHECK_RANGE_TO\n");
		break;

	    case Code::STORES:
		fprintf(stderr, "STORES %d\n", code->size);
		break;

	    case Code::STORE_PARAM:
		fprintf(stderr, "STORE_PARAM %d\n", code->param);
		break;

	    case Code::STORE_LOCAL:
		fprintf(stderr, "STORE_LOCAL %d\n", code->local);
		break;

	    case Code::STORE_GLOBAL:
		fprintf(stderr, "STORE_GLOBAL <%d, %d>\n", code->var.inherit,
			code->var.index);
		break;

	    case Code::STORE_INDEX:
		fprintf(stderr, "STORE_INDEX\n");
		break;

	    case Code::STORE_PARAM_INDEX:
		fprintf(stderr, "STORE_PARAM_INDEX %d\n", code->param);
		break;

	    case Code::STORE_LOCAL_INDEX:
		fprintf(stderr, "STORE_LOCAL_INDEX %d\n", code->local);
		break;

	    case Code::STORE_GLOBAL_INDEX:
		fprintf(stderr, "STORE_GLOBAL_INDEX <%d, %d>\n",
			code->var.inherit, code->var.index);
		break;

	    case Code::STORE_INDEX_INDEX:
		fprintf(stderr, "STORE_INDEX_INDEX\n");
		break;

	    case Code::JUMP:
		fprintf(stderr, "JUMP %04x\n", code->target);
		break;

	    case Code::JUMP_ZERO:
		fprintf(stderr, "JUMP_ZERO %04x\n", code->target);
		break;

	    case Code::JUMP_NONZERO:
		fprintf(stderr, "JUMP_NONZERO %04x\n", code->target);
		break;

	    case Code::SWITCH_INT:
		fprintf(stderr, "SWITCH_INT %04x\n", code->caseInt->addr);
		for (i = 1; i < code->size; i++) {
		    fprintf(stderr, "              case %lld: %04x\n",
			    code->caseInt[i].num, code->caseInt[i].addr);
		}
		break;

	    case Code::SWITCH_RANGE:
		fprintf(stderr, "SWITCH_RANGE %04x\n", code->caseRange->addr);
		for (i = 1; i < code->size; i++) {
		    fprintf(stderr, "              case %lld..%lld: %04x\n",
			    code->caseRange[i].from, code->caseRange[i].to,
			    code->caseRange[i].addr);
		}
		break;

	    case Code::SWITCH_STRING:
		fprintf(stderr, "SWITCH_STRING %04x\n", code->caseString->addr);
		for (i = 1; i < code->size; i++) {
		    if (code->caseString[i].str.inherit == 0 &&
			code->caseString[i].str.index == 0xffff) {
			fprintf(stderr, "              case nil: %04x\n",
				code->caseString[i].addr);
		    } else {
			fprintf(stderr, "              case <%d, %d>: %04x\n",
				code->caseString[i].str.inherit,
				code->caseString[i].str.index,
				code->caseString[i].addr);
		    }
		}
		break;

	    case Code::KFUNC:
		fprintf(stderr, "KFUNC %d ", code->kfun.func);
		dis_type(code->kfun.type);
		fprintf(stderr, " (%d)\n", code->kfun.nargs);
		break;

	    case Code::KFUNC_STORES:
		fprintf(stderr, "KFUNC_STORES %d ", code->kfun.func);
		dis_type(code->kfun.type);
		fprintf(stderr, " (%d)\n", code->kfun.nargs);
		break;

	    case Code::DFUNC:
		fprintf(stderr, "DFUNC <%d, %d> ", code->dfun.inherit,
			code->dfun.nargs);
		dis_type(code->dfun.type);
		fprintf(stderr, " (%d)\n", code->dfun.func);
		break;

	    case Code::FUNC:
		fprintf(stderr, "FUNC %d (%d)\n", code->fun.call,
			code->fun.nargs);
		break;

	    case Code::CATCH:
		fprintf(stderr, "CATCH %04x\n", code->target);
		break;

	    case Code::END_CATCH:
		fprintf(stderr, "END_CATCH\n");
		break;

	    case Code::RLIMITS:
		fprintf(stderr, "RLIMITS\n");
		break;

	    case Code::RLIMITS_CHECK:
		fprintf(stderr, "RLIMITS_CHECK\n");
		break;

	    case Code::END_RLIMITS:
		fprintf(stderr, "END_RLIMITS\n");
		break;

	    case Code::RETURN:
		fprintf(stderr, "RETURN\n");
		break;

	    default:
		fatal("unknown instruction %d\n", code->instruction);
	    }
	    if (code == b->last) {
		break;
	    }
	}

	fprintf(stderr, "} ");
	if (b->nfollow != 0) {
	    fprintf(stderr, "=> ");
	    for (i = 0; i < b->nfollow; i++) {
		fprintf(stderr, "%04x", b->follow[i]->first->addr);
		if (i != b->nfollow - 1) {
		    fprintf(stderr, ", ");
		}
	    }
	    fprintf(stderr, " ");
	}
    }
    fprintf(stderr, "\n");
}
