#include <stdio.h>
#include "lilscheme.h"

int main(int argc, char **argv) {
    Handle code, fn;
    
    puts("lilscheme repl");    
    InitMem();
    ConstructPrimitives();

    while(!(feof(stdin) || ferror(stdin))) {
	putchar('\n');
	DisplayObject(CreateSymbol("ok"), stdout);
	putchar(' ');
	code = ReadObject(stdin);
	//putchar('\n');
	if (code != nil) {
	    Retain(code);
	    //DisplayObject(code, stdout);
	    putchar('\n');
	    fn = Compile(code, COMPILER_MODE_REPL);
	    Unretain(code);
	    Retain(fn);
	    //Disassemble(fn);
	    //puts("=== end of disasm ===");
	    
	    Handle result = StartInterpreter(fn, nil);
	    if (result != nil) DisplayObject(result, stdout);
	    //putchar('\n');
	    Unretain(fn);
	}
    }

    return 0;
}
