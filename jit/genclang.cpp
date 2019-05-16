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
# include "genclang.h"
# include "jitcomp.h"

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
bool ClangCode::onStack(FlowContext *context, StackSize sp)
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
	if (ref == FlowContext::INITIAL) {
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
	if (ref == FlowContext::INITIAL) {
	    ref = 0;
	}
	sprintf(buf, "%%l%dr%d", local, ref);
    }
    return buf;
}

/*
 * return a name for the parameter this Code refers to
 */
char *ClangCode::paramRef(FlowContext *context, LPCParam param)
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
char *ClangCode::localRef(FlowContext *context, LPCLocal local)
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
void ClangCode::result(FlowContext *context)
{
    StackSize sp;

    sp = stackPointer();
    if (!pop && onStack(context, sp)) {
	if (context->get(sp).type == LPC_TYPE_INT) {
	    fprintf(stderr, "\tlpc_vm_int(f, %s)\n", tmpRef(sp));
	} else {
	    fprintf(stderr, "\tlpc_vm_float(f, %s)\n", tmpRef(sp));
	}
    }
    context->sp = sp;
}

/*
 * emit pseudo instructions for this Code
 */
void ClangCode::emit(FlowContext *context)
{
    StackSize sp;
    TVC val;
    long double d;

    if (line != context->line) {
	fprintf(stderr, "# line %d\n", line);
	context->line = line;
    }

    sp = stackPointer();
    switch (instruction) {
    case INT:
	fprintf(stderr, "\t%s <int> = %lld\n", tmpRef(sp), (long long) num);
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
	fprintf(stderr, "\t%s <float> = %.26LgL\n", tmpRef(sp), d);
	result(context);
	return;

    case STRING:
	fprintf(stderr, "\tlpc_vm_string(f, %u, %u)\n", str.inherit, str.index);
	break;

    case PARAM:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
		    paramRef(context, param));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
		    paramRef(context, param));
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_param(f, %d)\n", param);
	    break;
	}
	break;

    case LOCAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
		    localRef(context, local));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
		    localRef(context, local));
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_local(f, %d)\n", local);
	    break;
	}
	break;

    case GLOBAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_global_int(f, %u, %u)\n",
		    tmpRef(sp), var.inherit, var.index);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_global_float(f, %u, %u)\n",
		    tmpRef(sp), var.inherit, var.index);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_global(f, %u, %u)\n", var.inherit,
		    var.index);
	    break;
	}
	break;

    case INDEX:
	if (context->get(sp).type == LPC_TYPE_INT) {
	    fprintf(stderr, "\t%s <int> = lpc_vm_index_int(f, false)\n",
		    tmpRef(sp));
	    result(context);
	    return;
	}

	fprintf(stderr, "\tlpc_vm_index(f, false)\n");
	break;

    case INDEX2:
	if (context->get(sp).type == LPC_TYPE_INT) {
	    fprintf(stderr, "\t%s <int> = lpc_vm_index_int(f, true)\n",
		    tmpRef(sp));
	    result(context);
	    return;
	}

	fprintf(stderr, "\tlpc_vm_index(f, true)\n");
	break;

    case AGGREGATE:
	fprintf(stderr, "\tlpc_vm_aggregate(f, %d)\n", size);
	break;

    case MAP_AGGREGATE:
	fprintf(stderr, "\tlpc_vm_map_aggregate(f, %d)\n", size);
	break;

    case CAST:
	switch (type.type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_cast_int(f)\n", tmpRef(sp));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_cast_float(f)\n",
		    tmpRef(sp));
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_cast(f, %d, %u, %u)\n", type.type,
		    type.inherit, type.index);
	    break;
	}
	break;

    case INSTANCEOF:
	fprintf(stderr, "\t%s <int> = lpc_vm_instanceof(f)\n", tmpRef(sp));
	result(context);
	return;

    case CHECK_RANGE:
	fprintf(stderr, "\tlpc_vm_range(f)\n");
	break;

    case CHECK_RANGE_FROM:
	fprintf(stderr, "\tlpc_vm_range_from(f)\n");
	break;

    case CHECK_RANGE_TO:
	fprintf(stderr, "\tlpc_vm_range_to(f)\n");
	break;

    case STORE_PARAM:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = %s\n", paramRef(context, param),
		    tmpRef(context->sp));
	    fprintf(stderr, "\tlpc_vm_store_param_int(f, %d, %s)\n", param,
		    tmpRef(context->sp));
	    if (!pop) {
		fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = %s\n", paramRef(context, param),
		    tmpRef(context->sp));
	    fprintf(stderr, "\tlpc_vm_store_param_float(f, %d, %s)\n", param,
		    tmpRef(context->sp));
	    if (!pop) {
		fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_param(f, %d)\n", param);
	    break;
	}
	break;

    case STORE_LOCAL:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = %s\n", localRef(context, local),
		    tmpRef(context->sp));
	    if (context->level > 0) {
		fprintf(stderr, "\tlpc_vm_store_local_int(f, %d, %s)\n", local,
			tmpRef(context->sp));
	    }
	    if (!pop) {
		fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = %s\n", localRef(context, local),
		    tmpRef(context->sp));
	    if (context->level > 0) {
		fprintf(stderr, "\tlpc_vm_store_local_float(f, %d, %s)\n", local,
			tmpRef(context->sp));
	    }
	    if (!pop) {
		fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_local(f, %d)\n", local);
	    break;
	}
	break;

    case STORE_GLOBAL:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\tlpc_vm_store_global_int(f, %u, %u, %s)\n",
		    var.inherit, var.index, tmpRef(context->sp));
	    if (!pop) {
		fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\tlpc_vm_store_global_float(f, %u, %u, %s)\n",
		    var.inherit, var.index, tmpRef(context->sp));
	    if (!pop) {
		fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
	    }
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_global(f, %u, %u)\n", var.inherit,
		    var.index);
	    break;
	}
	break;

    case STORE_INDEX:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_store_index_int(f)\n",
		    tmpRef(sp));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_store_index_float(f)\n",
		    tmpRef(sp));
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_index(f)\n");
	    break;
	}
	break;

    case STORE_PARAM_INDEX:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_store_param_index_int(f, %d)\n",
		    tmpRef(sp), param);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_store_param_index_float(f, %d)\n",
		    tmpRef(sp), param);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_param_index(f, %d)\n", param);
	    break;
	}
	break;

    case STORE_LOCAL_INDEX:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_store_local_index_int(f, %d)\n",
		    tmpRef(sp), local);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_store_local_index_float(f, %d)\n",
		    tmpRef(sp), local);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_local_index(f, %d)\n", local);
	    break;
	}
	break;

    case STORE_GLOBAL_INDEX:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_store_global_index_int(f, %u, %u)\n",
		    tmpRef(sp), var.inherit, var.index);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_store_global_index_float(f, %u, %u)\n",
		    tmpRef(sp), var.inherit, var.index);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_global_index(f, %u, %u)\n",
		    var.inherit, var.index);
	    break;
	}
	break;

    case STORE_INDEX_INDEX:
	switch (context->get(context->sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_store_index_index_int(f)\n",
		    tmpRef(sp));
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_store_index_index_float(f)\n",
		    tmpRef(sp));
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_store_index_index(f)\n");
	    break;
	}
	break;

    case STORES:
	fprintf(stderr, "\tlpc_vm_stores(f, %d)\n", size);
	if (!context->stores(size, NULL)) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	}
	break;

    case STORES_LVAL:
	fprintf(stderr, "\tlpc_vm_stores_lval(f, %d)\n", size);
	if (!context->stores(size, (pop) ? this : NULL)) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (pop) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	context->sp = sp;
	return;

    case STORES_SPREAD:
	fprintf(stderr, "\tlpc_vm_stores_spread(f, %d, %d, %u, %u)\n", spread,
		type.type, type.inherit, type.index);
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_CAST:
	fprintf(stderr, "\tlpc_vm_stores_cast(f, %d, %u, %u)\n", type.type,
		type.inherit, type.index);
	context->castType = simplifiedType(type.type);
	break;

    case STORES_PARAM:
	switch (context->castType) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_stores_param_int(f, %d)\n",
		    paramRef(context, param), param);
	    break;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_stores_param_float(f, %d)\n",
		    paramRef(context, param), param);
	    break;

	default:
	    fprintf(stderr, "\tlpc_vm_stores_param(f, %d)\n", param);
	    break;
	}
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_LOCAL:
	switch (context->castType) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_stores_local_int(f, %d)\n",
		    localRef(context, local), local);
	    break;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_stores_local_float(f, %d)\n",
		    localRef(context, local), local);
	    break;

	default:
	    fprintf(stderr, "\tlpc_vm_stores_local(f, %d)\n", local);
	    break;
	}
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_GLOBAL:
	fprintf(stderr, "\tlpc_vm_stores_global(f, %u, %u)\n", var.inherit,
		var.index);
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_INDEX:
	fprintf(stderr, "\tlpc_vm_stores_index(f)\n");
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_PARAM_INDEX:
	fprintf(stderr, "\tlpc_vm_stores_param_index(f, %d)\n", param);
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_LOCAL_INDEX:
	fprintf(stderr, "\tlpc_vm_stores_local_index(f, %d)\n", local);
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_GLOBAL_INDEX:
	fprintf(stderr, "\tlpc_vm_stores_global_index(f, %u, %u)\n",
		var.inherit, var.index);
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case STORES_INDEX_INDEX:
	fprintf(stderr, "\tlpc_vm_stores_index_index(f)\n");
	if (!context->storeN()) {
	    fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    if (context->storePop() != NULL) {
		fprintf(stderr, "\tlpc_vm_pop(f)\n");
	    }
	}
	break;

    case JUMP:
	// XXX account ticks for backward jump
	break;

    case JUMP_ZERO:
	// XXX account ticks for backward jump
	if (context->get(context->sp).type == LPC_TYPE_INT) {
	    fprintf(stderr, "\tbranch %s, %%L%04x, %%L%04x\n",
		    tmpRef(context->sp), context->next, target);
	} else {
	    fprintf(stderr, "\t%s <int> = lpc_vm_pop_bool(f)\n", tmpRef(sp));
	    fprintf(stderr, "\tbranch %s, %%L%04x, %%L%04x\n",
		    tmpRef(context->sp), context->next, target);
	}
	context->sp = sp;
	return;

    case JUMP_NONZERO:
	// XXX account ticks for backward jump
	if (context->get(context->sp).type == LPC_TYPE_INT) {
	    fprintf(stderr, "\tbranch %s, %%L%04x, %%L%04x\n",
		    tmpRef(context->sp), target, context->next);
	} else {
	    fprintf(stderr, "\t%s <int> = lpc_vm_pop_bool(f)\n", tmpRef(sp));
	    fprintf(stderr, "\tbranch %s, %%L%04x, %%L%04x\n",
		    tmpRef(context->sp), target, context->next);
	}
	context->sp = sp;
	return;

    case SWITCH_INT:
	// XXX account ticks for backward jump
	fprintf(stderr, "\tlpc_vm_switch_int\n");
	break;

    case SWITCH_RANGE:
	// XXX account ticks for backward jump
	fprintf(stderr, "\tlpc_vm_switch_range\n");
	break;

    case SWITCH_STRING:
	// XXX account ticks for backward jump
	fprintf(stderr, "\tlpc_vm_switch_string\n");
	break;

    case KFUNC:
    case KFUNC_LVAL:
	switch (kfun.func) {
	case KF_ADD_INT:
	    fprintf(stderr, "\t%s <int> = add_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_ADD1_INT:
	    fprintf(stderr, "\t%s <int> = add1_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_AND_INT:
	    fprintf(stderr, "\t%s <int> = and_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_DIV_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_div_int(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_EQ_INT:
	    fprintf(stderr, "\t%s <int> = eq_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_GE_INT:
	    fprintf(stderr, "\t%s <int> = ge_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_GT_INT:
	    fprintf(stderr, "\t%s <int> = gt_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LE_INT:
	    fprintf(stderr, "\t%s <int> = le_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LSHIFT_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_lshift_int(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LT_INT:
	    fprintf(stderr, "\t%s <int> = lt_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_MOD_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_mod_int(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_MULT_INT:
	    fprintf(stderr, "\t%s <int> = mult_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NE_INT:
	    fprintf(stderr, "\t%s <int> = ne_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NEG_INT:
	    fprintf(stderr, "\t%s <int> = neg_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NOT_INT:
	    fprintf(stderr, "\t%s <int> = not_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_OR_INT:
	    fprintf(stderr, "\t%s <int> = or_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_RSHIFT_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_rshift_int(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB_INT:
	    fprintf(stderr, "\t%s <int> = sub_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB1_INT:
	    fprintf(stderr, "\t%s <int> = sub1_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_TOFLOAT:
	    switch (context->get(context->sp).type) {
	    case LPC_TYPE_INT:
		fprintf(stderr, "\t%s <float> = tofloat_int(%s)\n",
			tmpRef(sp), tmpRef(context->sp));
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		fprintf(stderr, "\t%s <float> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
		result(context);
		return;

	    default:
		fprintf(stderr, "\t%s <float> = lpc_vm_tofloat(f)\n",
			tmpRef(sp));
		result(context);
		return;
	    }
	    break;

	case KF_TOINT:
	    switch (context->get(context->sp).type) {
	    case LPC_TYPE_INT:
		fprintf(stderr, "\t%s <int> = %s\n", tmpRef(sp),
			tmpRef(context->sp));
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		fprintf(stderr, "\t%s <int> = lpc_vm_toint_float(f, %s)\n",
			tmpRef(sp), tmpRef(context->sp));
		result(context);
		return;

	    default:
		fprintf(stderr, "\t%s <int> = lpc_vm_toint(f)\n",
			tmpRef(sp));
		result(context);
		return;
	    }
	    break;

	case KF_TST_INT:
	    fprintf(stderr, "\t%s <int> = tst_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_UMIN_INT:
	    fprintf(stderr, "\t%s <int> = umin_int(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_XOR_INT:
	    fprintf(stderr, "\t%s <int> = xor_int(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NIL:
	    fprintf(stderr, "\tlpc_vm_nil(f)\n");
	    break;

	case KF_ADD_FLT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_add_float(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_ADD1_FLT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_add_float(f, %s, 1.0)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_DIV_FLT:
	    fprintf(stderr, "\t%s <float> = div_float(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_EQ_FLT:
	    fprintf(stderr, "\t%s <float> = eq_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_GE_FLT:
	    fprintf(stderr, "\t%s <float> = ge_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_GT_FLT:
	    fprintf(stderr, "\t%s <float> = gt_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LE_FLT:
	    fprintf(stderr, "\t%s <float> = le_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_LT_FLT:
	    fprintf(stderr, "\t%s <float> = lt_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_MULT_FLT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_mult_float(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NE_FLT:
	    fprintf(stderr, "\t%s <float> = ne_float(%s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_NOT_FLT:
	    fprintf(stderr, "\t%s <float> = not_float(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB_FLT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_sub_float(f, %s, %s)\n",
		    tmpRef(sp), tmpRef(context->nextSp()),
		    tmpRef(context->sp));
	    result(context);
	    return;

	case KF_SUB1_FLT:
	    fprintf(stderr, "\t%s <float> = lpc_vm_sub_float(f, %s, 1.0)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_TST_FLT:
	    fprintf(stderr, "\t%s <float> = tst_float(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	case KF_UMIN_FLT:
	    fprintf(stderr, "\t%s <float> = umin_float(%s)\n",
		    tmpRef(sp), tmpRef(context->sp));
	    result(context);
	    return;

	default:
	    switch (context->get(sp).type) {
	    case LPC_TYPE_INT:
		fprintf(stderr, "\t%s <int> = lpc_vm_kfunc_int(f, %d, %d)\n",
			tmpRef(sp), kfun.func, kfun.nargs);
		result(context);
		return;

	    case LPC_TYPE_FLOAT:
		fprintf(stderr,
			"\t%s <float> = lpc_vm_kfunc_float(f, %d, %d)\n",
			tmpRef(sp), kfun.func, kfun.nargs);
		result(context);
		return;

	    default:
		fprintf(stderr, "\tlpc_vm_kfunc(f, %d, %d)\n", kfun.func,
			kfun.nargs);
		break;
	    }
	}
	break;

    case KFUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_kfunc_spread_int(f, %d, %d)\n",
		    tmpRef(sp), kfun.func, kfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_kfunc_spread_float(f, %d, %d)\n",
		    tmpRef(sp), kfun.func, kfun.nargs);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_kfunc_spread(f, %d, %d)\n", kfun.func,
		    kfun.nargs);
	    break;
	}
	break;

    case KFUNC_SPREAD_LVAL:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr,
		    "\t%s <int> = lpc_vm_kfunc_spread_lval_int(f, %d, %d, %d)\n",
		    tmpRef(sp), kfun.func, kfun.nargs, kfun.spread);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_kfunc_spread_lval_float(f, %d, %d, %d)\n",
		    tmpRef(sp), kfun.func, kfun.nargs, kfun.spread);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_kfunc_spread_lval(f, %d, %d, %d)\n",
		    kfun.func, kfun.nargs, kfun.spread);
	    break;
	}
	break;

    case DFUNC:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_dfunc_int(f, %d, %d, %d)\n",
		    tmpRef(sp), dfun.inherit, dfun.func, dfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_dfunc_float(f, %d, %d, %d)\n",
		    tmpRef(sp), dfun.inherit, dfun.func, dfun.nargs);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_dfunc(f, %d, %d, %d)\n", dfun.inherit,
		    dfun.func, dfun.nargs);
	    break;
	}
	break;

    case DFUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_dfunc_spread_int(f, %d, %d, %d)\n",
		    tmpRef(sp), dfun.inherit, dfun.func, dfun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_dfunc_spread_float(f, %d, %d, %d)\n",
		    tmpRef(sp), dfun.inherit, dfun.func, dfun.nargs);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_dfunc_spread(f, %d, %d, %d)\n",
		    dfun.inherit, dfun.func, dfun.nargs);
	    break;
	}
	break;

    case FUNC:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_func_int(f, %d, %d)\n",
		    tmpRef(sp), fun.call, fun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_func_float(f, %d, %d)\n",
		    tmpRef(sp), fun.call, fun.nargs);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_func(f, %d, %d)\n", fun.call, fun.nargs);
	    break;
	}
	break;

    case FUNC_SPREAD:
	switch (context->get(sp).type) {
	case LPC_TYPE_INT:
	    fprintf(stderr, "\t%s <int> = lpc_vm_func_spread_int(f, %d, %d)\n",
		    tmpRef(sp), fun.call, fun.nargs);
	    result(context);
	    return;

	case LPC_TYPE_FLOAT:
	    fprintf(stderr,
		    "\t%s <float> = lpc_vm_func_spread_float(f, %d, %d)\n",
		    tmpRef(sp), fun.call, fun.nargs);
	    result(context);
	    return;

	default:
	    fprintf(stderr, "\tlpc_vm_func_spread(f, %d, %d)\n", fun.call,
		    fun.nargs);
	    break;
	}
	break;

    case CATCH:
	if (pop) {
	    fprintf(stderr, "\tcatch_pop(f) %%L%04x, %%L%04x\n", context->next,
		    target);
	} else {
	    fprintf(stderr, "\tcatch(f) %%L%04x, %%L%04x\n", context->next,
		    target);
	}
	context->sp = sp;
	return;

    case END_CATCH:
	fprintf(stderr, "\tcatch_end\n");
	break;

    case RLIMITS:
	fprintf(stderr, "\trlimits(f, %s, %s)\n",
		tmpRef(context->nextSp()), tmpRef(context->sp));
	break;

    case RLIMITS_CHECK:
	fprintf(stderr, "\trlimits_check(f, %s, %s)\n",
		tmpRef(context->nextSp()), tmpRef(context->sp));
	break;

    case END_RLIMITS:
	fprintf(stderr, "\trlimits_end\n");
	break;

    case RETURN:
	fprintf(stderr, "\treturn\n");
	break;

    default:
	break;
    }

    if (pop) {
	fprintf(stderr, "\tlpc_vm_pop(f)\n");
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
    FlowContext context(function, size);
    ClangBlock *b;
    LPCParam nParams, n;
    bool initParam;
    CodeSize i;
    Type type;
    int ref;
    Code *code;

    FlowBlock::evaluate(&context);

    initParam = false;
    nParams = function->nargs + function->vargs;
    for (n = 1; n <= nParams; n++) {
	switch (function->proto[n].type) {
	case LPC_TYPE_INT:
	    if (!initParam) {
		fprintf(stderr, "%%Lparam:\n");
	    }
	    fprintf(stderr, "\t%s <int> = lpc_vm_param_int(f, %d)\n",
		    ClangCode::paramRef(nParams - n, 0), nParams - n);
	    break;

	case LPC_TYPE_FLOAT:
	    if (!initParam) {
		fprintf(stderr, "%%Lparam:\n");
	    }
	    fprintf(stderr, "\t%s <float> = lpc_vm_param_float(f, %d)\n",
		    ClangCode::paramRef(nParams - n, 0), nParams - n);
	    break;

	default:
	    continue;
	}
	initParam = true;
    }
    if (initParam) {
	fprintf(stderr, "\tbranch %%L0000\n");
    }

    for (b = this; b != NULL; b = (ClangBlock *) b->next) {
	if (b->nFrom == 0 && b != this) {
	    continue;
	}
	fprintf(stderr, "%%L%04x:\n", b->first->addr);
	b->prepareFlow(&context);

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
			fprintf(stderr, "\t%s <int> = phi",
				ClangCode::paramRef(n, context.inParams[n]));
		    } else if (type == LPC_TYPE_FLOAT) {
			fprintf(stderr, "\t%s <float> = phi",
				ClangCode::paramRef(n, context.inParams[n]));
		    } else {
			continue;
		    }
		    if (b == this) {
			fprintf(stderr, ", [ %s, %%Lparam ]",
				ClangCode::paramRef(n, 0));
		    }
		    for (i = 0; i < b->nFrom; i++) {
			ref = b->from[i]->paramRef(n);
			if (ref != context.inParams[n]) {
			    fprintf(stderr, ", [ %s, %%L%04x ]",
				    ClangCode::paramRef(n, ref),
				    b->from[i]->first->addr);
			}
		    }
		    fprintf(stderr, "\n");
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
			fprintf(stderr, "\t%s <int> = phi",
				ClangCode::localRef(n, context.inLocals[n]));
		    } else if (type == LPC_TYPE_FLOAT) {
			fprintf(stderr, "\t%s <float> = phi",
				ClangCode::localRef(n, context.inLocals[n]));
		    } else {
			continue;
		    }
		    for (i = 0; i < b->nFrom; i++) {
			ref = b->from[i]->localRef(n);
			if (ref != context.inLocals[n]) {
			    fprintf(stderr, ", [ %s, %%L%04x ]",
				    ClangCode::localRef(n, ref),
				    b->from[i]->first->addr);
			}
		    }
		    fprintf(stderr, "\n");
		}
	    }

	    /*
	     * XXX stack
	     */
	}

	context.level = level;
	for (code = b->first; ; code = code->next) {
	    code->emit(&context);
	    if (code == b->last) {
		switch (code->instruction) {
		case Code::JUMP:
		    fprintf(stderr, "\tbranch %%L%04x\n", code->target);
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
		    fprintf(stderr, "\tbranch %%L%04x\n", context.next);
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
