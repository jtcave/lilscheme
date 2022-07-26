#include "lilscheme.h"

// Integers
// TODO: integrate small integers into handles to save space

Handle CreateInteger(int value) {
    Handle hnd = CreateObject(TYPE_INT,0);
    DATA_DEREF(int,hnd) = value;
    return hnd;
}

int UnboxInteger(Handle hnd) {
    if (DEREF(hnd)->type != TYPE_INT) {
	panic("can't UnboxInteger that type");
    }
    return DATA_DEREF(int,hnd);
}

// Floating-point

Handle CreateFloat(double value) {
    Handle hnd = CreateObject(TYPE_FLOAT,0);
    DATA_DEREF(double,hnd) = value;
    return hnd;
}

double UnboxFloat(Handle hnd) {
    if (DEREF(hnd)->type != TYPE_FLOAT) {
	panic("can't UnboxFloat that type");
    }
    return DATA_DEREF(double,hnd);
}

// Coercive unboxing
int CoerceToInt(Handle n) {
    OBJTYPE t = TYPEOF(n);
    if      (t == TYPE_INT) return UnboxInteger(n);
    else if (t == TYPE_FLOAT) return (int)UnboxFloat(n);
    else panic("can't CoerceToInteger that type");
}

double CoerceToDouble(Handle n) {
    OBJTYPE t = TYPEOF(n);
    if      (t == TYPE_FLOAT) return UnboxFloat(n);
    else if (t == TYPE_INT) return (double)UnboxInteger(n);
    else panic("can't CoerceToDouble that type");
}

// Numericity

int IsNumericType(OBJTYPE t) {
    return t == TYPE_INT || t == TYPE_FLOAT;
}

int IsNumeric(Handle o) {
    return IsNumericType(TYPEOF(o));
}

// Comparisons

// returns a positive number if a > b
//         a negative number if a < b
//         zero              if a = b
int CompareNumbers(Handle a, Handle b) {
    TypecheckNumeric(a);
    TypecheckNumeric(b);
    OBJTYPE ta = TYPEOF(a);
    OBJTYPE tb = TYPEOF(b);
    if (ta == TYPE_FLOAT || tb == TYPE_FLOAT) {
	// compare as floating point
	double da = CoerceToDouble(a);
	double db = CoerceToDouble(b);
	if (da > db) return 1;
	else if (da < db) return -1;
	else return 0;
    }
    else {
	int ia = UnboxInteger(a);
	int ib = UnboxInteger(b);
	if (ia > ib) return 1;
	else if (ia < ib) return -1;
	else return 0;	
    }
}
