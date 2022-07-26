#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lilscheme.h"


int issymbolstart(int);
int issymbol(int);
int ReadNextNonSpace(FILE*);
Handle ReadNextObject(FILE*);
Handle ReadNumber(FILE*);
Handle ReadSymbol(FILE*);
Handle ReadList(FILE*);
Handle ReadQuoted(FILE*);
Handle ReadVector(FILE*);

// TODO: allow reading from a string, not just a file
//       options: fmemopen(3), a Scheme-like port abstraction, wacky HAL stuff


// TODO: handle EOFs properly
//       disable garbage collection while reading
Handle ReadObject(FILE *input) {
    DisableGC();
    Handle o = ReadNextObject(input);
    EnableGC();
    return o;
}


Handle ReadNextObject(FILE *input) {
    int c;
    c = ReadNextNonSpace(input);
    if (ferror(input)) panic("EOF or read error in reader");
    if (feof(input)) return nil;
    ungetc(c, input);

    if (isdigit(c)) return ReadNumber(input);
    if (issymbolstart(c)) return ReadSymbol(input);
    if (c == '(') return ReadList(input);
    if (c == '\'') return ReadQuoted(input);
    if (c == '#') return ReadVector(input);
    // TODO: quasiquote/unquote

    // control isn't supposed to fall through to here
    fprintf(stderr, "c = %d ('%c')\n", c, (char)c);
    panic("reader doesn't understand that");
    return nil;
}


// TODO: hex, scientific notation, floating point
Handle ReadNumber(FILE *input) {
    char buffer[32];
    int idx = 0;
    int c = 0;
    while (c=fgetc(input), isdigit(c)) {
	if (idx >= 31) break; // clumsy
	buffer[idx++] = (char)c;
    }
    ungetc(c, input);
    buffer[idx] = '\0';
    int result = atoi(buffer);
    return CreateInteger(result);
}

int matches(int c, const char *ds) {
    while (*ds != '\0') {
	if (c == *ds) return 1;
	ds++;
    }
    return 0;
}
    
int issymbolstart(int c) {
    const char* SYM_CHARS = "!$%&*+-./:<=>?@^_~";
    return isalpha(c) || matches(c, SYM_CHARS);
}

int issymbol(int c) {
    return issymbolstart(c) || isdigit(c);
}

Handle ReadSymbol(FILE *input) {
    char buffer[64];
    int idx = 0;
    int c = 0;
    while (c=fgetc(input), issymbol(c)) {
	if (idx >= 63) break; // clumsy termination
	buffer[idx++] = (char)c;
    }
    ungetc(c, input);
    buffer[idx] = '\0';
    return CreateSymbol(buffer);
}

Handle ReadList(FILE *input) {
    Handle head, tail, next;
    Handle item;
    int c;

    fgetc(input); //discard left parenthesis
    if ((c = fgetc(input)) == ')') {
	// let's say that the empty list is nil for now
	return nil;
    }
    else ungetc(c, input);

    item = ReadNextObject(input);
    head = CreateCons(item, nil);
    tail = head;
    
    while ((c = ReadNextNonSpace(input)) != ')') {
	if (feof(input)) panic("list not closed");
	ungetc(c, input);
	item = ReadNextObject(input);
	next = CreateCons(item, nil);
	SetCdr(tail, next);
	tail = next;
    }
    return head;
}

Handle ReadQuoted(FILE *input) {
    fgetc(input); // discard quote tick
    Handle o = ReadNextObject(input);
    // 'o -> (quote o)
    return CreateCons(CreateSymbol("quote"),
		      CreateCons(o,
				 nil));		 
}

int ReadNextNonSpace(FILE *input) {
    int c;
    do {
	c = fgetc(input);
    } while (isspace(c));
    return c;
}


// XXX: concise, but inefficient implementation
Handle ReadVector(FILE *input) {
    fgetc(input); // discard hash sign
    Handle intermed = ReadNextObject(input);
    if (DEREF(intermed)->type != TYPE_CONS){
	// not a list
	panic("reader doesn't understand that");
	return nil;
    }
    Handle v = VectorFromList(intermed);
    return v;
}
