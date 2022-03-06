#
# Makefile for compiler project files. For use under Linux, Cygwin or
# Mac OS/X (i.e., Unix-style environments).
#
# Targets:
#
#       make libcomp.a		generate the library libcomp.a
#	make smallparser	generate small example parser
#	make parser1		generate parser1 from parser1.c
#	make parser2		generate parser2 from parser2.c
#	make comp1		generate comp1 from comp1.c
#	make comp2		generate comp2 from comp2.c
#       make sim                build the machine code simulator program
#	make clean		delete all object files (but NOT the library
#				file) created by this Makefile
#	make veryclean		delete all object and library files created
#				by this Makefile
#
# The versions of C compiler and archiver being used are set by the macros
# CC and AR. The version of MAKE being used is set by MAKE
#
# Name of your local gcc (or other ANSI C compiler).
CC=gcc
# Flags to pass to compiler.
CFLAGS=-ansi -pedantic -Wall -Iheaders

# Name of your local make utility. Needed for recursive invocation of
# make on the "simulator" directory
MAKE=make

# Name of your local delete program. Usually "/bin/rm -rf".
RM=/bin/rm -rf

# Name of library of object files.
CODELIB=libcomp.a

# Build rules follow.

$(CODELIB) :
	$(MAKE) -C libsrc $(CODELIB)
	mv libsrc/$(CODELIB) .
	$(MAKE) -C libsrc veryclean

smallparser: smallparser.o $(CODELIB)
	$(CC) -o $@ smallparser.o $(CODELIB)

parser1: parser1.o $(CODELIB)
	$(CC) -o $@ parser1.o $(CODELIB)

parser2: parser2.o $(CODELIB)
	$(CC) -o $@ parser2.o $(CODELIB)

comp1: comp1.o $(CODELIB)
	$(CC) -o $@ comp1.o $(CODELIB)

comp2: comp2.o $(CODELIB)
	$(CC) -o $@ comp2.o $(CODELIB)

sim  : 
	$(MAKE) -C simulator sim
	mv simulator/sim .
	$(MAKE) -C simulator veryclean

clean:
	$(RM) *.o

veryclean:
	$(RM) $(CODELIB) *.o parser1 parser2 comp1 comp2 smallparser simparse simpars2
