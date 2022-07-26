#include <stdio.h>
#include "lilscheme.h"

int main(int argc, char **argv) {
    puts("reader test");    
    InitMem();
    DisplayObject(CreateSymbol("ready"), stdout);
    puts("");

    while(!(feof(stdin) || ferror(stdin))) {
	DisplayObject(ReadObject(stdin), stdout);
	puts("");
    }

    return 0;
}
