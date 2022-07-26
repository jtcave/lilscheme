#include <stdio.h>
#include "lilscheme.h"

Handle CallPrimitive(Handle prim, Handle argv) {
    // TODO: check arity
    PRIMITIVE *pr = DATA_AREA(PRIMITIVE, prim);
    PRIMPTR proc = pr->procedure;
    return (*proc)(argv);
}

Handle CreatePrimitive(PRIMPTR proc, int arity) {
    Handle prim = CreateObject(TYPE_PRIMITIVE, 0);
    PRIMITIVE *pr = DATA_AREA(PRIMITIVE, prim);
    pr->procedure = proc;
    pr->arguments = arity;
    return prim;
}

Handle prim_PLUS(Handle argv) {
    // TODO: handle floats
    int sum = 0;
    FOR_IN_VECTOR(i, argv) {
	Handle x = VectorRef(argv, i);
	Typecheck(x, TYPE_INT);
	sum += UnboxInteger(x);
    }
    return CreateInteger(sum);
}

Handle prim_MINUS(Handle argv) {
    // TODO: handle floats
    Handle first = VectorRef(argv, 0);
    Typecheck(first, TYPE_INT);
    int a = UnboxInteger(first);
    int len = VectorLength(argv);
    for (int i = 1; i < len; i++) {
	Handle x = VectorRef(argv, i);
	Typecheck(x, TYPE_INT);
	a -= UnboxInteger(x);
    }
    return CreateInteger(a);
    
}

Handle prim_TIMES(Handle argv) {
    // TODO: handle floats
    int product = 1;
    FOR_IN_VECTOR(i, argv) {
	Handle x = VectorRef(argv, i);
	Typecheck(x, TYPE_INT);
	product *= UnboxInteger(x);
    }
    return CreateInteger(product);
}

Handle prim_EQUAL(Handle argv) {
    Handle first = VectorRef(argv, 0);
    TypecheckNumeric(first);
    int len = VectorLength(argv);
    for (int i = 1; i < len; i++) {
	Handle x = VectorRef(argv, i);
	TypecheckNumeric(x);
	if (CompareNumbers(first, x) != 0) {
	    return LISP_FALSE;
	}
    }
    return LISP_TRUE;
    
}

Handle prim_cons(Handle argv) {
    Handle a = VectorRef(argv, 0);
    Handle b = VectorRef(argv, 1);
    return CreateCons(a, b);
}

Handle prim_car(Handle argv) {
    Handle pair = VectorRef(argv, 0);
    return Car(pair);
}

Handle prim_cdr(Handle argv) {
    Handle pair = VectorRef(argv, 0);
    return Cdr(pair);
}

Handle prim_set_car(Handle argv) {
    Handle pair = VectorRef(argv, 0);
    Handle obj = VectorRef(argv, 1);
    SetCar(pair, obj);
    return pair;
}

Handle prim_set_cdr(Handle argv) {
    Handle pair = VectorRef(argv, 0);
    Handle obj = VectorRef(argv, 1);
    SetCdr(pair, obj);
    return pair;
}



// eq? compares object identity; it returns true iff "both" arguments are the
// same object, which is true iff "they" have the same handle.
Handle prim_eqp(Handle argv) {
    Handle first = VectorRef(argv, 0);
    Handle second = VectorRef(argv, 1);
    return LISP_BOOLEAN(first == second);
}


// eqv? compares the identity of compound objects like conses and vectors and the
// value of atomic objects. In practice, this means that eqv? is the same as eq?
// for non-numeric objects and is the same as = for numeric objects.
Handle prim_eqvp(Handle argv) {
    Handle first = VectorRef(argv, 0);
    Handle second = VectorRef(argv, 1);
    return LISP_BOOLEAN(Equivalent(first, second));
}

// TODO: ports
Handle prim_display(Handle argv) {
    Handle o = VectorRef(argv, 0);
    DisplayObject(o, stdout);
    putchar('\n');
    return nil;
}

Handle prim_type_of(Handle argv) {
    Handle o = VectorRef(argv, 0);
    switch (TYPEOF(o)) {
    case TYPE_NIL:
	return SYM(nil);
    case TYPE_INT:
	return SYM(integer);
    case TYPE_FLOAT:
	return SYM(float);
    case TYPE_CONS:
	return SYM(pair);
    case TYPE_VECTOR:
	return SYM(vector);
    case TYPE_BYTEVECTOR:
	return SYM(bytevector);
    case TYPE_FUNCTION:
	return SYM(procedure);
    case TYPE_PRIMITIVE:
	return SYM(primitive-procedure);
    case TYPE_CONTEXT:
	return SYM(continuation);
	
    default:
	panic("forgot a type in the type-of primitive");
	return SYM(UNKNOWN-TYPE);
    }
}

struct PrimTableEntry {
    char *name;
    PRIMPTR proc;
    int arity;
};

static struct PrimTableEntry primTable[] = {
    {"+", prim_PLUS,  -1},
    {"-", prim_MINUS, -1},
    {"*", prim_TIMES, -2},
    {"=", prim_EQUAL, -2},
    {"eq?", prim_eqp, 2},
    {"eqv?", prim_eqvp, 2},
    {"cons", prim_cons, 2},
    {"car", prim_car, 1},
    {"cdr", prim_car, 1},
    {"set-car!", prim_set_car, 2},
    {"set-cdr!", prim_set_cdr, 2},
    {"display", prim_display, 1},
    {"type-of", prim_type_of, 1},
    {NULL, NULL, 0}
};

void ConstructPrimitives() {
    int idx = 0;
    while (primTable[idx].name != NULL) {
	Handle key = CreateSymbol(primTable[idx].name);
	Handle prim = CreatePrimitive(primTable[idx].proc, primTable[idx].arity);
	globals = AlistSet(globals, key, prim);
	idx++;
    }
}
