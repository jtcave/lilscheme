# lilscheme - a limited Scheme interpreter

This is an early attempt at a Scheme interpreter. I've had this lying around on a flash drive
for a really long time after getting distracted by life, so very little of this is done.
However, I think what I've done is nice. I'm especially proud of the garbage collector code
(see mm.c).

Basically the only data types implemented are integers, symbols, cons cells (lists), and
functions. There is no tail call optimization, which limits the number of runnable Scheme
programs even further.

## Build

To build this:

    make

This will build several test executables as well as the main executable, named `repl`.

I've tested the build on NetBSD and Ubuntu 20.04, but it should work on most systems. As
configured, this uses AddressSanitizer. If you don't have the runtime library, you'll have
to remove the flags from the Makefile.

## Run

To run the REPL:

    ./repl

There are no line editing facilities, nor is there any real way to run scripts. You can run
the included demo by redirecting stdin, but it'll look messy:

    $ ./repl < factorial.scm
    lilscheme repl
    
    ok
    factorial
    ok
    120
    
    ok
    720
        
    ok $

## License

MIT license.

Really, this code isn't even worth licensing in its current state, and I have no plans to
work on it. But I like to license things under MIT license anyway.
