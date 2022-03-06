#ifndef  SIMHEADER
/*

        sim.h

        Header file for simulator program.

*/

#define   SIMHEADER

#define   CODESIZE      1024    /*      Size of code space              */
#define   STACKSIZE     1024    /*      Size of stack space             */

#define   FATAL            1    /*      Code to DisplayError telling it */
                                /*      to cloase the logging file and  */
                                /*      exit.                           */

#define   BANNER        "sim v1.0.  Type ? for help\n"

#define   LOGFILE       "sim.log"    /* Name of file containing trace   */
#define   BACKUPFILE    "sim.bak"    /* Name of file containing backup  */

/*
        Instruction opcodes
*/

#define   HALT          0       /*      Halt instruction                */
#define   ADD           1       /*      Add                             */
#define   SUB           2       /*      Subtract                        */
#define   MULT          3       /*      Multiply                        */
#define   DIV           4       /*      Divide                          */
#define   NEG           5       /*      Negate                          */
#define   BR            6       /*      Branch to address               */
#define   BGZ           7       /*      Branch if >= 0                  */
#define   BG            8       /*      Branch if > 0                   */
#define   BLZ           9       /*      Branch if <= 0                  */
#define   BL            10      /*      Branch if < 0                   */
#define   BZ            11      /*      Branch if != 0                  */
#define   BNZ           12      /*      Branch if != 0                  */
#define   CALL          13      /*      Call a subroutine               */
#define   RET           14      /*      Return from a subroutine        */
#define   BSF           15      /*      Build a stack frame             */
#define   RSF           16      /*      Remove a stack frame            */
#define   LDP           17      /*      Load Display Pointer            */
#define   RDP           18      /*      Restore old Display Pointer     */
#define   INC           19      /*      Push value in FP register       */
#define   DEC           20      /*      Push value in FP register       */
#define   PUSHFP        21      /*      Push value in FP register       */
#define   LOADI         22      /*      Push Immediate Data             */
#define   LOADA         23      /*      Push memory at <address>        */
#define   LOADFP        24      /*      Push memory at FP+<offset>      */
#define   LOADSP        25      /*      Replace Top of stack with       */
                                /*      contents of the memory location */
                                /*      given by the current top of     */
                                /*      stack plus an offset            */
#define   STOREA        26      /*      Pop ToS and store at <address>  */
#define   STOREFP       27      /*      Pop ToS and store relative to   */
                                /*      FP  ( i.e., FP + <offset> )     */
#define   STORESP       28      /*      Use ToS + <offset> to form an   */
                                /*      address. Store the second stack */
                                /*      element at that address, then   */
                                /*      remove these two elements       */
#define   READ          29      /*      Read an integer value from the  */
                                /*      input device and push it.       */
#define   WRITE         30      /*      Pop Top of stack and display    */
                                /*      it as a decimal value on the    */
                                /*      terminal.                       */

typedef  struct  {
    int  opcode;
    int  offset;
}
    INSTRUCTION;

void  WriteOut( char *formatstring, ... );
char  *ReadString( void );
void  DisplayError( int flag, char *formatstring, ... );
#endif
