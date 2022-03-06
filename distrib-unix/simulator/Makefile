#############################################################################
#
#     Makefile for the stack-machine simulator.
#
#     Targets:
#
#     make sim       - makes the simulator executable "sim".
#     make clean     - deletes all object files generated during the build.
#     make veryclean - deletes all object files and the executable.
#
#
#############################################################################
#
# Command names - CC should be an ANSI compiler such as gcc. RM is a delete
# command.
#
CC = gcc
RM = /bin/rm -f
#
# These are flags for the gcc compiler. Obviously, they will have to be
# changed if a different compiler is employed.  Note, in particular, the
# definition of "__unix__", this is needed by gcc on MacOS X 10.4 to get
# the conditional compilation in "sim.c" to work.
#
CFLAGS = -ansi -pedantic -D__unix__
#
# A list of object files to build into an executable.
#
OBJS = sim.o command.o sdis.o simulate.o sparse.o
#
# targets
#
sim : $(OBJS)
	$(CC) -o sim $(OBJS)
clean :
	$(RM) $(OBJS)
veryclean :
	$(RM) $(OBJS) sim
