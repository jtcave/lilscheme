#include <string.h>
#include "lilscheme.h"


// TODO: what is the type checking policy? Should primitives protect the user,
//       or should these routines protect _me_?


// Symbols
// todo: reconsider the policy on case-sensitivity
Handle CreateSymbol(const char *name) {
    // Despite the name of this procedure, this will only create a new symbol
    // if one is already in the interned symbols list
    Handle sym = FindSymbolNamed(name);
    if (sym == nil) {
	size_t len = strlen(name);
	sym = CreateObject(TYPE_SYMBOL, len+1); // null termination
	Retain(sym);
	strcpy(DATA_AREA(char, sym), name);
	internedSymbols = CreateCons(sym, internedSymbols);
	Unretain(sym);
    }
    return sym;
}

Handle FindSymbolNamed(const char *desiredName) {
    Handle here = internedSymbols;
    while (here != nil) {
	Handle sym = Car(here);
	if (strcmp(NameOfSymbol(sym), desiredName) == 0) {
	    return sym;
	}
	here = Cdr(here);
    }
    return nil;
}

char *NameOfSymbol(Handle sym) {
    if (DEREF(sym)->type != TYPE_SYMBOL) {
	panic("can't NameOfSymbol that type");
    }
    return DATA_AREA(char, sym);
}

