#include <stdio.h>
#include <stdlib.h>
#include "lilscheme.h"

// this file should go away

void panic(char *mesg) {
    fprintf(stderr, "FATAL ERROR: %s\n", mesg);
    abort();
}


const char *NameOfType(OBJTYPE type) {
    switch (type){
    case TYPE_NIL:        return "TYPE_NIL";
    case TYPE_INT:        return "TYPE_INT";
    case TYPE_FLOAT:      return "TYPE_FLOAT";
    case TYPE_CONS:       return "TYPE_CONS";
    case TYPE_SYMBOL:     return "TYPE_SYMBOL";
    case TYPE_VECTOR:     return "TYPE_VECTOR";
    case TYPE_BYTEVECTOR: return "TYPE_BYTEVECTOR";
    case TYPE_FUNCTION:   return "TYPE_FUNCTION";
    case TYPE_CONTEXT:    return "TYPE_CONTEXT";
    case TYPE_PRIMITIVE:  return "TYPE_PRIMITIVE";
    default: return "???";
    }
}

void Typecheck(Handle o, OBJTYPE type) {
    if (TYPEOF(o) != type) {
	panic("incorrect type");
    }
}

void TypecheckNumeric(Handle o) {
    if (!IsNumeric(o)) {
	panic("non-numeric object");
    }
}

// equivalence is what eqv? test for
int Equivalent(Handle x, Handle y) {
    if (x == y) return 1;
    
    OBJTYPE type = TYPEOF(x);
    OBJTYPE secondType = TYPEOF(y);
    if (type != secondType) {
	// disjoint types are ok if they're both numeric
	if (IsNumericType(type) && IsNumericType(secondType)) {
	    return CompareNumbers(x, y) == 0;
	}
	else return 0;
    }
    // it can be assumed that first != second
    assert(type != TYPE_NIL); // that would mean there's more than one nil
    switch (type) {
    case TYPE_INT:
	return (UnboxInteger(x) == UnboxInteger(y));
    case TYPE_FLOAT:
	return (UnboxFloat(x) == UnboxFloat(y));
    default: return 0;
    }

}
