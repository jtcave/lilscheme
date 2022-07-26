#include <stdio.h>
#include <stdlib.h>
#include "lilscheme.h"

int main(int argc, char **argv) {
    Handle one,two,three,pair1,pair2;
    puts("Hello...");
    InitMem();
    puts("...world!");
    
    one = CreateInteger(1);
    two = CreateFloat(2);
    CreateFloat(3.3);
    pair1 = CreateCons(two, nil);
    pair2 = CreateCons(one, pair1);
    three = CreateSymbol("three");
    CreateSymbol("fish");
    CreateCons(one, three);
    DeclareRoot(pair1);

    InspectAllObjects();
    
    puts("\n\tCollecting garbage.\n");
    GarbageCollect();
    InspectAllObjects();
    return 0;
}
