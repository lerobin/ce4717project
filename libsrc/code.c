/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      code.c                                                               */
/*                                                                           */
/*      Implementation file for the code generator.                          */
/*                                                                           */
/*      Code is stored in an array "CodeTable", and written out on a call    */
/*      to "WriteCodeFile". Since the code is all stored in memory, it is    */
/*      possible to perform backpatching of branch addresses. However, the   */
/*      finite size of the "CodeTable" array means that there is a limit on  */
/*      the size of program which may be generated.                          */
/*                                                                           */ 
/*      Six routines and one macro are provided by this module.              */
/*                                                                           */ 
/*          "InitCodeGenerator"  -- this is used to prepare the code         */
/*          generator by establishing the output file where the assembly     */
/*          code is to be written and by setting up various internal code    */
/*          generator structures.                                            */
/*                                                                           */ 
/*          "WriteCodeFile" -- this is used to output the contents of the    */ 
/*          code array as an ASCII assembly language file for the stack      */ 
/*          computer.                                                        */ 
/*                                                                           */ 
/*          "KillCodeGeneration" is a call which is used to stop the         */ 
/*          production of further assenbly language instructions in the      */ 
/*          event of an error being detected by the parser or semantic       */ 
/*          analysis routines. Code generation is governed by a variable     */ 
/*          "ErrorsInProgram". If this variable is set by this routine, no   */ 
/*          code can be output to the assembly file.                         */ 
/*                                                                           */ 
/*          "Emit" is the call which outputs instructions. "Emit" is         */
/*          designed to work with "1-address" instructions, i.e., those      */
/*          which have an "address" field. For convenience a macro,          */
/*          "_Emit" is defined in the header file, which allows zero         */
/*          address instructions to be easily handled. It acts like an       */
/*          "Emit" call with only one parameter, the opcode. It simply       */
/*          expands into a call to "Emit" with the address field zeroed.     */
/*          This macro is defined in the header file.                        */
/*                                                                           */ 
/*          "CurrentCodeAddress" is a routine which returns the address of   */ 
/*          the next loaction available in code memory. It is used by        */ 
/*          routines which need to backpatch the code array.                 */ 
/*                                                                           */ 
/*          "BackPatch" is a routine which actually backpatches the code     */ 
/*          array.                                                           */ 
/*                                                                           */ 
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "code.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Definitions of constants local to the module                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  CODETABLESIZE                 1024 

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Data Structures for this module                                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/

typedef struct  {       /* definition of an instruction in the internal code */
    int opcode;         /* array.                                            */
    int address;
}
    INSTRUCTION;

PRIVATE FILE         *CodeFile = NULL;
PRIVATE INSTRUCTION  CodeTable[CODETABLESIZE];
PRIVATE int          CodePosition = 0;
PRIVATE int          ErrorsInProgram = 0;

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Function Prototypes for private routines                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void  Output( int i );
PRIVATE void  OutputControlInst( char *s, int i );
PRIVATE void  OutputDataInst( char *s, int i );
PRIVATE void  OutputFPInst( char *s, int i );
PRIVATE void  OutputSPInst( char *s, int i );

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      InitCodeGenerator                                                    */
/*                                                                           */
/*      Starts up the code generator.                                        */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          codefile   pointer to a FILE structure, this is the file to      */
/*                     which the assembly code will be written.              */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   InitCodeGenerator( FILE *codefile )
{
    if ( codefile == NULL ) {
      fprintf( stderr, "Fatal Error: InitCodeGenerator: attempt to\n");
      fprintf( stderr, "use an invalid file handle (NULL) for output\n");
      exit( EXIT_FAILURE );
    }
    CodeFile = codefile;
    CodePosition = 0;
    ErrorsInProgram = 0;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      WriteCodeFile                                                        */
/*                                                                           */
/*      Outputs the contents of the CodeTable to the file.                   */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   WriteCodeFile( void )
{
    int i;

    if ( CodeFile == NULL ) {
      fprintf( stderr, "Fatal Error: WriteCodeFile: attempt to\n");
      fprintf( stderr, "use an invalid file handle (NULL) for output\n");
      exit( EXIT_FAILURE );
    }

    if ( !ErrorsInProgram )
        for ( i = 0; i < CodePosition; i++ )  Output( i );
    else  {
        fprintf( CodeFile, ";; Errors detected in input file, no code\n" );
        fprintf( CodeFile, ";; generated\n" );
    }
    fclose( CodeFile );
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      KillCodeGeneration                                                   */
/*                                                                           */
/*      Prevents any further code generation taking place and stops the      */
/*      code file being written to disk. Sets the module global variable     */
/*      "ErrorsInProgram" to 1.                                              */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   KillCodeGeneration( void )
{
    ErrorsInProgram = 1;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Emit                                                                 */
/*                                                                           */
/*      Places an instruction opcode/address pair in the current location    */
/*      in the CodeTable (indexed by CodePosition) and increments this       */
/*      location. If the CodeTable overflows, issues an error message to     */
/*      stderrr and forces program exit.                                     */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          opcode    integer, instruction opcode.                           */
/*                                                                           */
/*          address   integer, instruction address, offset or value field.   */
/*                    the interpretation of this value depends on the        */
/*                    instruction opcode.                                    */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   Emit( int opcode, int offset )
{
    if ( CodePosition >= CODETABLESIZE )  {
        fprintf( stderr, "Fatal compiler error, code table overflow\n" );
        fprintf( stderr, "(max allowed code size is %d instructions\n",
                 CODETABLESIZE );
        exit( EXIT_FAILURE );
    }
    else  {
        CodeTable[CodePosition].opcode = opcode;
        CodeTable[CodePosition].address = offset;
        CodePosition++;
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      CurrentCodeAddress                                                   */
/*                                                                           */
/*      Returns the current value of "CodePosition". This is where the next  */
/*      "Emit" call will place its instruction. Used to retrieve code        */
/*      memory addresses for backpatching operations while compiling, for    */
/*      example, IF or WHILE statements.                                     */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Integer, current value of "CodePosition".             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC int    CurrentCodeAddress( void )
{
    return CodePosition;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      BackPatch                                                            */
/*                                                                           */
/*      Allows the address field of a specific instruction in memory to be   */
/*      modified after an opcode/address pair has already been placed in     */
/*      it. Used to modify jump and call instructions so that they have the  */
/*      correct target addresses.                                            */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          codeaddr  integer, location in code memory the address field of  */
/*                    whose instruction is to be modified.                   */
/*                                                                           */
/*          value     integer, the new value to be placed in the address     */
/*                    field of the instruction at location "codeaddr".       */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   BackPatch( int codeaddr, int value )
{
    if ( codeaddr < 0 || codeaddr >= CODETABLESIZE )  {
        fprintf( stderr, "Fatal internal error, attempt to BackPatch to " );
        fprintf( stderr, "location %d\n", codeaddr );
        fprintf( stderr, "This location is outside the valid set of code " );
        fprintf( stderr, "addresses, 0 .. %d\n", CODETABLESIZE-1 );
        exit( EXIT_FAILURE );
    }
    else  CodeTable[codeaddr].address = value;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Private routines (only accessable from within this module).          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Output                                                               */
/*                                                                           */
/*      Outputs a mnemonic form of an instruction to the code file where     */
/*      the assembly language program is being built. Only does this if      */
/*      there are no errors in the program.                                  */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          i         integer, index in the CodeTable of the instruction     */
/*                    which is to be output.                                 */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void  Output( int i )
{
    if ( ErrorsInProgram )  return;

    fprintf( CodeFile, "%3d  ", i );
    switch ( CodeTable[i].opcode )  {
        case  I_ADD      :  fprintf( CodeFile, "Add\n" );         break;
        case  I_SUB      :  fprintf( CodeFile, "Sub\n" );         break;
        case  I_MULT     :  fprintf( CodeFile, "Mult\n" );        break;
        case  I_DIV      :  fprintf( CodeFile, "Div\n" );         break;
        case  I_NEG      :  fprintf( CodeFile, "Neg\n" );         break;
        case  I_RET      :  fprintf( CodeFile, "Ret\n" );         break;
        case  I_BSF      :  fprintf( CodeFile, "Bsf\n" );         break;
        case  I_RSF      :  fprintf( CodeFile, "Rsf\n" );         break;
        case  I_PUSHFP   :  fprintf( CodeFile, "Push  FP\n" );    break;
        case  I_READ     :  fprintf( CodeFile, "Read\n" );        break;
        case  I_WRITE    :  fprintf( CodeFile, "Write\n" );       break;
        case  I_HALT     :  fprintf( CodeFile, "Halt\n" );        break;
        case  I_BR       :  OutputControlInst( "Br  ", i );       break;
        case  I_BGZ      :  OutputControlInst( "Bgz ", i );       break;
        case  I_BG       :  OutputControlInst( "Bg  ", i );       break;
        case  I_BLZ      :  OutputControlInst( "Blz ", i );       break;
        case  I_BL       :  OutputControlInst( "Bl  ", i );       break;
        case  I_BZ       :  OutputControlInst( "Bz  ", i );       break;
        case  I_BNZ      :  OutputControlInst( "Bnz ", i );       break;
        case  I_CALL     :  OutputControlInst( "Call", i );       break;
        case  I_LDP      :  OutputControlInst( "Ldp ", i );       break;
        case  I_RDP      :  OutputControlInst( "Rdp ", i );       break;
        case  I_INC      :  OutputControlInst( "Inc ", i );       break;
        case  I_DEC      :  OutputControlInst( "Dec ", i );       break;
        case  I_LOADI    :  fprintf( CodeFile, "Load  #%-4d\n",
                                     CodeTable[i].address );      break;
        case  I_LOADA    :  OutputDataInst( "Load ", i );         break;
        case  I_LOADFP   :  OutputFPInst( "Load ", i );           break;
        case  I_LOADSP   :  OutputSPInst( "Load ", i );           break;
        case  I_STOREA   :  OutputDataInst( "Store", i );         break;
        case  I_STOREFP  :  OutputFPInst( "Store", i );           break;
        case  I_STORESP  :  OutputSPInst( "Store", i );           break;
        default:
            fprintf( CodeFile, "Fatal compiler error, unknown opcode %d\n",
                               CodeTable[i].opcode );
            fclose( CodeFile );
            fprintf( stderr, "Fatal compiler error, unknown opcode %d\n",
                             CodeTable[i].opcode );
            fprintf( stderr, "Code address %d\n", i );
            exit( EXIT_FAILURE );
            break;
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      OutputControlInst                                                    */
/*      OutputDataInst                                                       */
/*      OutputFPInst                                                         */
/*      OutputSPInst                                                         */
/*                                                                           */
/*      These four small utility routines handle the ouput of various        */
/*      types of instructions. They all share the same parameter types.      */
/*                                                                           */
/*      OutputControlInst     Outputs a mnemonic form of a branch, call      */
/*                            ldp, rdp inc or dec instruction                */
/*                                                                           */
/*      OutputDataInst        Outputs a mnemonic form of a LOAD or STORE     */
/*                            <addr> (absolute addressing) instruction       */
/*                                                                           */
/*      OutputFPInst          Outputs a mnemonic form of a LOAD or STORE     */
/*                            FP relative instruction                        */
/*                                                                           */
/*      OutputSPInst          Outputs a mnemonic form of a LOAD or STORE     */
/*                            SP relative instruction                        */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          s         pointer to a character string, the mnemonic.           */
/*          i         integer, index in the CodeTable of the instruction     */
/*                    which is to be output.                                 */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void  OutputControlInst( char *s, int i )
{
    fprintf( CodeFile, "%s  %-4d\n", s, CodeTable[i].address );
}

PRIVATE void  OutputDataInst( char *s, int i )
{
    fprintf( CodeFile, "%s %-4d\n", s, CodeTable[i].address );
}

PRIVATE void  OutputFPInst( char *s, int i )
{
    int offset;

    offset = CodeTable[i].address;
    fprintf( CodeFile, "%s FP", s );
    if ( offset == 0 )  fputc( '\n', CodeFile );
    else if ( offset > 0 )  fprintf( CodeFile, "+%-4d\n", offset );
    else  fprintf( CodeFile, "%-4d\n", offset );
}

PRIVATE void  OutputSPInst( char *s, int i )
{
    int offset;

    offset = CodeTable[i].address;
    fprintf( CodeFile, "%s [SP]", s );
    if ( offset == 0 )  fputc( '\n', CodeFile );
    else if ( offset > 0 )  fprintf( CodeFile, "+%-4d\n", offset );
    else  fprintf( CodeFile, "%-4d\n", offset );
}


