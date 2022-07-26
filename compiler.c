#include <assert.h>
#include <stdio.h>
#include "lilscheme.h"

/* MAJOR TODO: implement syntax checking for most of these special forms */

// compiler state
typedef struct state {
    int currentStack;
    int maxStack;
    int nLocals;
    int nArgs;
    Handle locals;
    Handle bytecode;
    Handle literals;
    struct state *prior;
} STATE;

void CompileForm(STATE*, Handle, COMPILER_MODE);
void CompileLiteral(STATE*, Handle, COMPILER_MODE);
void CompileVariable(STATE*, Handle, COMPILER_MODE); // incomplete

void CompileCompound(STATE*, Handle, COMPILER_MODE);
void CompileBody(STATE*, Handle, COMPILER_MODE);
void CompileBegin(STATE*, Handle, COMPILER_MODE);
void CompileQuote(STATE*, Handle, COMPILER_MODE);
void CompileApply(STATE*, Handle, COMPILER_MODE);
void CompileIf(STATE*, Handle, COMPILER_MODE);
void CompileLambda(STATE*, Handle, COMPILER_MODE);
void CompileDefine(STATE*, Handle, COMPILER_MODE);
// void CompileSet







Handle CreateFunction(STATE *state) {    
    Handle fn = CreateObject(TYPE_FUNCTION, 0);
    FUNCTION *data = DATA_AREA(FUNCTION,fn);
    data->stacksize = state->maxStack;
    data->nLocals = state->nLocals;
    data->arguments = state->nArgs;
    data->bytecode = state->bytecode;
    data->literals = state->literals;
    data->closure = nil;
    assert(data->nLocals >= data->arguments);
    assert(TYPEOF(data->literals) == TYPE_VECTOR);
    assert(TYPEOF(data->bytecode) == TYPE_BYTEVECTOR);
    return fn;
}

/* utility procedures */

void StackEffect(STATE *state, int delta) {
    state->currentStack += delta;
    if (state->currentStack > state->maxStack) {
	state->maxStack = state->currentStack;
    }
    assert(state->currentStack >= 0);
}


void AppendBytecode(STATE *state, uint8_t op) {
    BytevectorAppend(state->bytecode, op);
}
void AppendBytecodeWithArg(STATE *state, uint8_t op, int arg) {
    if (arg > 0xff) {
	panic("argument too large; implement `big-arg`");
    }
    BytevectorAppend(state->bytecode, op);
    BytevectorAppend(state->bytecode, (uint8_t)arg); // the damage happens here
}

int CurrentBytecodePosition(STATE *state) {
    return BytevectorLength(state->bytecode);
}

void ApplyFixup(STATE *state, int opPos, int targetPos) {
    // this will need to be modified for big-arg
    uint8_t *bytecode = BVEC_CONTENTS(state->bytecode);
    uint8_t op = bytecode[opPos];
    if (op != OP_JUMP_TRUE && op != OP_JUMP_FALSE && op != OP_JUMP) {
	// TODO: handle jump-back properly?
	panic("wrong instruction for fixups");
    }
    int argPos = opPos+1;
    int srcPos = opPos+2;
    int distance = targetPos - srcPos;
    if (distance < 0) {
	panic("fixup would go backwards");
    }
    if (distance > 0xff) {
	panic("jump distance too big; implement `big-arg`");
    }
    bytecode[argPos] = (uint8_t)distance;
}

void InitializeState(STATE *state) {
    state->currentStack = state->maxStack = 0;
    state->nLocals = 0;
    state->nArgs = 0;
    state->locals = nil;
    state->bytecode = CreateBytevector(0);
    Retain(state->bytecode);
    state->literals = CreateVector(0);
    Retain(state->literals);
    state->prior = NULL;
}

// sole entry point to the compiler
Handle Compile(Handle code, COMPILER_MODE mode) {
    STATE state;
    InitializeState(&state);

    CompileForm(&state, code, mode);
    assert (state.currentStack == 1);

    AppendBytecode(&state, OP_RETURN);
    AppendBytecode(&state, OP_END);
    StackEffect(&state, -1);
    Handle fn = CreateFunction(&state);
    Unretain(state.bytecode);
    Unretain(state.literals);
    // unretain `bytecode` and `literals`
    return fn;
}

void CompileForm(STATE *state, Handle code, COMPILER_MODE mode) {
    switch(TYPEOF(code)) {
    case TYPE_INT: case TYPE_FLOAT:
    case TYPE_VECTOR: case TYPE_BYTEVECTOR:
	CompileLiteral(state, code, mode);
	break;
    case TYPE_SYMBOL:
	CompileVariable(state, code, mode);
	break;
    case TYPE_CONS:
	CompileCompound(state, code, mode);
	break;
    default:
	panic("can't compile that type");
    }
}

void CompileLiteral(STATE *state, Handle code, COMPILER_MODE mode) {
    if (code != nil) {
	int litpos = AddToVector(state->literals, code);
	assert (litpos < 256); // todo: see if extended args are really needed
	AppendBytecodeWithArg(state, OP_LITERAL, litpos);
    }
    else {
	AppendBytecode(state, OP_NIL);
    }
    StackEffect(state, 1);
}

/* Variable references */

void CompileGlobalVariable(STATE *state, Handle code, COMPILER_MODE mode) {
    int varpos = AddToVector(state->literals, code);
    assert (varpos < 0xff);
    AppendBytecodeWithArg(state, OP_GLOBAL, varpos);
    StackEffect(state, 1);
}

int SearchForLocal(STATE *state, Handle var) {
    Handle locals = state->locals;
    int idx = 0;
    while (locals != nil) {
	if (Car(locals) == var) return idx;
	else {
	    locals = Cdr(locals);
	    idx++;
	}
    }
    // search failed
    return -1;
}

// TODO: abstract into list.c
int AddLocal(STATE *state, Handle var) {
    Handle locals = state->locals;
    if (locals == nil) {
	state->locals = CreateCons(var, nil);
	state->nLocals++;
	return 0;
    }
    else {
	Handle prev = locals;
	int idx = 0;
	while (locals != nil) {
	    if (Car(locals) == var) return idx; // already present
	    prev = locals;
	    locals = Cdr(locals);
	    idx++;
	}
	SetCdr(prev, CreateCons(var, nil));
	state->nLocals++;
	return idx+1;
    }
}

// for closure variables
int SearchForClosure(STATE *state, Handle var) {
    STATE *parentState = state->prior;
    int idx = 0;
    while (parentState != NULL) {
	Handle locals = parentState->locals;
	while (locals != nil) {
	    if (Car(locals) == var) return idx;
	    else {
		locals = Cdr(locals);
		idx++;
	    }
	}
	parentState = parentState->prior;
    }
    return -1;
}

void CompileVariable(STATE *state, Handle code, COMPILER_MODE mode) {
    if (mode != COMPILER_MODE_LAMBDA) {
	CompileGlobalVariable(state, code, mode);
    }
    else {
	// see if the variable is in scope
	int localIdx = SearchForLocal(state, code);
	if (localIdx != -1) {
	    assert(localIdx < 0xff);
	    AppendBytecodeWithArg(state, OP_LOCAL, localIdx);
	    StackEffect(state, 1);
	}
	else {
	    int closureIdx = SearchForClosure(state, code);
	    if (closureIdx != -1) {
		assert(closureIdx < 0xff);
		AppendBytecodeWithArg(state, OP_CLOSURE, closureIdx);
		StackEffect(state, 1);
	    }
	    else {
		CompileGlobalVariable(state, code, mode);
	    }
	}
    }
}

/* Compound expressions */

void CompileCompound(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle determinant = Car(code);
    if (determinant == CreateSymbol("quote")) {
	CompileQuote(state, code, mode);
    }
    else if (determinant == CreateSymbol("if")) {
	CompileIf(state, code, mode);
    }
    else if (determinant == CreateSymbol("begin")) {
	CompileBegin(state, code, mode);
    }
    else if (determinant == CreateSymbol("lambda")) {
	CompileLambda(state, code, mode);
    }
    else if (determinant == CreateSymbol("define")) {
	CompileDefine(state, code, mode);
    }
    else {
	CompileApply(state, code, mode);
    }
}

void CompileBody(STATE *state, Handle code, COMPILER_MODE mode) {
    while (code != nil) {
	CompileForm(state, Car(code), mode);
	Handle next = Cdr(code);
	if (next != nil) {
	    // drop return values of non-ultimate forms
	    AppendBytecode(state, OP_DROP);
	    StackEffect(state, -1);
	}
	code = next;
    }
}

/* (begin expr1 [expr2 ...]) */

void CompileBegin(STATE *state, Handle code, COMPILER_MODE mode) {
    CompileBody(state, Cdr(code), mode);
}

/* function appllication */

void CompileArguments(STATE *state, Handle code, COMPILER_MODE mode) {
    if (code == nil) return;
    CompileArguments(state, Cdr(code), mode);
    CompileForm(state, Car(code), mode);
}
void CompileApply(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle fn, arglist;
    int len;
    fn = Car(code);
    arglist = Cdr(code);
    len = ListLength(arglist);
    CompileArguments(state, arglist, mode);
    CompileForm(state, fn, mode);
    AppendBytecodeWithArg(state, OP_APPLY, len);
    StackEffect(state, -len);
    //printf("%d %d\n", len, currentStack);
}

/* 'foo or (quote foo) */

void CompileQuote(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle quoted = Car(Cdr(code));
    CompileLiteral(state, quoted, mode);
}

/* (if condition consequent [alternate]) */

void CompileIfThen(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle condition, consequent;

    // skip 'if
    code = Cdr(code);
    condition = Car(code);
    consequent = Car(Cdr(code));

    // compile condition
    CompileForm(state, condition, mode);

    // remember where we put the jump so we can fix it up later
    int addrOfJump = CurrentBytecodePosition(state);
    AppendBytecodeWithArg(state, OP_JUMP_FALSE, 0);
    StackEffect(state, -1);

    // compile consequent
    CompileForm(state, consequent, mode);

    // apply fixup to the jump we compiled earlier
    int jumpDest = CurrentBytecodePosition(state);
    ApplyFixup(state, addrOfJump, jumpDest);
}

void CompileIfThenElse(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle condition, consequent, alternative;

    // skip 'if
    code = Cdr(code);
    condition = Car(code); code = Cdr(code);
    consequent = Car(code); code = Cdr(code);
    alternative = Car(code);

    // compile condition
    CompileForm(state, condition, mode);

    // remember the jump to the alternative
    int jumpToAlternative = CurrentBytecodePosition(state);
    AppendBytecodeWithArg(state, OP_JUMP_FALSE, 0);
    StackEffect(state, -1);

    // compile consequent; remember jump to end
    CompileForm(state, consequent, mode);
    int jumpToEnd = CurrentBytecodePosition(state);
    AppendBytecodeWithArg(state, OP_JUMP, 0);

    // compile alternative
    int altPos = CurrentBytecodePosition(state);
    CompileForm(state, alternative, mode);

    // apply fixups
    int endPos = CurrentBytecodePosition(state);
    ApplyFixup(state, jumpToAlternative, altPos);
    ApplyFixup(state, jumpToEnd, endPos);

    // if you don't do this, then the stack effect will be one too high
    // that's because it counts both the consequent and the alternative
    StackEffect(state, -1);
}

void CompileIf(STATE *state, Handle code, COMPILER_MODE mode) {
    int len = ListLength(code);
    if (len < 3) {
	panic("too few arguments to if");
    }
    else if (len == 3) {
	CompileIfThen(state, code, mode);
    }
    else {
	CompileIfThenElse(state, code, mode);
    }
}

/* (lambda ([arg1 ...]) expr1 [expr2 ...]) */

void CompileFunction(STATE *state, Handle args, Handle body,
		     COMPILER_MODE mode) {
    STATE newState;
    InitializeState(&newState);
    newState.nLocals = newState.nArgs = ListLength(args);
    newState.locals = args;
    newState.prior = state;
    
    CompileBody(&newState, body, COMPILER_MODE_LAMBDA);
    assert(newState.currentStack == 1);
    AppendBytecode(&newState, OP_RETURN);
    AppendBytecode(&newState, OP_END);

    Handle fn = CreateFunction(&newState);
    Retain(fn);
    Unretain(newState.bytecode);
    Unretain(newState.literals);
    CompileLiteral(state, fn, mode);
    if (mode == COMPILER_MODE_LAMBDA) {
	AppendBytecode(state, OP_BIND_CLOSURE);
    }
    Unretain(fn);
}

void CompileLambda(STATE *state, Handle code, COMPILER_MODE mode) {
    // TODO: deduplicate from Compile()
    // TODO: dotted args
    Handle args, body;
    // skip 'lambda
    code = Cdr(code);
    
    args = Car(code);
    body = Cdr(code);

    CompileFunction(state, args, body, mode);
}

/* (define var value) or (define (var [arg1 ...]) body) */
void CompileVarDefine(STATE*, Handle, COMPILER_MODE);
void CompileFunctionDefine(STATE*, Handle, COMPILER_MODE);
void CompileBinding(STATE*, Handle, COMPILER_MODE);


void CompileDefine(STATE *state, Handle code, COMPILER_MODE mode) {
    Handle var = Cadr(code);
    if (TYPEOF(var) == TYPE_CONS) {
	// function define
	CompileFunctionDefine(state, code, mode);
    }
    else {
	CompileVarDefine(state, code, mode);
    }
}

void CompileVarDefine(STATE *state, Handle code, COMPILER_MODE mode) {
    // skip 'define
    code = Cdr(code);
    Handle var = Car(code);
    Handle value = Cadr(code);
    CompileForm(state, value, mode);
    CompileBinding(state, var, mode);
}

void CompileFunctionDefine(STATE *state, Handle code, COMPILER_MODE mode) {
    // skip 'define
    code = Cdr(code);
    Handle formals = Car(code);
    Handle var = Car(formals);
    Handle args = Cdr(formals);
    Handle body = Cdr(code);
    if (mode == COMPILER_MODE_LAMBDA) {
	AddLocal(state, var);
    }
    CompileFunction(state, args, body, mode);
    CompileBinding(state, var, mode);
}

// bind the top-of-stack to a variable
// create a local if it's being bound in a function
// else set a global
void CompileBinding(STATE *state, Handle var, COMPILER_MODE mode) {
    int idx;
    if (mode == COMPILER_MODE_LAMBDA) {
	idx = AddLocal(state, var);
	AppendBytecodeWithArg(state, OP_SET_LOCAL, idx);
    }
    else {
	// set global
	idx = AddToVector(state->literals, var);
	AppendBytecodeWithArg(state, OP_SET_GLOBAL, idx);
    }
    StackEffect(state, -1); // set-*! op consumes top-of-stack
    CompileLiteral(state, var, mode); // return symbol of variable we just set
}

/* Disassembler */

const char* OpcodeName(uint8_t);

void Disassemble(Handle fn) {
    FUNCTION *contents = DATA_AREA(FUNCTION, fn);
    printf("%d stack, %d vars\n", contents->stacksize, contents->nLocals);
    printf("literals: ");
    DisplayObject(contents->literals, stdout);
    printf("\nbytecode:\n");
    
    // bytecode disassembly
    uint8_t *code = BVEC_CONTENTS(contents->bytecode);
    FOR_IN_VECTOR(i, contents->bytecode) {
	uint8_t op = code[i];
	if (op < OPCODE_ARGUMENTS) {
	    printf("%02x\t%-14s\n", op, OpcodeName(op));
	}
	else {
	    uint8_t arg = code[++i];
	    printf("%02x %02x\t%-14s %3d", op, arg, OpcodeName(op), arg);

	    switch(op) {
	    case OP_LITERAL: case OP_GLOBAL: case OP_SET_GLOBAL: {
	        printf(" (");
		Handle lit = VectorRef(contents->literals, arg);
		DisplayObject(lit, stdout);
		putchar(')');
	    }
	    default: break;
	    }
	    putchar('\n');
	}
    }

    // disassemble lambdas
    FOR_IN_VECTOR(i, contents->literals) {
	Handle obj = VectorRef(contents->literals, i);
	if (TYPEOF(obj) == TYPE_FUNCTION) {
	    putchar('\n');
	    DisplayObject(obj, stdout);
	    putchar('\n');
	    Disassemble(obj);
	}
    }
    putchar('\n');
}

const char* OpcodeName(uint8_t op) {
    switch(op) {
    case OP_END: return "end";
	
    case OP_NOP: return "nop";
    case OP_DROP: return "drop";
    case OP_DUP: return "dup";
    case OP_RETURN: return "return";
    case OP_NIL: return "nil";
    case OP_BIND_CLOSURE: return "bind-closure!";

    case OP_LITERAL: return "literal";
    case OP_GLOBAL: return "global";
    case OP_SET_GLOBAL: return "set-global!";
    case OP_LOCAL: return "local";
    case OP_SET_LOCAL: return "set-local!";
    case OP_CLOSURE: return "closure";
    case OP_SET_CLOSURE: return "set-closure!";
    case OP_APPLY: return "apply";
    case OP_TAIL_APPLY: return "tail-apply";
    case OP_JUMP_TRUE: return "jump-true";
    case OP_JUMP_FALSE: return "jump-false";
    case OP_JUMP: return "jump";
	
    default: return "???";
    }
}
