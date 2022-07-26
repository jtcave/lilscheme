/* mm.c - memory manager and garbage collector */

#include <stdlib.h>
#include <string.h>
#include <stdio.h> // for debugging
#include <assert.h>
#include "lilscheme.h"

#define HEAP_SIZE (1*1024*1024)
#define MAX_HANDLES 0xffff
#define FOR_EACH_HANDLE(_VAR_) for (int _VAR_ = 0; _VAR_ < MAX_HANDLES; _VAR_++)

// XXX: Lots of code here involves pointer arithmetic with void*.
// GCC thinks sizeof(void) == 1 for purposes of pointer arithmetic
// But that is an extension; MSVC complains about "unknown size"


OBJ **objectTable;

size_t SizeOfType(OBJTYPE);
int IsAtBottomOfHeap(Handle);
void MoveToBottomOfHeap(Handle);

void *heap;
void *freeMark;

Handle nil;
Handle internedSymbols;
Handle retainedObjects[MAX_HANDLES];
size_t retainedObjectsSize = 0;

void InitMem() {
    void *h = malloc(HEAP_SIZE);
    if (h == 0) {
	panic("could not create heap");
    }
    freeMark = heap = h;

    objectTable = malloc(MAX_HANDLES * sizeof(OBJ*));
    if (objectTable == 0){
	panic("could not create object table");
    }
    FOR_EACH_HANDLE(i) {
	objectTable[i] = 0;
    }

    nil = CreateObject(TYPE_NIL,0);
    assert(nil == 0);
    internedSymbols = nil;
    currentContext = nil;
    globals = nil;
}

void *HeapBottom() {
    return heap + HEAP_SIZE;
}

size_t AvailableSpace() {
    return HeapBottom() - freeMark;
}

int ValidHandle(Handle handle) {
    return handle < MAX_HANDLES;
}

// Instrumented dereference
OBJ *Dereference(Handle handle) {
    assert(handle < MAX_HANDLES);
    assert(ValidHandle(handle));
    OBJ *entry = objectTable[handle];
    assert(entry != NULL);
    return entry;
}



// TODO: alignment?

static int gcFrequency = 2;
static int allocsSinceCollection = 1;
void *AllocRawMem(size_t size) {
    void *newMark = freeMark + size;
    // TODO: we currently have this hardwired to GC every second alloc
    // we should have a setting to GC every Nth alloc
    if (newMark > HeapBottom() || allocsSinceCollection > gcFrequency) {
	GarbageCollect();
	allocsSinceCollection = 0;
	newMark = freeMark + size;
	if (newMark > HeapBottom()) {
	    panic("out of memory");
	}
    }
    allocsSinceCollection++;
    void *mem = freeMark;
    freeMark = newMark;
    return mem;
}

int ContainedInHeap(void *addr) {
    return (addr >= heap && addr <= HeapBottom());
}

void Retain(Handle hnd) {
    // do not double-retain
    for (size_t i = 0; i < retainedObjectsSize; i++) {
	if (retainedObjects[i] == hnd) return;
    }
    
    retainedObjects[retainedObjectsSize++] = hnd;
}

void Unretain(Handle hnd) {
    for (size_t i = 0; i < retainedObjectsSize; i++) {
	if (retainedObjects[i] == hnd) {
	    retainedObjectsSize--;
	    for (size_t j = i; j < retainedObjectsSize; j++) {
		retainedObjects[j] = retainedObjects[j+1];
	    }
	    return;
	}
    }
    panic("Unretain without Retain");
}

size_t MoveObjectFromHeap(Handle hnd, void *to) {
    if (!ValidHandle(hnd)) return 0;
    OBJ *from = objectTable[hnd];
    if (!ContainedInHeap(from)) return 0;
    size_t size = from->size;
    memcpy(to, from, size);
    objectTable[hnd] = to;
    return size;
}

int IsAtBottomOfHeap(Handle hnd) {
    size_t size = DEREF(hnd)->size;
    void *objBottom = (void*)(DEREF(hnd)) + size;
    if (objBottom > freeMark) panic("object extends into free space");
    return (objBottom == freeMark);
}

// is this necessary
void MoveToBottomOfHeap(Handle hnd) {
    size_t size = DEREF(hnd)->size;
    if (IsAtBottomOfHeap(hnd)) return; // already at the bottom
    else {
	void *to = AllocRawMem(size);
	MoveObjectFromHeap(hnd, to);
    }
    assert((void*)(DEREF(hnd)) + size == freeMark);
}

void ExtendObject(Handle hnd, size_t amount) {
    size_t newSize = DEREF(hnd)->size + amount;
    if (!IsAtBottomOfHeap(hnd) || AvailableSpace() < amount) {
	OBJ *to = AllocRawMem(newSize);
	MoveObjectFromHeap(hnd, to);
    }
    else {
	// We know there's enough memory, so don't let the GC move the object
	// out from under us.
	DisableGC();
	AllocRawMem(amount);
	EnableGC();
    }
    DEREF(hnd)->size = newSize;
}

static int gcEnabled = 1;

void EnableGC() {gcEnabled = 1;}
void DisableGC() {gcEnabled = 0;}


void GarbageCollect() {
    /* This is a stop-and-copy collector using Cheney's algorithm. */
    
    // It may be wise to relocate declarations to the top of the function

    if (!gcEnabled) return;
    
    void *newHeap = malloc(HEAP_SIZE);
    if (newHeap == 0) {
	panic("could not alloc memory for garbage collection");
    }
    void *newMark = newHeap;
    // first, move nil
    newMark += MoveObjectFromHeap(nil, newMark);
    
    // move retained objects
    for (size_t i = 0; i < retainedObjectsSize; i++) {
	Handle hnd = retainedObjects[i];
	newMark += MoveObjectFromHeap(hnd, newMark);
    }
    // move interned symbol list head
    newMark += MoveObjectFromHeap(internedSymbols, newMark);
    // move current context
    newMark += MoveObjectFromHeap(currentContext, newMark);
    // move globals
    newMark += MoveObjectFromHeap(globals, newMark);
    
    // move children of moved objects
    void *remaining = newHeap;
    while (remaining < newMark) {
	OBJ *obj = (OBJ *)remaining;
	switch(obj->type) {
	case TYPE_NIL: case TYPE_INT: case TYPE_FLOAT: case TYPE_SYMBOL:
	case TYPE_BYTEVECTOR: case TYPE_PRIMITIVE:
	    break;
	case TYPE_CONS: {
	    CONS *cons = (CONS *)(obj->data);
	    newMark += MoveObjectFromHeap(cons->car, newMark);
	    newMark += MoveObjectFromHeap(cons->cdr, newMark);
	    break;
	}
	case TYPE_VECTOR: {
	    VECTOR *vec = (VECTOR *)(obj->data);
	    for (int i = 0; i < vec->length; i++) {
		newMark += MoveObjectFromHeap(vec->elements[i], newMark);
	    }
	    break;
	}
	case TYPE_FUNCTION: {
	    FUNCTION *func = (FUNCTION *)(obj->data);
	    newMark += MoveObjectFromHeap(func->bytecode, newMark);
	    newMark += MoveObjectFromHeap(func->literals, newMark);
	    break;
	}
	case TYPE_CONTEXT: {
	    CONTEXT *ctx = (CONTEXT *)(obj->data);
	    newMark += MoveObjectFromHeap(ctx->function, newMark);
	    newMark += MoveObjectFromHeap(ctx->locals, newMark);
	    newMark += MoveObjectFromHeap(ctx->stack, newMark);
	    newMark += MoveObjectFromHeap(ctx->prior, newMark);
	    break;
	}
	default:
	    panic("there's a type I don't know how to collect");
	}
	remaining += obj->size;
    }
    // free handles that aren't used anymore
    FOR_EACH_HANDLE(i) {
	if (ContainedInHeap(objectTable[i])) {
	    objectTable[i] = NULL;
	}
    }
    // replace the heap
    void *oldHeap = heap;
    heap = newHeap;
    freeMark = newMark;
    free(oldHeap);
    allocsSinceCollection = 0;
}

Handle UnusedHandle() {
    // This could be optimized if we cache the last handle allocated.
    FOR_EACH_HANDLE(i) {
	if (objectTable[i] == NULL) return i;
    }
    panic("out of handles");
    return 0; // we don't ever return this way; this just suppresses warnings
}

Handle CreateObject(OBJTYPE type, size_t extra) {
    size_t size = SizeOfType(type) + extra;
    OBJ *optr = AllocRawMem(size);
    optr->type = type;
    optr->size = size;
    Handle hnd = UnusedHandle();
    objectTable[hnd] = optr;
    assert((void*)optr + size == freeMark);
    return hnd;
}

size_t SizeOfType(OBJTYPE type) {
    switch (type){
    case TYPE_NIL:
	return sizeof(OBJ);
    case TYPE_INT:
	return sizeof(OBJ) + sizeof(int);
    case TYPE_FLOAT:
	return sizeof(OBJ) + sizeof(double);
    case TYPE_CONS:
	return sizeof(OBJ) + sizeof(CONS);
    case TYPE_SYMBOL:
	return sizeof(OBJ); // there is no SYMBOL struct
    case TYPE_VECTOR:
	return sizeof(OBJ) + sizeof(VECTOR);
    case TYPE_BYTEVECTOR:
	return sizeof(OBJ) + sizeof(BYTEVECTOR);
    case TYPE_FUNCTION:
	return sizeof(OBJ) + sizeof(FUNCTION);
    case TYPE_CONTEXT:
	return sizeof(OBJ) + sizeof(CONTEXT);
    case TYPE_PRIMITIVE:
	return sizeof(OBJ) + sizeof(PRIMITIVE);
    default:
	panic("there's a type I don't know the size of");
	return sizeof(OBJ);
    }
}

// Debug routines


void InspectObject(Handle hnd) {
    OBJ *o = DEREF(hnd);
    printf("#%-5hd @%04x  %-16s size 0x%zx\t",
	   hnd, (unsigned int)((void*)o-heap), NameOfType(o->type), o->size);
    DumpObject(hnd, stdout);
    printf("\n");
}

void InspectAllObjects() {
    puts("=== Object Report Start ===");
    FOR_EACH_HANDLE(i) {
	if (objectTable[i] != 0) {
	    InspectObject(i);
	}
    }
    puts("=== Object Report End ===");
}
