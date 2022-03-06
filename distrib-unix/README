This directory contains those files which are made avaialble to the
students of the CE4717 module to aid them in writing the project compiler.


1) Sources and header files for the support routines needed to build the
   project compiler. These files will compile under any ANSI C system.

   code.c     -- source for the assembly language interface module.
   code.h     -- header file for this module.
   debug.c    -- source file containing some useful debugging routines
                 (these just provide translation of TOKEN codes to 
                  string equivalents, also for SETs of TOKEN codes).
   debug.h    -- header file for this module.
   global.h   -- header file containing global stuff, should be #included
                 in all source files.
   line.c     -- source for the character handler, also handles list-file
                 generation.
   line.h     -- header file for this module.
   scanner.c  -- an implementation of the scanner. 
   scanner.h  -- header file for this module.
   sets.c     -- source for sets module.
   sets.h     -- header file for this module.
   strtab.c   -- source for string table module.
   strtab.h   -- header file for this module.
   symbol.c   -- source for symbol table module.
   symbol.h   -- header file for this module.

   The headers (.h files) are in subdirectory "headers", the sources
   (.c files) in subdirectory "libsrc".


2) A Makefile to help in building the compiler. Note that the target
   "make libcomp.a" (without the quotes) will build a library module
   (called libcomp.a) which simplifies subsequent building of the 
   compiler project. This library only needs to be built once, thereafter 
   you just use it:

   (initial session):

   $ make libcomp.a

   (thereafter):

   $ gcc -o parser1 -Iheaders parser1.c libcomp.a

   or:

   $ make parser1

   This is better because it supplies the appropriate paths to the
   headers and builds the library if it is not already present.  It
   also uses the compiler flags (-ansi -pedantic -Wall) that I use
   when testing.  These enforce strong error-checking and conformance
   to ANSI-C standards, so it's a good idea to use them.  C, by 
   tradition, is fairly permissive in what it will compile; sometimes
   this is dangerous as the compiler will compile code that won't
   execute on all computers.  So, using "-ansi" forces adherence to
   ANSI standards, "-pedantic" tells the compiler to be "picky" about
   what language features it will accept, and "-Wall" tells it to
   treat all warnings as errors, and to refuse to generate output
   code for any source that compiles with warnings.

   See the Makefile for a discussion about the targets available.


3) A simulator for the stack machine target of the compiler.

   Make the target "sim" to build the simulator program. This builds
   "sim" from sources in the subdirectory "simulator" and moves it to
   the main directory. Again, this only has to be done once, when you
   first download the archive. You may delete the "simulator" 
   directory if you wish once "sim" is built.

   $ make sim

   Help on how to use the simulator is contained in the ASCII text
   file "simhelp.txt".

   
4) Some example source and input files for the compiler.

   "simparse.c", "simpars2.c" -- These are *very* simple examples
   of recursive-descent parsers which read single-character
   tokens from the input.  They are just here to show the workings
   of a recursive-descent parser in the simplest possible way.
   They should *not* be used as the basis of your parsers and
   compilers.  Instead, use the coding approach presented in
   "smallparser.c".

   "smallparser.c" -- This is a parser for a very simple "language" 
   which uses the character handler and scanner. To build the 
   executable "smallparser", use the command:

   $ make smallparser

   This will have the side effect of automatically building the 
   library "libcomp.a" if it is not already present.

   "parser1.c", "parser2.c", "comp1.c" and "comp2.c" -- These four
   files are just placeholders (no, they are not the project done
   for you).  They are just here to provide "make" targets 
   initially, and to show you the formatting and commenting
   conventions I expect in your code (note in particular the
   "header comment" that includes the names and ID numbers of
   group members).

   In the "test" subdirectory there are some test programs to get you
   started building your parsers and compilers.  It is *really* 
   important to test as you go.  These test files are just a 
   start, I test your projects on different test sources.

   "test.prog" ... "test6.prog" -- These are example programs from the
   compiler project language. Your parsers should parse all of these
   without any errors being reported. The test programs also exercise
   successively more complex features of the semantic analysis and code
   generation of your compiler.

   "test.errs" ... "test8.errs" -- These are example programs with various
   types of syntax or semantic error. Your compiler should be able to
   recognise and recover from these errors in appropriate ways.

EOF
