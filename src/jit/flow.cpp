# include <stdlib.h>
# include <stdint.h>
# include <new>
# include <string.h>
extern "C" {
# include "lpc_ext.h"
}
# include "data.h"
# include "code.h"
# include "stack.h"
# include "block.h"
# include "typed.h"
# include "flow.h"
# include "jitcomp.h"


FlowContext::FlowContext(CodeFunction *func, StackSize size) :
    TypedContext(func, size)
{
    inParams = outParams = inLocals = outLocals = NULL;
}

FlowContext::~FlowContext()
{
}

/*
 * get a reference to a parameter
 */
int FlowContext::paramRef(LPCParam param)
{
    if (outParams[param] != 0) {
	return outParams[param];
    } else {
	inParams[param] = NEEDED;
	return 0;
    }
}

/*
 * get a reference to a local variable
 */
int FlowContext::localRef(LPCLocal local)
{
    if (outLocals[local] != 0) {
	return outLocals[local];
    } else {
	inLocals[local] = NEEDED;
	return 0;
    }
}


FlowCode::FlowCode(CodeFunction *function) :
    TypedCode(function)
{
    in = out = 0;
}

FlowCode::~FlowCode()
{
}

/*
 * determine inputs and outputs
 */
void FlowCode::evaluateFlow(FlowContext *context)
{
    LPCParam n;

    switch (instruction) {
    case PARAM:
	out = context->paramRef(param);
	break;

    case LOCAL:
	out = context->localRef(local);
	break;

    case STORES_PARAM:
    case STORES_PARAM_INDEX:
	in = context->paramRef(param);
	/* fall through */
    case STORE_PARAM:
    case STORE_PARAM_INDEX:
	context->outParams[param] = out = addr + 1;
	break;

    case STORES_LOCAL:
    case STORES_LOCAL_INDEX:
	in = context->localRef(local);
	/* fall through */
    case STORE_LOCAL:
    case STORE_LOCAL_INDEX:
	context->outLocals[local] = out = addr + 1;
	break;

    case CAUGHT:
	out = addr + 1;
	for (n = 0; n < context->nParams; n++) {
	    if (context->block->mod[n]) {
		context->outParams[n] = out;
	    }
	}
	for (n = 0; n < context->nLocals; n++) {
	    if (context->block->mod[context->nParams + n]) {
		context->outLocals[n] = out;
	    }
	}
	break;

    default:
	break;
    }
}


FlowBlock::FlowBlock(Code *first, Code *last, CodeSize size) :
    TypedBlock(first, last, size)
{
    inParams = inLocals = outParams = outLocals = NULL;
}

FlowBlock::~FlowBlock()
{
    delete[] inParams;
    delete[] inLocals;
    delete[] outParams;
    delete[] outLocals;
}

/*
 * return input param reference
 */
int FlowBlock::paramIn(LPCParam param)
{
    return inParams[param];
}

/*
 * return output param reference
 */
int FlowBlock::paramOut(LPCParam param)
{
    return (outParams[param] != 0) ? outParams[param] : inParams[param];
}

/*
 * was a parameter merged?
 */
bool FlowBlock::paramMerged(LPCParam param)
{
    return (paramIn(param) == -(first->addr + 1));
}

/*
 * return input local var reference
 */
int FlowBlock::localIn(LPCLocal local)
{
    return inLocals[local];
}

/*
 * return output local var reference
 */
int FlowBlock::localOut(LPCLocal local)
{
    return (outLocals[local] != 0) ? outLocals[local] : inLocals[local];
}

/*
 * was a local var merged?
 */
bool FlowBlock::localMerged(LPCLocal local)
{
    return (localIn(local) == -(first->addr + 1));
}

/*
 * parameter merged type
 */
Type FlowBlock::mergedParamType(LPCParam param, Type type)
{
    CodeSize i;

    if (!paramMerged(param)) {
	return LPC_TYPE_VOID;
    }

    i = 0;
    if (type == LPC_TYPE_VOID) {
	for (; i < nFrom; i++) {
	    if (from[i]->paramOut(param) != 0) {
		type = from[i]->paramType(param);
		break;
	    }
	}
    }
    for (; i < nFrom; i++) {
	if (from[i]->paramOut(param) != 0 && from[i]->paramType(param) != type)
	{
	    type = LPC_TYPE_MIXED;
	    break;
	}
    }
    return type;
}

/*
 * local variable merged type
 */
Type FlowBlock::mergedLocalType(LPCLocal local)
{
    Type type;
    CodeSize i;

    if (!localMerged(local)) {
	return LPC_TYPE_VOID;
    }

    type = LPC_TYPE_MIXED;
    for (i = 0; i < nFrom; i++) {
	if (from[i]->localOut(local) != 0) {
	    type = from[i]->localType(local);
	    break;
	}
    }
    for (; i < nFrom; i++) {
	if (from[i]->localOut(local) != 0 && from[i]->localType(local) != type)
	{
	    type = LPC_TYPE_MIXED;
	    break;
	}
    }
    return type;
}

/*
 * give Context access to the inputs and outputs of this block
 */
void FlowBlock::prepareFlow(FlowContext *context)
{
    context->inParams = inParams;
    context->inLocals = inLocals;
    context->outParams = outParams;
    context->outLocals = outLocals;
}

/*
 * determine inputs and outputs for this block
 */
void FlowBlock::evaluateFlow(FlowContext *context, Block **list)
{
    Code *code;
    LPCParam n;

    /*
     * initialize params & locals
     */
    if (context->nParams != 0) {
	memset(inParams = new int[context->nParams], '\0',
	       context->nParams * sizeof(int));
	memset(outParams = new int[context->nParams], '\0',
	       context->nParams * sizeof(int));
    }
    if (context->nLocals != 0) {
	memset(inLocals = new int[context->nLocals], '\0',
	       context->nLocals * sizeof(int));
	memset(outLocals = new int[context->nLocals], '\0',
	       context->nLocals * sizeof(int));
    }

    prepareFlow(context);
    context->block = this;

    for (code = first; ; code = code->next) {
	code->evaluateFlow(context);
	if (code == last) {
	    break;
	}
    }

    for (n = 0; n < context->nParams; n++) {
	if (inParams[n] != 0) {
	    toVisit(list);
	    break;
	}
    }
    for (n = 0; n < context->nLocals; n++) {
	if (inLocals[n] != 0) {
	    toVisit(list);
	    break;
	}
    }
}

/*
 * flow back inputs
 */
void FlowBlock::evaluateInputs(FlowContext *context, Block **list)
{
    LPCParam n;

    for (n = 0; n < context->nParams; n++) {
	if (context->inParams[n] != 0) {
	    if (inParams[n] == 0 && outParams[n] == 0) {
		inParams[n] = FlowContext::NEEDED;
		toVisit(list);
	    }
	}
    }
    for (n = 0; n < context->nLocals; n++) {
	if (context->inLocals[n] != 0) {
	    if (inLocals[n] == 0 && outLocals[n] == 0) {
		inLocals[n] = FlowContext::NEEDED;
		toVisit(list);
	    }
	}
    }
}

/*
 * flow forward outputs
 */
void FlowBlock::evaluateOutputs(FlowContext *context, Block **list)
{
    LPCParam n;

    for (n = 0; n < context->nParams; n++) {
	if (inParams[n] != 0 && inParams[n] != -(first->addr + 1)) {
	    if (context->outParams[n] != 0) {
		if (context->outParams[n] != inParams[n]) {
		    if (inParams[n] == FlowContext::NEEDED ||
			nFrom + (first->addr == 0) <= 1) {
			inParams[n] = context->outParams[n];
		    } else {
			inParams[n] = -(first->addr + 1);
		    }
		    toVisit(list);
		}
	    } else if (context->inParams[n] != 0 &&
		       context->inParams[n] != FlowContext::NEEDED &&
		       context->inParams[n] != inParams[n]) {
		if (inParams[n] == FlowContext::NEEDED ||
		    nFrom + (first->addr == 0) <= 1) {
		    inParams[n] = context->inParams[n];
		} else {
		    inParams[n] = -(first->addr + 1);
		}
		toVisit(list);
	    }
	}
    }

    for (n = 0; n < context->nLocals; n++) {
	if (inLocals[n] != 0 && inLocals[n] != -(first->addr + 1)) {
	    if (context->outLocals[n] != 0) {
		if (context->outLocals[n] != inLocals[n]) {
		    if (inLocals[n] == FlowContext::NEEDED ||
			nFrom + (first->addr == 0) <= 1) {
			inLocals[n] = context->outLocals[n];
		    } else {
			inLocals[n] = -(first->addr + 1);
		    }
		    toVisit(list);
		}
	    } else if (context->inLocals[n] != 0 &&
		       context->inLocals[n] != FlowContext::NEEDED &&
		       context->inLocals[n] != inLocals[n]) {
		if (inLocals[n] == FlowContext::NEEDED ||
		    nFrom + (first->addr == 0) <= 1) {
		    inLocals[n] = context->inLocals[n];
		} else {
		    inLocals[n] = -(first->addr + 1);
		}
		toVisit(list);
	    }
	}
    }
}

/*
 * evaluate all blocks
 */
void FlowBlock::evaluate(FlowContext *context)
{
    Block *list, *b;
    CodeSize i;
    LPCParam n;

    TypedBlock::evaluate(context);

    /*
     * determine inputs and outputs for each block
     */
    startAllVisits(&list);
    for (b = this; b != NULL; b = b->next) {
	b->evaluateFlow(context, &list);
    }

    /*
     * flow back inputs
     */
    while ((b=nextVisit(&list)) != NULL) {
	b->prepareFlow(context);
	for (i = 0; i < b->nFrom; i++) {
	    b->from[i]->evaluateInputs(context, &list);
	}
    }

    /*
     * inputs for the first block are initial values
     */
    for (n = 0; n < context->nParams; n++) {
	if (inParams[n] != 0) {
	    inParams[n] = FlowContext::INITIAL;
	}
    }
    for (n = 0; n < context->nLocals; n++) {
	if (inLocals[n] != 0) {
	    inLocals[n] = FlowContext::INITIAL;
	}
    }

    /*
     * flow forward outputs
     */
    startAllVisits(&list);
    for (b = this; b != NULL; b = b->next) {
	b->prepareFlow(context);
	for (i = 0; i < b->nTo; i++) {
	    b->to[i]->evaluateOutputs(context, &list);
	}
    }
    while ((b=nextVisit(&list)) != NULL) {
	b->prepareFlow(context);
	for (i = 0; i < b->nTo; i++) {
	    b->to[i]->evaluateOutputs(context, &list);
	}
    }
}
