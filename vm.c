#include <stdio.h>
#include "lilscheme.h"

Handle currentContext;
Handle globals;

Handle Interpret(Handle);

Handle CreateContext(Handle fn, Handle prior) {
    int nLocals, stacksize;

    // use scopes to destroy GC-invalidated pointers
    {
	FUNCTION *f = DATA_AREA(FUNCTION, fn);
	nLocals = f->nLocals;
	stacksize = f->stacksize;
    }

    Handle locals = CreateVector(nLocals);
    Retain(locals);
    Handle stack = CreateVector(stacksize);
    Retain(stack);

    Handle context = CreateObject(TYPE_CONTEXT, 0);
    
    CONTEXT *cxt = DATA_AREA(CONTEXT, context);
    cxt->function = fn;
    cxt->locals = locals;
    cxt->stack = stack;
    cxt->prior = prior;
    cxt->ip = 0;
    cxt->sp = 0;

    Unretain(stack);
    Unretain(locals);
    return context;
}

Handle ClosureGet(Handle fn, int idx) {
    Handle enclosingContext = DATA_AREA(FUNCTION, fn)->closure;
    while (enclosingContext != nil) {
	Handle enclosingLocals = DATA_AREA(CONTEXT, enclosingContext)->locals;
	int len = VectorLength(enclosingLocals);
	if (idx < len) {
	    return VectorRef(enclosingLocals, idx);
	}
	else {
	    idx -= len;
	    enclosingContext = DATA_AREA(CONTEXT, enclosingContext)->prior;
	}
    }
    panic("no such closure variable");
    return nil;
}

void ClosureSet(Handle fn, int idx, Handle val) {
    Handle enclosingContext = DATA_AREA(FUNCTION, fn)->closure;
    while (enclosingContext != nil) {
	Handle enclosingLocals = DATA_AREA(CONTEXT, enclosingContext)->locals;
	int len = VectorLength(enclosingLocals);
	if (idx < len) {
	    VectorSet(enclosingLocals, idx, val);
	}
	else {
	    idx -= len;
	    enclosingContext = DATA_AREA(CONTEXT, enclosingContext)->prior;
	}
    }
    panic("no such closure variable");
    return;
}

void LoadArgumentsFromList(Handle locals, Handle argList) {
    int i = 0;
    while (argList != nil) {
	VectorSet(locals, i, Car(argList));
	argList = Cdr(argList);
	i++;
    }
}

Handle StartInterpreter(Handle fn, Handle arglist) {
    Handle context = CreateContext(fn, nil);
    LoadArgumentsFromList(DATA_AREA(CONTEXT, context)->locals, arglist);
    return Interpret(context);
}


/* WELCOME TO DIE */
#define POP() (VectorRef(stack, --sp))
#define PUSH(_H) (VectorSet(stack, sp, _H),sp++)
#define TOS() (VectorRef(stack, sp-1))

int LoadArgumentsFromStack(Handle stack, int sp, Handle locals, int count) {
    for (int i = 0; i < count; i++) {
	VectorSet(locals, i, POP());
    }
    return sp;
}


Handle Interpret(Handle context) {
    Handle function, literals, locals, stack;
    Handle priorContext;
    Handle bytecode;
    int ip, sp;

    uint8_t op; int arg;
    Handle returnValue = nil;

    // unpack the context
 init:
    currentContext = context;
    if (context == nil) goto terminate;
    else
    {
	CONTEXT *cxt = DATA_AREA(CONTEXT, context);
	function = cxt->function;
	FUNCTION *fn = DATA_AREA(FUNCTION, function);
	literals = fn->literals;
	locals = cxt->locals;
	stack = cxt->stack;
	bytecode = fn->bytecode;
	ip = cxt->ip;
	sp = cxt->sp;
	priorContext = cxt->prior;
    }

    // fetch the current instruction
    
 fetch:
    op = BytevectorRef(bytecode, ip);
    ip++;
    if (op > OPCODE_ARGUMENTS) {
	arg = BytevectorRef(bytecode, ip);
	ip++;
    }

    
    // now dispatch
    switch (op) {
    case OP_END:
	// we put this here to catch off-by-one errors;
	panic("end of code; did not return");
	break;
    case OP_NOP:
	// nop
	break;
    case OP_DROP:
	// remove top-of-stack
	sp--;
	break;
    case OP_DUP:
	PUSH(TOS());
	break;
    case OP_NIL:
	PUSH(nil);
	break;
    case OP_BIND_CLOSURE:
	{
	    Handle fn = TOS();
	    Typecheck(fn, TYPE_FUNCTION);
	    FUNCTION *contents = DATA_AREA(FUNCTION, fn);
	    contents->closure = currentContext;
	}
	break;
    case OP_RETURN:
	// return top-of-stack to the prior context
	// if there is no context, leave the interpreter
	returnValue = POP();
	context = currentContext = priorContext;
	if (context == nil) goto terminate;
	else
	{
	    CONTEXT *cxt = DATA_AREA(CONTEXT, context);
	    sp = cxt->sp;
	    stack = cxt->stack;
	    PUSH(returnValue);
	    cxt->sp = sp;
	    goto init;
	}
	break;

    case OP_LITERAL:
	PUSH(VectorRef(literals, arg));
	break;
    case OP_GLOBAL:
	{
	    Handle result = AlistGet(globals, VectorRef(literals, arg));
	    if (result == nil) panic("undefined global");
	    PUSH(Cdr(result));
	    break;
	}
    case OP_SET_GLOBAL:
	globals = AlistSet(globals, VectorRef(literals, arg), POP());
	break;
    case OP_LOCAL:
	PUSH(VectorRef(locals, arg));
	break;
    case OP_SET_LOCAL:
	VectorSet(locals, arg, POP());
	break;
    case OP_CLOSURE:
	PUSH(ClosureGet(function, arg));
	break;
    case OP_SET_CLOSURE:
	ClosureSet(function, arg, POP());
	break;
    case OP_APPLY:
	{
	    Handle proc = POP();
	    switch(TYPEOF(proc)) {
	    case TYPE_FUNCTION:
		{
		    Handle newContext = CreateContext(proc, currentContext);
		    CONTEXT *cxt = DATA_AREA(CONTEXT, newContext);
		    sp = LoadArgumentsFromStack(stack, sp, cxt->locals, arg);
		    cxt = DATA_AREA(CONTEXT, currentContext);
		    cxt->sp = sp;
		    cxt->ip = ip;
		    context = newContext;
		    goto init;
		}
		break;
	    case TYPE_PRIMITIVE:
		{
		    Handle argv = CreateVector(arg);
		    sp = LoadArgumentsFromStack(stack, sp, argv, arg);
		    Handle result = CallPrimitive(proc, argv);
		    PUSH(result);
		}
		break;
	    default:
		panic("attempted to call a non-procedure");
	    }
	}
	break;
    case OP_TAIL_APPLY:
	panic("tail-apply not implented");
	break;
    case OP_JUMP_TRUE:
	if (POP() != nil) ip += arg;
	break;
    case OP_JUMP_FALSE:
	if (POP() == nil) ip += arg;
	break;
    case OP_JUMP:
	ip += arg;
	break;
    case OP_JUMP_BACK:
	ip -= arg;
	break;
	
    case OP_INVALID:
	panic("runaway fall-through in VM dispatch");
    default:
	panic("invalid opcode");
    }
    assert(sp >= 0);
    goto fetch;
    
 terminate:
    return returnValue;
}
