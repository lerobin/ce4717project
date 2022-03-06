#ifndef SPARSEHEADER
/*
        sparse.h

        header file for routine to read and assemble instructions from
        a file
*/

#define  SPARSEHEADER
#include "sim.h"

void  InitParsing( FILE *inputfile );
int   ReadInstruction( INSTRUCTION *Instruction );
#endif
