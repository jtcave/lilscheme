#include "lilscheme.h"

// Vectors

Handle CreateVector(int length) {
    size_t allocSize = length*sizeof(Handle);
    Handle v = CreateObject(TYPE_VECTOR, allocSize);
    DATA_AREA(VECTOR,v)->length = length;
    FOR_IN_VECTOR(i, v) {
	VectorSet(v, i, nil);
    }
    return v;
}

int VectorLength(Handle v) {
    return DATA_AREA(VECTOR,v)->length;
}

Handle VectorRef(Handle v, int idx) {
    VectorBoundsCheck(v, idx);
    return DATA_AREA(VECTOR,v)->elements[idx];
}

void VectorSet(Handle v, int idx, Handle value) {
    VectorBoundsCheck(v, idx);
    DATA_AREA(VECTOR,v)->elements[idx] = value;
}

void VectorBoundsCheck(Handle v, int idx) {
    if (idx >= VectorLength(v)) {
	panic("vector index out of bounds");
    }
}

Handle VectorFromList(Handle list) {
    int length = ListLength(list);
    Handle vec = CreateVector(length);
    FOR_IN_VECTOR(i, vec) {
	VectorSet(vec, i, Car(list));
	list = Cdr(list);
    }
    return vec;
}

void ResizeVector(Handle v, int newLength) {
    int oldLength = VectorLength(v);
    if (oldLength == newLength) return; // no need to do anything
    if (oldLength > newLength) {
	// truncate the vector in-place
	DATA_AREA(VECTOR, v)->length = newLength;
	RemoveVectorSlack(v);
    }
    else {
	// TODO: make use of slack to expand vectors in place
	size_t sizeDelta = (newLength - oldLength) * sizeof(Handle);
	ExtendObject(v, sizeDelta);
	DATA_AREA(VECTOR, v)->length = newLength;
    }
}

void RemoveVectorSlack(Handle v) {
    size_t eltSize = (TYPEOF(v) == TYPE_BYTEVECTOR) ?
	sizeof(uint8_t) : sizeof(Handle);
    size_t exactSize = sizeof(OBJ) + sizeof(VECTOR)
	+ VectorLength(v)*eltSize;
    DEREF(v)->size = exactSize;
}

void VectorAppend(Handle v, Handle x) {
    int size = VectorLength(v);
    ResizeVector(v, size+1);
    VectorSet(v, size, x);
}

int AddToVector(Handle v, Handle x) {
    // TODO: use equivalence not identity
    FOR_IN_VECTOR(i, v) {
	if (VectorRef(v, i) == x) return i;
    }
    // if it's not there, add it
    int idx = VectorLength(v);
    VectorAppend(v, x);
    return idx;
}


// Bytevectors

Handle CreateBytevector(int length) {
    Handle bv = CreateObject(TYPE_BYTEVECTOR, length);
    DATA_AREA(BYTEVECTOR,bv)->length = length;
    return bv;
}

int BytevectorLength(Handle bv) {
    return DATA_AREA(BYTEVECTOR,bv)->length;
}

uint8_t BytevectorRef(Handle bv, int idx) {
    return BVEC_CONTENTS(bv)[idx];
}

void RemoveBytevectorSlack(Handle bv) {
    RemoveVectorSlack(bv);
}

void ResizeBytevector(Handle bv, int newLength) {;
    int oldLength = BytevectorLength(bv);
    if (oldLength == newLength) return; // no need to do anything
    if (oldLength > newLength) {
	DATA_AREA(BYTEVECTOR, bv)->length = newLength;
	RemoveBytevectorSlack(bv);
    }
    else {
	// TODO: make use of slack to expand bytevectors in place
	size_t sizeDelta = (newLength - oldLength) * sizeof(uint8_t);
	ExtendObject(bv, sizeDelta);
	DATA_AREA(BYTEVECTOR, bv)->length = newLength;
    }
}

void BytevectorAppend(Handle bv, uint8_t x) {
    int size = BytevectorLength(bv);
    ResizeBytevector(bv, size+1);
    assert(BytevectorLength(bv) == size+1);
    BVEC_CONTENTS(bv)[size] = x;
}
