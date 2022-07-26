#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "lilscheme.h"

void DisplayCons(Handle, FILE*);
void DisplayVector(Handle, FILE*);

void DisplayObject(Handle hnd, FILE *output) {
    switch(TYPEOF(hnd)) {
    case TYPE_NIL:
	fputs("nil", output);
	break;
    case TYPE_INT:
	fprintf(output, "%d", UnboxInteger(hnd));
	break;
    case TYPE_FLOAT:
	fprintf(output, "%f", UnboxFloat(hnd));
	break;
    case TYPE_SYMBOL:
	fprintf(output, "%s", NameOfSymbol(hnd));
	break;
    case TYPE_CONS:
	DisplayCons(hnd, output);
	break;
    case TYPE_VECTOR:
	DisplayVector(hnd, output);
	break;
    case TYPE_BYTEVECTOR:
	fprintf(output, "<bytevector #%hd>", hnd);
	break;
    case TYPE_FUNCTION:
	fprintf(output, "<function #%hd>", hnd);
	break;
    case TYPE_CONTEXT:
	fprintf(output, "<continuation #%hd>", hnd);
	break;
    case TYPE_PRIMITIVE:
	fprintf(output, "<primitive #%hd>", hnd);
	break;
    default:
	panic("can't DisplayObject that type");
    }
}

void DisplayCons(Handle hnd, FILE *output) {
    Handle car, cdr;
    car = Car(hnd);
    cdr = Cdr(hnd);
    fputc('(', output);
    DisplayObject(car, output);
    while (DEREF(cdr)->type == TYPE_CONS) {
	car = Car(cdr);
	cdr = Cdr(cdr);
	fputc(' ', output);
	DisplayObject(car, output);
    }
    if (cdr != nil) {
	fputs(" . ", output);
	DisplayObject(cdr, output);
    }
    fputc(')', output);
}

void DisplayVector(Handle hnd, FILE *output) {
    fputs("#(", output);
    for (int i = 0; i < VectorLength(hnd); i++) {
	if (i > 0) fputc(' ', output);
	Handle elt = VectorRef(hnd, i);
	DisplayObject(elt, output);
    }
    fputc(')', output);
}

void DumpObject(Handle hnd, FILE *output) {
    switch(TYPEOF(hnd)) {
    case TYPE_NIL:
	fputs("nil", output);
	break;
    case TYPE_INT:
	fprintf(output, "%d", UnboxInteger(hnd));
	break;
    case TYPE_FLOAT:
	fprintf(output, "%f", UnboxFloat(hnd));
	break;
    case TYPE_SYMBOL:
	fprintf(output, "'%s", NameOfSymbol(hnd));
	break;
    case TYPE_CONS:
	fprintf(output, "(#%hd . #%hd)", Car(hnd), Cdr(hnd));
	break;
    case TYPE_VECTOR: {
	fputs("#(", output);
	FOR_IN_VECTOR(i, hnd) {
	    if (i > 0) fputc(' ', output);
	    fprintf(output, "#%hd", VectorRef(hnd, i));
	}
	fputc(')', output);
	break;
    }
    case TYPE_BYTEVECTOR:
	fprintf(output, "{BVEC#%hd}", hnd);
	break;
    case TYPE_FUNCTION:
	fprintf(output, "{FUNC#%hd}", hnd);
	break;
    default:
	panic("can't DumpObject that type");
    }
}
