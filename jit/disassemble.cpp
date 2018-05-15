# include <stdlib.h>
# include <stdint.h>
# include <string.h>
# include <stdio.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "disassemble.h"
# include "jit.h"


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
void dis_program(CodeFunction *func)
{
    CodeLine line;
    Code *code;
    int i;

    line = 0;
    for (code = func->list; code != NULL; code = code->next) {
	fprintf(stderr, "%04x", code->addr);
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
	    fprintf(stderr, "INT %lld\n", code->u.num);
	    break;

	case Code::FLOAT:
	    fprintf(stderr, "FLOAT <%08X, %016llX>\n", code->u.flt.high,
		   code->u.flt.low);
	    break;

	case Code::STRING:
	    fprintf(stderr, "STRING <%d, %d>\n", code->u.str.inherit,
		    code->u.str.index);
	    break;

	case Code::PARAM:
	    fprintf(stderr, "PARAM %d\n", code->u.param);
	    break;

	case Code::LOCAL:
	    fprintf(stderr, "LOCAL %d\n", code->u.local);
	    break;

	case Code::GLOBAL:
	    fprintf(stderr, "GLOBAL <%d, %d> ", code->u.var.inherit,
		    code->u.var.index);
	    dis_type(code->u.var.type);
	    fprintf(stderr, "\n");
	    break;

	case Code::INDEX:
	    fprintf(stderr, "INDEX\n");
	    break;

	case Code::INDEX2:
	    fprintf(stderr, "INDEX2\n");
	    break;

	case Code::SPREAD:
	    fprintf(stderr, "SPREAD %d\n", code->u.spread);
	    break;

	case Code::SPREAD_STORES:
	    fprintf(stderr, "SPREAD_STORES %d\n", code->u.spread);
	    break;

	case Code::AGGREGATE:
	    fprintf(stderr, "AGGREGATE %d\n", code->size);
	    break;

	case Code::MAP_AGGREGATE:
	    fprintf(stderr, "MAP_AGGREGATE %d\n", code->size);
	    break;

	case Code::CAST:
	    fprintf(stderr, "CAST ");
	    dis_casttype(&code->u.type);
	    fprintf(stderr, "\n");
	    break;

	case Code::INSTANCEOF:
	    fprintf(stderr, "INSTANCEOF <%d, %d>\n", code->u.str.inherit,
		   code->u.str.index);
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
	    fprintf(stderr, "STORE_PARAM %d\n", code->u.param);
	    break;

	case Code::STORE_LOCAL:
	    fprintf(stderr, "STORE_LOCAL %d\n", code->u.local);
	    break;

	case Code::STORE_GLOBAL:
	    fprintf(stderr, "STORE_GLOBAL <%d, %d>\n", code->u.var.inherit,
		    code->u.var.index);
	    break;

	case Code::STORE_INDEX:
	    fprintf(stderr, "STORE_INDEX\n");
	    break;

	case Code::STORE_PARAM_INDEX:
	    fprintf(stderr, "STORE_PARAM_INDEX %d\n", code->u.param);
	    break;

	case Code::STORE_LOCAL_INDEX:
	    fprintf(stderr, "STORE_LOCAL_INDEX %d\n", code->u.local);
	    break;

	case Code::STORE_GLOBAL_INDEX:
	    fprintf(stderr, "STORE_GLOBAL_INDEX <%d, %d>\n",
		    code->u.var.inherit, code->u.var.index);
	    break;

	case Code::STORE_INDEX_INDEX:
	    fprintf(stderr, "STORE_INDEX_INDEX\n");
	    break;

	case Code::JUMP:
	    fprintf(stderr, "JUMP %04x\n", code->u.addr);
	    break;

	case Code::JUMP_ZERO:
	    fprintf(stderr, "JUMP_ZERO %04x\n", code->u.addr);
	    break;

	case Code::JUMP_NONZERO:
	    fprintf(stderr, "JUMP_NONZERO %04x\n", code->u.addr);
	    break;

	case Code::SWITCH_INT:
	    fprintf(stderr, "SWITCH_INT %04x\n", code->u.caseInt->addr);
	    for (i = 1; i < code->size; i++) {
		fprintf(stderr, "             case %lld: %04x\n",
			code->u.caseInt[i].num, code->u.caseInt[i].addr);
	    }
	    break;

	case Code::SWITCH_RANGE:
	    fprintf(stderr, "SWITCH_RANGE %04x\n", code->u.caseRange->addr);
	    for (i = 1; i < code->size; i++) {
		fprintf(stderr, "             case %lld..%lld: %04x\n",
			code->u.caseRange[i].from, code->u.caseRange[i].to,
			code->u.caseRange[i].addr);
	    }
	    break;

	case Code::SWITCH_STRING:
	    fprintf(stderr, "SWITCH_STRING %04x\n", code->u.caseString->addr);
	    for (i = 1; i < code->size; i++) {
		if (code->u.caseString[i].str.inherit == 0 &&
		    code->u.caseString[i].str.index == 0xffff) {
		    fprintf(stderr, "             case nil: %04x\n",
			    code->u.caseString[i].addr);
		} else {
		    fprintf(stderr, "             case <%d, %d>: %04x\n",
			    code->u.caseString[i].str.inherit,
			    code->u.caseString[i].str.index,
			    code->u.caseString[i].addr);
		}
	    }
	    break;

	case Code::KFUNC:
	    fprintf(stderr, "KFUNC %d ", code->u.kfun.func);
	    dis_type(code->u.kfun.type);
	    fprintf(stderr, " (%d)\n", code->u.kfun.nargs);
	    break;

	case Code::KFUNC_STORES:
	    fprintf(stderr, "KFUNC_STORES %d ", code->u.kfun.func);
	    dis_type(code->u.kfun.type);
	    fprintf(stderr, " (%d)\n", code->u.kfun.nargs);
	    break;

	case Code::DFUNC:
	    fprintf(stderr, "DFUNC <%d, %d> ", code->u.dfun.inherit,
		    code->u.dfun.nargs);
	    dis_type(code->u.dfun.type);
	    fprintf(stderr, " (%d)\n", code->u.dfun.func);
	    break;

	case Code::FUNC:
	    fprintf(stderr, "FUNC %d (%d)\n", code->u.fun.call,
		    code->u.fun.nargs);
	    break;

	case Code::CATCH:
	    fprintf(stderr, "CATCH %04x\n", code->u.addr);
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
	}
    }
}
