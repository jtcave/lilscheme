#include <stdio.h>
#include "lilscheme.h"

int main(int argc, char **argv) {
    Handle code, fn;
    
    puts("compiler test");    
    InitMem();
    DisplayObject(CreateSymbol("ready"), stdout);
    puts("");

    while(!(feof(stdin) || ferror(stdin))) {
	code = ReadObject(stdin);
	putchar('\n');
	if (code != nil) {
	    Retain(code);
	    DisplayObject(code, stdout);
	    putchar('\n');
	    fn = Compile(code, COMPILER_MODE_REPL);
	    Disassemble(fn);
	    putchar('\n');
	    Unretain(code);
	}
    }

    return 0;
}
