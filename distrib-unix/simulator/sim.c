/*

        sim.c

        simulator program for stack machine

*/

#include  <stdio.h>
#include  <stdlib.h>
#include  <stdarg.h>
#include  <string.h>
#if (defined(__CYGWIN__) || defined(__linux__) || defined(__unix__)) 
#include  <unistd.h>            /* for Linux/gcc or Cygwin/gcc system   */
#else
#include  <io.h>                /* for Windows using CL or BCC32        */
#endif
#include  "sparse.h"
#include  "sdis.h"
#include  "sim.h"
#include  "simulate.h"
#include  "command.h"

#define   RUN_MODE           1  /* Non-interactive mode, where the sim
                                   reads a program and executes it at
                                   once, without issuing a command
                                   prompt.                              */
#define   PROMPT_MODE        0  /* Interative mode, where the simulator
                                   issues a prompt and waits for the
                                   user to enter commands.              */
#define   MAXEXE_R     1000000  /* max number of instructions to 
                                   execute (without tracing) in one go
                                   in run (non_interative) mode.        */
#define   MAXEXE_P        1000  /* max number of instructions to 
                                   execute (without tracing) in one go
                                   in prompt (interative) mode.         */
#define   MAXTRACE          20  /* max number of instructins to trace 
                                   in one go                            */
#define   MAXDATA          100  /* max number of words of data memory
                                   to display in one go                 */
#define   DEFAULTDATA       20  /* default number of words of data 
                                   memory to display in one go          */
#define   MAXCODE           20  /* max number of instructions to display
                                   in one go                            */
#define   TERMWIDTH         80  /* assumed width of terminal            */

#define   MAXBREAKPTS       10  /* Maximum number of simultaneous       */
                                /* breakpoints allowed in code          */

static int bplst[MAXBREAKPTS];  /* List of current active breakpoints   */
static int bpcount = 0;         /* Count of current active breakpoints  */

FILE  *LogFile = NULL;

static void ReadInstructions( FILE *inputfile );
static void ProcessCommands( void );
static void DisplayCodeMemory( int start, int words );
static void DisplayDataMemory( int start, int words );
static void TraceCode( int InitPC, int InitSP, int InitFP, int InstLimit );
static void RunCode( int InitPC, int InitSP, int InitFP, int Mode, int InstLimit );
static void DisplayState( int ShowBanner );
static int  CheckandAssign( int InitPC,  int InitSP, int InitFP );
static void CentrePrint( int value, int width, int rightspacing );
static void SetLogging( void );
static void MakeBackup( void );
static void AddBreakPoint(int address);
static void ShowBreakPoints(void);
static void ClearBreakPoints(void);


int main(int argc, char *argv[])
{
    FILE *inputfile;

    if ( argc != 2 && argc != 3 )  {
        fprintf( stderr, "Usage:  sim [-r] <assemblyfile>\n" );
        fprintf( stderr, "    Assemble <assemblyfile>, then:\n");
        fprintf( stderr, "      If -r is specified, run the code immediately "
                               "and then exit the simulator;\n");
        fprintf( stderr, "      Otherwise, issue a simulator prompt and "
                               "await interactive commands.\n");
    }
    else if ( strncmp(argv[1], "-r", 2) == 0 ) {  /* non-interactive mode */
        if ( NULL == ( inputfile = fopen( argv[2], "r" ) ) )  {
            fprintf( stderr, "Error, can't open %s for input\n", argv[2] );
            exit( EXIT_FAILURE );
        }
        else {
            ReadInstructions( inputfile );
            fclose( inputfile );
            /* Immediately run the code on loading, with a very big
             * execution limit.  If the program gets stuck in an infinite
             * loop, the user can type Ctrl-C!
             */
            RunCode( 0, -1, 0, RUN_MODE, MAXEXE_R );
        }
    }
    else if ( NULL == ( inputfile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "Error, can't open %s for input\n", argv[1] );
        exit( EXIT_FAILURE );
    }
    else  {
        printf( BANNER );
        ReadInstructions( inputfile );
        fclose( inputfile );
        printf( "Completed instruction read\n\n" );
        ProcessCommands();
        if ( LogFile != NULL )  fclose( LogFile );
    }
    return EXIT_SUCCESS;
}

char *ReadString( void )
{
    static char buffer[132];

    fgets( buffer, 132, stdin );
    if ( LogFile != NULL )  fprintf( LogFile, "%s\n", buffer );
    return buffer;
}

void WriteOut( char *formatstring, ... )
{
    va_list argptr;
    char buffer[132];

    va_start(argptr, formatstring);
    vsnprintf( buffer, sizeof(buffer), formatstring, argptr );
    va_end(argptr);

    fputs( buffer, stdout );
    if ( LogFile != NULL )  fputs( buffer, LogFile );
}

void DisplayError( int flag, char *formatstring, ... )
{
    va_list argptr;
    char buffer[132];

    va_start(argptr, formatstring);
    vsnprintf( buffer, sizeof(buffer), formatstring, argptr );
    va_end(argptr);

    fputs( buffer, stderr );
    if ( LogFile != NULL )  fputs( buffer, LogFile );  
    if ( flag == FATAL )  {
        if ( NULL != LogFile )  fclose( LogFile );
        exit( EXIT_FAILURE );
    }
}

static void ReadInstructions( FILE *inputfile )
{
    int i, *dp;
    INSTRUCTION *ip;

    for ( ip = Pmem, i = 0; i < CODESIZE; i++, ip++ )  {
        ip->opcode = HALT;  ip->offset = 0;
    }
    for ( dp = Dmem, i = 0; i < STACKSIZE; i++, dp++ )  *dp = 0;

    InitParsing( inputfile );
    for ( ip = Pmem, i = 0; i < CODESIZE; i++, ip++ )
        if ( !ReadInstruction( ip ) )  break;
}

static void ProcessCommands( void )
{
    int  i, quit, command, v1, v2;

    quit = 0;
    while ( !quit )  {
        command = ReadCommand( &v1, &v2 );
        switch( command )  {
            case  EXIT :  quit = 1;                                     break;
            case  DISREGS :
                WriteOut( "  PC = %4d,  SP = %4d,  FP = %4d\n", PC, SP, FP );
                break;
            case  DDMEM :
                if ( v1 != -1 )  {
                    if ( v2 > 0 && v2 <= MAXDATA )  DisplayDataMemory( v1, v2 );
                    else  DisplayDataMemory( v1, DEFAULTDATA );
                }
                else  DisplayDataMemory( SP, DEFAULTDATA );
                break;
            case  DCMEM :
                if ( v1 != -1 )  {
                    if ( v2 > 0 && v2 <= MAXCODE )  DisplayCodeMemory( v1, v2 );
                    else  DisplayCodeMemory( v1, MAXCODE );
                }
                else  DisplayCodeMemory( PC, MAXCODE );
                break;
            case  RUN :
                if ( v1 == -1 )  v1 = MAXEXE_P;
                RunCode( PC, SP, FP, PROMPT_MODE, v1 );
                break;
            case  TRACE :
                if ( v1 != -1 )  TraceCode( PC, SP, FP, v1 );
                else  TraceCode( PC, SP, FP, 1 );
                break;
            case  SETPC :
                if ( v1 >= 0 && v1 < CODESIZE )  PC = v1;
                else  DisplayError( !FATAL, "Illegal PC\n" );           break;
            case  SETSP :
                if ( v1 >= 0 && v1 < STACKSIZE )  SP = v1;
                else  DisplayError( !FATAL, "Illegal SP\n" );           break;
            case  SETFP :
                if ( v1 >= 0 && v1 < STACKSIZE )  FP = v1; 
                else  DisplayError( !FATAL, "Illegal FP\n" );           break;
            case  SETDM :  
                if ( v1 >= 0 && v1 < STACKSIZE )  Dmem[v1] = v2;
                else  DisplayError( !FATAL, "Illegal Address\n" );      break;
            case  RESET :  
                PC = 0;  SP = -1;  FP = 0;
                for ( i = 0; i < STACKSIZE; i++ )  *(Dmem+i) = 0;
                break;
            case  LOG   :  SetLogging();                                break;
            case  SETBP : 
                AddBreakPoint(v1);                                      break;
            case  SHOWBP : 
                ShowBreakPoints();                                      break;
            case  CLEARBP : 
                ClearBreakPoints();                                     break;
            default     :  
                DisplayError( FATAL, "Error, unknown command %d\n", command );
                break;
        }
    }
}

static void DisplayDataMemory( int start, int words )
{
    int i, j, wordsperline, finish;

    wordsperline = ( TERMWIDTH - 6 ) / 7;
    finish = start - words;

    if ( start == -1 )  
        WriteOut( "Stack Empty\n" );
    else if ( start < 0 || start >= STACKSIZE )
        DisplayError ( !FATAL, "Error, start address %d is illegal\n", start );
    else  {
        for ( j = 1, i = start; i > finish && i >= 0; i-- )  {
            if ( j == 1 )  WriteOut( "%4d: ", i );
            CentrePrint( Dmem[i], 6, 1 );
            if ( j == wordsperline )  {
                j = 1;  WriteOut( "\n" );
            }
            else j++;
        }
        if ( j != 1 )  WriteOut( "\n" );
    }
}

static void DisplayCodeMemory( int start, int words )
{
    char buffer[80];
    int i, finish;

    finish = start + words;

    if ( start < 0 || start >= CODESIZE )
        DisplayError( !FATAL, "Error, start address %d is illegal\n", start );
    else  {
        WriteOut( "\n" );
        for ( i = start; i < finish && i < CODESIZE; i++ )  {
            Disassemble( Pmem+i, buffer );
            WriteOut( "%4d  %s\n", i, buffer );
        }
        WriteOut( "\n" );
    }
}

static void TraceCode( int InitPC, int InitSP, int InitFP, int InstLimit )
{
    int i;

    if ( CheckandAssign( InitPC, InitSP, InitFP ) )  {
        if ( InstLimit <= MAXTRACE )  {
            Running = 1;
            DisplayState( 1 );
            for ( i = 0; i < InstLimit && Running; i++ )  {
                Interpret();
                DisplayState( 0 );
            }
        }
        else  
            DisplayError( !FATAL, "Error: Too many instructions to trace, max %d\n",
                            MAXTRACE );
    }
}

static void RunCode( int InitPC, int InitSP, int InitFP, int Mode, int InstLimit )
{
    int insts, bp, HardUpperInstCountLimit;

    HardUpperInstCountLimit = ( Mode == RUN_MODE ) ? MAXEXE_R : MAXEXE_P;

    if ( CheckandAssign( InitPC, InitSP, InitFP ) )  {
        if ( InstLimit <= HardUpperInstCountLimit ) {
            Running = 1;
            for ( insts = 0; Running & (insts < InstLimit); insts++ )  {
                for ( bp = 0; bp < bpcount; bp++ ) {
                    if ( PC == bplst[bp] ) {
                        WriteOut( "\n!! Breakpoint at %d\n\n", PC );
                        Running = 0;
                    }
                }
                if ( Running ) Interpret();
            }
            if ( insts == InstLimit )
                WriteOut( "\n!! Instruction execution limit reached (%d instructions)\n\n",
                          InstLimit );
            DisplayState( 1 );
        }
        else DisplayError( !FATAL, "Error: Too many instructions to execute, max %d\n",
                           HardUpperInstCountLimit );
    }
}

static void DisplayState( int ShowBanner )
{
    char buffer[80];
    int  i;

    if ( ShowBanner )  {
        WriteOut( "Addr   SP    FP    [SP]  [SP-1] [SP-2] [SP-3]" );
        WriteOut( "   Instruction\n" );
        WriteOut( "----  ----  ----  ------ ------ ------ ------" );
        WriteOut( " ----------------\n" );
    }
    CentrePrint( PC, 4, 2 ); 
    CentrePrint( SP, 4, 2 ); 
    CentrePrint( FP, 4, 2 );
    for ( i = 0; i < 4; i++ )  {
        if ( SP-i >= 0 )  CentrePrint( Dmem[SP-i], 6, 1 );
        else  WriteOut( "  --   " );
    }
    Disassemble( Pmem+PC, buffer );  WriteOut( "  %-16s\n", buffer );  
}

static int  CheckandAssign( int InitPC,  int InitSP, int InitFP )
{
    int status;

    status = 0;
    if ( InitPC < 0 || InitPC >= CODESIZE ) 
        DisplayError( !FATAL, "Illegal Program Counter %d\n", InitPC );
    else if ( InitSP < -1 || InitSP >= STACKSIZE ) 
        DisplayError( !FATAL, "Illegal Stack Pointer %d\n", InitSP );
    else if ( InitFP < 0  || InitFP >= STACKSIZE ) 
        DisplayError( !FATAL, "Illegal Frame Pointer %d\n", InitFP );
    else  {
        PC = InitPC;  SP = InitSP;  FP = InitFP;
        status = 1;
    }
    return status;
}

static void CentrePrint( int value, int width, int rightspacing )
{
    char buffer[20], buffer2[30], *p, *q;
    int  slen, i, j;

    if ( width >= 20 )  {
        DisplayError( FATAL, "Error: CentrePrint, width > 20\n" );
    }
    snprintf( buffer, sizeof(buffer), "%1d", value );  
    slen = strnlen( buffer, sizeof(buffer) );
    if ( slen > width )  {
        for ( p = buffer2, i = 0; i < width; i++, p++ )  *p = '*';
        *p = '\0';
    }
    else  {
        i = (width+1-slen)/2;
        for ( q = buffer2, j = 0; j < i; j++, q++ )  *q = ' ';
        for ( p = buffer; *p != '\0'; p++, j++, q++ )  *q = *p;
        for ( ; j < width; j++, q++ )  *q = ' ';
        for ( j = 0; j < rightspacing; j++, q++ )  *q = ' ';
        *q = '\0';
    }
    WriteOut( buffer2 );
}

static void SetLogging( void )
{
    static int  FirstOpen = 1;       /* True when the log file is first   */
                                     /* opened in a session               */

    if ( FirstOpen )  {
        MakeBackup();
        if ( NULL == ( LogFile = fopen( LOGFILE, "w" ) ) ) 
            DisplayError( !FATAL, "Error, can't open the logging file %s\n",
                          LOGFILE );
        else  {
            printf( "Logging output on \"%s\"\n", LOGFILE );
            FirstOpen = 0;
        }
    }
    else  {
        if ( LogFile != NULL )  {
            fclose( LogFile );
            LogFile = NULL;
            printf( "Logging disabled\n" );
        }
        else  {
            if ( NULL == ( LogFile = fopen( LOGFILE, "a" ) ) )
                DisplayError( !FATAL, "Error, can't open the logging file %s\n",
                              LOGFILE );
            else  printf( "Appending output to \"%s\"\n", LOGFILE );
        }
    }
}


/* Some fiddling needed here because ISO C99 (default in Visual Studio    *
 * 2005) wants "_access" and "_unlink".  However, gcc (ISO C90) wants     *
 * "access" and "unlink".  Borland C and Visual Studio 6 seem happy with  *
 * either.
 */

static void MakeBackup( void )
{
#if (defined(__CYGWIN__) || defined(__linux__) || defined(__unix__))
    /* for Linux/gcc or Cygwin/gcc system                                 */
    if ( 0 == access( BACKUPFILE, F_OK ) )  unlink( BACKUPFILE );
    if ( 0 == access( LOGFILE, F_OK ) )  rename( LOGFILE, BACKUPFILE );
#else                          
    /* for Windows using CL or BCC32                                      */
#define  F_OK   0
    if ( 0 == _access( BACKUPFILE, F_OK ) )  _unlink( BACKUPFILE );
    if ( 0 == _access( LOGFILE, F_OK ) )  rename( LOGFILE, BACKUPFILE );
#endif
}

static void AddBreakPoint(int address)
{
    if ( address > 0 && address < CODESIZE ) {
        if ( bpcount < MAXBREAKPTS )
            bplst[bpcount++] = address;
        else
            WriteOut( "\n Too many breakpoints requested, only %d allowed\n", 
                      MAXBREAKPTS );
    }
    else WriteOut( "\n !! Illegal breakpoint address, must be in range 0 .. %d\n\n",
                   CODESIZE );
}

static void ShowBreakPoints(void)
{
    int i;

    if ( bpcount > 0 ) {
        if ( bpcount == 1 ) 
            WriteOut( "\n 1 breakpoint active, at code address %d\n\n", bplst[0] );
        else {
            WriteOut( "\n %d breakpoints active, at code addresses:", bpcount );
            for ( i = 0; i < bpcount; i++) { 
                WriteOut( " %d", bplst[i] );
                if ( i < bpcount - 1 ) WriteOut(",");
            }
            WriteOut( "\n\n" );
        }
    }
    else WriteOut( "\n No breakpoints active\n\n" );
}

static void ClearBreakPoints(void)
{
    bpcount = 0;
}
