# include <stdlib.h>
# include <stdint.h>
# include <stdbool.h>
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
	printf("nil");
	break;

    case LPC_TYPE_INT:
	printf("int");
	break;

    case LPC_TYPE_FLOAT:
	printf("float");
	break;

    case LPC_TYPE_STRING:
	printf("string");
	break;

    case LPC_TYPE_OBJECT:
    case LPC_TYPE_CLASS:
	printf("object");
	break;

    case LPC_TYPE_ARRAY:
	printf("mixed *");
	break;

    case LPC_TYPE_MAPPING:
	printf("mapping");
	break;

    case LPC_TYPE_MIXED:
	printf("mixed");
	break;

    case LPC_TYPE_VOID:
	printf("void");
	break;

    default:
	printf("unknown");
	break;
    }

    for (i = LPC_TYPE_REF(type); i != 0; --i) {
	printf("*");
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
	printf("nil");
	break;

    case LPC_TYPE_INT:
	printf("int");
	break;

    case LPC_TYPE_FLOAT:
	printf("float");
	break;

    case LPC_TYPE_STRING:
	printf("string");
	break;

    case LPC_TYPE_OBJECT:
	printf("object");
	break;

    case LPC_TYPE_ARRAY:
	printf("mixed *");
	break;

    case LPC_TYPE_MAPPING:
	printf("mapping");
	break;

    case LPC_TYPE_CLASS:
	printf("class <%d, %d>", type->inherit, type->index);
	break;

    case LPC_TYPE_MIXED:
	printf("mixed");
	break;

    case LPC_TYPE_VOID:
	printf("void");
	break;

    default:
	printf("unknown");
	break;
    }

    for (i = LPC_TYPE_REF(type->type); i != 0; --i) {
	printf("*");
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
	printf("%04x", code->addr);
	if (code->line != line) {
	    printf("%5d", code->line);
	    line = code->line;
	} else {
	    printf("     ");
	}
	if (code->pop) {
	    printf(" P ");
	} else {
	    printf("   ");
	}

	switch (code->instruction) {
	case Code::INT:
	    printf("INT %lld\n", code->u.num);
	    break;

	case Code::FLOAT:
	    printf("FLOAT <%08X, %016llX>\n", code->u.flt.high,
		   code->u.flt.low);
	    break;

	case Code::STRING:
	    printf("STRING <%d, %d>\n", code->u.str.inherit, code->u.str.index);
	    break;

	case Code::PARAM:
	    printf("PARAM %d\n", code->u.param);
	    break;

	case Code::LOCAL:
	    printf("LOCAL %d\n", code->u.local);
	    break;

	case Code::GLOBAL:
	    printf("GLOBAL <%d, %d> ", code->u.var.inherit, code->u.var.index);
	    dis_type(code->u.var.type);
	    printf("\n");
	    break;

	case Code::INDEX:
	    printf("INDEX\n");
	    break;

	case Code::INDEX2:
	    printf("INDEX2\n");
	    break;

	case Code::SPREAD:
	    printf("SPREAD %d\n", code->u.spread);
	    break;

	case Code::SPREAD_STORES:
	    printf("SPREAD_STORES %d\n", code->u.spread);
	    break;

	case Code::AGGREGATE:
	    printf("AGGREGATE %d\n", code->size);
	    break;

	case Code::MAP_AGGREGATE:
	    printf("MAP_AGGREGATE %d\n", code->size);
	    break;

	case Code::CAST:
	    printf("CAST ");
	    dis_casttype(&code->u.type);
	    printf("\n");
	    break;

	case Code::INSTANCEOF:
	    printf("INSTANCEOF <%d, %d>\n", code->u.str.inherit,
		   code->u.str.index);
	    break;

	case Code::CHECK_RANGE:
	    printf("CHECK_RANGE\n");
	    break;

	case Code::CHECK_RANGE_FROM:
	    printf("CHECK_RANGE_FROM\n");
	    break;

	case Code::CHECK_RANGE_TO:
	    printf("CHECK_RANGE_TO\n");
	    break;

	case Code::STORES:
	    printf("STORES %d\n", code->size);
	    break;

	case Code::STORE_PARAM:
	    printf("STORE_PARAM %d\n", code->u.param);
	    break;

	case Code::STORE_LOCAL:
	    printf("STORE_LOCAL %d\n", code->u.local);
	    break;

	case Code::STORE_GLOBAL:
	    printf("STORE_GLOBAL <%d, %d>\n", code->u.var.inherit,
		   code->u.var.index);
	    break;

	case Code::STORE_INDEX:
	    printf("STORE_INDEX\n");
	    break;

	case Code::STORE_PARAM_INDEX:
	    printf("STORE_PARAM_INDEX %d\n", code->u.param);
	    break;

	case Code::STORE_LOCAL_INDEX:
	    printf("STORE_LOCAL_INDEX %d\n", code->u.local);
	    break;

	case Code::STORE_GLOBAL_INDEX:
	    printf("STORE_GLOBAL_INDEX <%d, %d>\n", code->u.var.inherit,
		   code->u.var.index);
	    break;

	case Code::STORE_INDEX_INDEX:
	    printf("STORE_INDEX_INDEX\n");
	    break;

	case Code::JUMP:
	    printf("JUMP %04x\n", code->u.addr);
	    break;

	case Code::JUMP_ZERO:
	    printf("JUMP_ZERO %04x\n", code->u.addr);
	    break;

	case Code::JUMP_NONZERO:
	    printf("JUMP_NONZERO %04x\n", code->u.addr);
	    break;

	case Code::SWITCH_INT:
	    printf("SWITCH_INT %04x\n", code->u.caseInt->addr);
	    for (i = 1; i < code->size; i++) {
		printf("             case %lld: %04x\n", code->u.caseInt[i].num,
		       code->u.caseInt[i].addr);
	    }
	    break;

	case Code::SWITCH_RANGE:
	    printf("SWITCH_RANGE %04x\n", code->u.caseRange->addr);
	    for (i = 1; i < code->size; i++) {
		printf("             case %lld..%lld: %04x\n",
		       code->u.caseRange[i].from, code->u.caseRange[i].to,
		       code->u.caseRange[i].addr);
	    }
	    break;

	case Code::SWITCH_STRING:
	    printf("SWITCH_STRING %04x\n", code->u.caseString->addr);
	    for (i = 1; i < code->size; i++) {
		if (code->u.caseString[i].str.inherit == 0 &&
		    code->u.caseString[i].str.index == 0xffff) {
		    printf("             case nil: %04x\n",
			   code->u.caseString[i].addr);
		} else {
		    printf("             case <%d, %d>: %04x\n",
			   code->u.caseString[i].str.inherit,
			   code->u.caseString[i].str.index,
			   code->u.caseString[i].addr);
		}
	    }
	    break;

	case Code::KFUNC:
	    printf("KFUNC %d ", code->u.kfun.func);
	    dis_type(code->u.kfun.type);
	    printf(" (%d)\n", code->u.kfun.nargs);
	    break;

	case Code::KFUNC_STORES:
	    printf("KFUNC_STORES %d ", code->u.kfun.func);
	    dis_type(code->u.kfun.type);
	    printf(" (%d)\n", code->u.kfun.nargs);
	    break;

	case Code::DFUNC:
	    printf("DFUNC <%d, %d> ", code->u.dfun.inherit, code->u.dfun.nargs);
	    dis_type(code->u.dfun.type);
	    printf(" (%d)\n", code->u.dfun.func);
	    break;

	case Code::FUNC:
	    printf("FUNC %d (%d)\n", code->u.fun.call, code->u.fun.nargs);
	    break;

	case Code::CATCH:
	    printf("CATCH %04x\n", code->u.addr);
	    break;

	case Code::END_CATCH:
	    printf("END_CATCH\n");
	    break;

	case Code::RLIMITS:
	    printf("RLIMITS\n");
	    break;

	case Code::RLIMITS_CHECK:
	    printf("RLIMITS_CHECK\n");
	    break;

	case Code::END_RLIMITS:
	    printf("END_RLIMITS\n");
	    break;

	case Code::RETURN:
	    printf("RETURN\n");
	    break;
	}
    }
}
