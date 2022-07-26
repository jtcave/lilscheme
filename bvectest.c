#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lilscheme.h"

void dump_bvec(Handle bv) {
    FOR_IN_VECTOR(i, bv) {
	printf("%02hhx ", BVEC_CONTENTS(bv)[i]);
    }
    putchar('\n');
}

int main() {
    Handle bvec;
    InitMem();
    bvec = CreateBytevector(strlen("Hello")+1);
    Retain(bvec);
    CreateSymbol("slug");
    
    

    strcpy((char*)(BVEC_CONTENTS(bvec)), "Hello");
    BytevectorAppend(bvec, 0xf0);
    BytevectorAppend(bvec, 0x0d);

    InspectAllObjects();
    puts("\n\tCollecting garbage.\n");
    GarbageCollect();
    InspectAllObjects();

    dump_bvec(bvec);

    return 0;
}
