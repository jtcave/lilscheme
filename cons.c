#include "lilscheme.h"

// Conses

Handle CreateCons(Handle car, Handle cdr) {
    Handle hnd = CreateObject(TYPE_CONS,0);
    DATA_AREA(CONS,hnd)->car = car;
    DATA_AREA(CONS,hnd)->cdr = cdr;
    return hnd;
}

Handle Car(Handle cons) {
    if (DEREF(cons)->type != TYPE_CONS) {
	panic("can't Car that type");
    }
    return DATA_AREA(CONS,cons)->car;
}

Handle Cdr(Handle cons) {
    if (DEREF(cons)->type != TYPE_CONS) {
	panic("can't Cdr that type");
    }
    return DATA_AREA(CONS,cons)->cdr;
}

void SetCar(Handle cons, Handle car) {
    if (DEREF(cons)->type != TYPE_CONS) {
	panic("can't SetCar that type");
    }
    DATA_AREA(CONS,cons)->car = car;
}

void SetCdr(Handle cons, Handle cdr) {
    if (DEREF(cons)->type != TYPE_CONS) {
	panic("can't SetCdr that type");
    }
    DATA_AREA(CONS,cons)->cdr = cdr;
}

Handle Cadr(Handle cons) {
    return Car(Cdr(cons));
}
