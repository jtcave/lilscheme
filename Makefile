# CC=gcc
CFLAGS=-std=c11 -g -Wall -fsanitize=address
LDLIBS=-lasan

OBJ = mm.o number.o symbol.o cons.o list.o vector.o display.o reader.o util.o compiler.o vm.o prim.o
TESTS = mmtest readertest bvectest compilertest

all: $(TESTS) repl

mmtest: mmtest.o $(OBJ)
readertest: readertest.o $(OBJ)
bvectest: bvectest.o $(OBJ)
compilertest: compilertest.o $(OBJ)
repl: repl.o $(OBJ)


*.o: toilet.h #force recompile if the header changes

clean:
	rm -f *.o $(TESTS) repl *~

.PHONY: clean all
