#include <stdio.h>
#include <stdlib.h>
#include "lilscheme.h"

int main(int argc, char **argv) {
    Handle one,two,three,four,pair,vec;
    puts("Hello...");
    InitMem();
    puts("...world!");

    DisableGC();
    one = CreateInteger(1);
    two = CreateFloat(2);
    CreateFloat(3.3);
    pair = CreateCons(two, nil);
    three = CreateSymbol("three");
    four = CreateSymbol("QUATRO");
    CreateSymbol("fish");
    CreateCons(one, three);
    vec = CreateVector(2);
    VectorSet(vec, 0, three);
    VectorSet(vec, 1, four);
    
    Retain(pair);
    Retain(vec);
    EnableGC();
    
    InspectAllObjects();
    
    puts("\n\tCollecting garbage.\n");
    GarbageCollect();
    InspectAllObjects();

    return 0;
}
