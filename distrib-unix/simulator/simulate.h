#ifndef  SIMULATEHEADER
/*

        simulate.h

        header file for the simulation routines which actually modify
        the state of the machine.

        Note the external definitons of the code, stack and registers if
        the module currently being compiled is other than SIMULATEMODULE
*/

#define   SIMULATEHEADER
#include  "sim.h"

#ifdef    SIMULATEMODULE

INSTRUCTION        Pmem[CODESIZE];      /*  Program Memory space        */

int                Dmem[STACKSIZE];     /*  Data Memory (stack) space   */

int                PC = 0,      /*      Program Counter                 */
                   SP = -1,     /*      Stack Pointer                   */
                   FP =  0,     /*      Frame Pointer                   */
                   Running = 0; /*      State Flag                      */
#else

extern INSTRUCTION Pmem[CODESIZE];

extern int         Dmem[STACKSIZE];

extern int         PC,
                   SP,
                   FP,
                   Running;
#endif

void  Interpret( void );        /*      Instruction Execution Code      */

#endif

