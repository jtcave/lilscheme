/* lilscheme.h -- global system defines */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

/* memory manager */

// Type dispatches can be found in the MM, the display code, the compiler, some
// of the primitives, and `util.c`.

typedef enum LispTypes {
    TYPE_NIL,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CONS,
    TYPE_SYMBOL,
    TYPE_VECTOR,
    TYPE_BYTEVECTOR,
    TYPE_FUNCTION,
    TYPE_PRIMITIVE,
    TYPE_CONTEXT,
} OBJTYPE;

// other types:
// TYPE_STRING
// TYPE_BOOLEAN
// TYPE_SMALLINT (would be a magical type;
//                just let CreateInteger and UnboxInteger work with these)



typedef uint16_t Handle;
typedef struct LispObject {
    OBJTYPE type;
    size_t size; // includes the type and size fields
    void *data[];
} OBJ;

extern OBJ **objectTable;

OBJ *Dereference(Handle);

//#define DEREF(HND) (objectTable[(HND)])
#define DEREF(HND) Dereference((HND))
#define DATA_AREA(TYPE,hnd) ((TYPE*)&(DEREF((hnd))->data))
#define DATA_DEREF(TYPE,hnd) *DATA_AREA(TYPE,(hnd))
#define TYPEOF(hnd) DEREF((hnd))->type


extern void *heap;

// special objects
extern Handle nil;
extern Handle internedSymbols;


void InitMem();
size_t AvailableSpace();
void* AllocRawMem(size_t);
Handle CreateObject(OBJTYPE,size_t);
void GarbageCollect();
void ExtendObject(Handle, size_t);

// Regarding Retain: the system might throw an error in the middle of
// native-code object construction. Some way to unretain such objects after
// catching the error is needed. The best way might be to only retain objects
// during such construction, and unretain everything after a catch
void Retain(Handle);
void Unretain(Handle);
// void UnretainEverything();
void EnableGC();
void DisableGC();

void InspectAllObjects();

/* Types */

Handle CreateInteger(int);
int UnboxInteger(Handle);
Handle CreateFloat(double);
double UnboxFloat(Handle);
int IsNumericType(OBJTYPE);
int IsNumeric(Handle);
int CompareNumbers(Handle, Handle);


typedef struct LispCons {
    Handle car;
    Handle cdr;
} CONS;
Handle CreateCons(Handle, Handle);
Handle Car(Handle);
Handle Cdr(Handle);
Handle Cadr(Handle);
// possible add Caar, Cdar, Cddr
void SetCar(Handle, Handle);
void SetCdr(Handle, Handle);

int ListLength(Handle);
Handle AlistGet(Handle, Handle);
Handle AlistSet(Handle, Handle, Handle);

Handle CreateSymbol(const char*);
Handle FindSymbolNamed(const char*);
char *NameOfSymbol(Handle);
#define SYM(s) (CreateSymbol(#s))

#define LISP_FALSE nil
#define LISP_TRUE (CreateSymbol("t"))
#define LISP_BOOLEAN(p) ((p) ? LISP_TRUE : LISP_FALSE)

typedef struct LispVector {
    int length;
    Handle elements[];
} VECTOR;
Handle CreateVector(int);
int VectorLength(Handle);
Handle VectorRef(Handle, int);
void VectorSet(Handle, int, Handle);
void VectorBoundsCheck(Handle, int);
//Handle VectorFill(Handle, Handle);
Handle VectorFromList(Handle);
void ResizeVector(Handle, int);
void RemoveVectorSlack(Handle);
void VectorAppend(Handle, Handle);
int AddToVector(Handle, Handle);

typedef struct LispBytevector {
    int length;
    uint8_t contents[];
} BYTEVECTOR;
Handle CreateBytevector(int);
int BytevectorLength(Handle);
uint8_t BytevectorRef(Handle, int);
void ResizeBytevector(Handle, int);
void BytevectorAppend(Handle, uint8_t);

#define FOR_IN_VECTOR(_VAR_, VECHND) for (int _VAR_ = 0; _VAR_ < VectorLength(VECHND); _VAR_++)
#define BVEC_CONTENTS(BVECHND) DATA_AREA(BYTEVECTOR,BVECHND)->contents

typedef struct LispFunction {
    int stacksize;
    int nLocals;
    int arguments;
    Handle bytecode;
    Handle literals;
    Handle closure;
} FUNCTION;
//Handle CreateFunction(int,int,Handle,Handle);


/* I/O */
void DisplayObject(Handle, FILE*); // for machine-readable output
void DumpObject(Handle, FILE*);    // for debugging
Handle ReadObject(FILE*);

/* Compiler */
enum bytecodes {
    OP_END = 0,        // end of bytecode stream;
    OP_NOP,            // do nothing
    OP_DROP,           // pop and discard the top of the stack
    OP_DUP,            // copy the top of the stack
    OP_NIL,            // push nil
    OP_RETURN,         // leave this function
    OP_BIND_CLOSURE,   // set the closure of top-of-stack function to here

    OPCODE_BIG_ARG,    // indicates that this instruction's argument is two bytes
    OPCODE_ARGUMENTS,  // not an opcode, just a delimiter to mark what ops take
                       // an argument

    OP_LITERAL,        // push a literal value
    OP_GLOBAL,         // push the value of a global variable
    OP_SET_GLOBAL,     // alter a global
    OP_LOCAL,          // push the value of a local: argument name or define'd
    OP_SET_LOCAL,      // alter a local
    OP_CLOSURE,        // closure variable: in an outer function scope
    OP_SET_CLOSURE,    // alter a closure variable
    OP_APPLY,          // apply a function to arguments on the stack
    OP_TAIL_APPLY,     // apply a function, discarding the current call frame
    OP_JUMP_TRUE,      // jump forward N bytes if top-of-stack is true
    OP_JUMP_FALSE,     // jump forward N bytes if top-of-stack is false
    OP_JUMP,           // jump forward N bytes
    OP_JUMP_BACK,      // jump backwards N bytes

    OP_INVALID = 0xff  // all opcodes must be less than OP_INVALID
};

typedef enum LispCompilerModes {
    COMPILER_MODE_REPL,
    COMPILER_MODE_TOPLEVEL,
    COMPILER_MODE_LAMBDA
} COMPILER_MODE;

Handle Compile(Handle, COMPILER_MODE);
void Disassemble(Handle);

/* Virtual machine */

typedef struct LispContext {
    Handle function;
    Handle locals;
    Handle stack;
    Handle prior;
    int ip;
    int sp;
} CONTEXT;

extern Handle currentContext;
extern Handle globals;

Handle StartInterpreter(Handle, Handle);

/* Primitives */

typedef Handle (*PRIMPTR)(Handle);
typedef struct LispPrimitive {
    int arguments;
    PRIMPTR procedure;
} PRIMITIVE;

Handle CallPrimitive(Handle, Handle);
void ConstructPrimitives();

/* Utility -- sort these */
void panic(char*) __attribute__ ((noreturn));
const char *NameOfType(OBJTYPE);
void Typecheck(Handle, OBJTYPE);
void TypecheckNumeric(Handle);
int Equivalent(Handle, Handle);
//int RecursivelyEqual(Handle, Handle);
