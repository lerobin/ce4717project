/*
        command.c

        command line processor
*/

#include  <stdio.h>
#include  <stdlib.h>
#include  <ctype.h>
#include  <string.h>
#include  "sim.h"
#include  "command.h"

#define   PROMPT          "==> "

#define   MAXKEYWORDLEN      40
#define   MAXVALUE        32750

#define   ENDLINE          -1
#define   INTCONST         -2
#define   ERROR            -3
#define   KEYWORD          -4
#define   HELP             -5


static unsigned char *lptr;

static int  ParseKeyword( int *value );
static void DisplayHelp( void );

int  ReadCommand( int *p1, int *p2 )
{
    int quit, command, keyword, value;

    quit = 0;
    command = ERROR;  /* Microsoft CL at /W4 requires this initialisation. */
    while ( !quit )  {
        WriteOut( PROMPT );
        lptr = (unsigned char *) ReadString();  /* Cast stops cygwin gcc 4.8.3 complaining about (signed) char indices. */
        command = ParseKeyword( &value );
        switch ( command )  {
            case  ENDLINE:  break;
            case  ERROR:    break;
            case  HELP:  
                DisplayHelp();
                break;
            case  INTCONST:
                DisplayError( !FATAL, "Integer value not allowed here\n" );
                break;
            case  EXIT:
            case  LOG:
            case  RESET:
            case  DISREGS:  
	    case  SHOWBP:
	    case  CLEARBP:
                quit = 1;  
                break;
            case  RUN:
            case  TRACE:
                if ( INTCONST == ( keyword = ParseKeyword( &value ) ) )  {
                    *p1 = value;  *p2 = -1;  quit = 1;
                }
                else if ( keyword == ENDLINE )  {
                    *p1 = -1;  *p2 = -1;  quit = 1;
                }
                else  DisplayError( !FATAL, "Integer or end of line expected\n" );
                break;
            case  SETPC:
            case  SETSP:
            case  SETFP:
	    case  SETBP:
                if ( INTCONST == ParseKeyword( &value ) )  {
                    *p1 = value;  *p2 = -1;  quit = 1;
                }
                else  DisplayError( !FATAL, "Integer expected\n" );
                break;
            case  DDMEM:
            case  DCMEM:
                if ( INTCONST == ( keyword = ParseKeyword( &value ) ) )  {
                    *p1 = value;  
                    if ( INTCONST == ( keyword = ParseKeyword( &value ) ) )  {
                        *p2 = value;  quit = 1;
                    }
                    else if ( keyword == ENDLINE )  {
                        *p2 = -1;  quit = 1;
                    }
                    else  DisplayError( !FATAL, "Integer or end of line expected\n" );
                }
                else if ( keyword == ENDLINE )  {
                    *p1 = -1;  *p2 = -1;  quit = -1;
                }
                else  DisplayError( !FATAL, "Integer or end of line expected\n" );
                break;
            case  SETDM:
                if ( INTCONST == ParseKeyword( &value ) )  {
                    *p1 = value;  
                    if ( INTCONST == ParseKeyword( &value ) )  {
                        *p2 = value;  quit = 1;
                    }
                    else  DisplayError( !FATAL, "Integer expected\n" );
                }
                else  DisplayError( !FATAL, "Two integer values expected\n" );
                break;
            default:
                DisplayError( FATAL, "Error, unknown state in ReadCommand %d\n",
                              command );
                break;
        }
    }
    return  command;
}

static int  ParseKeyword( int *value )
{
    int keywordcode, state, len, quit;
    char keybuffer[MAXKEYWORDLEN], *kptr;

    if ( feof(stdin) )  { WriteOut("\n"); return EXIT; }

    keywordcode = ERROR; /* Microsoft CL at /W4 requires this initialisation */
    len = 0;             /* Microsoft CL at /W4 requires this initialisation */

    state = 0;  quit = 0;
    kptr = keybuffer;

    while ( !quit )  {
        switch( state )  {
            case  0:
                if ( *lptr == '\n' || *lptr == '\0' )  {
                    keywordcode = ENDLINE;  quit = 1;
                }
                else if ( *lptr == '?' )  {
                    keywordcode = HELP;  quit = 1;
                }
                else if ( isspace( *lptr ) )  lptr++;
                else if ( isalpha( *lptr ) )  {
                    *kptr = (char)toupper( *lptr );  kptr++;  lptr++;
                    len = 1;  state = 1;
                }
                else if ( isdigit( *lptr ) )  {
                    *value = *lptr - '0';  lptr++;
                    state = 2;
                }
                else  {
                    DisplayError( !FATAL, "Illegal Character in input %c\n", *lptr );
                    keywordcode = ERROR;  quit = 1;
                }
                break;
            case  1:
                if ( isalpha( *lptr ) )  {
                    if ( len < MAXKEYWORDLEN )  {
                        *kptr = (char)toupper( *lptr );  kptr++;
                    }
                    lptr++;  len++;
                }
                else  {
                    *kptr = '\0';  keywordcode = KEYWORD;  quit = 1;
                }
                break;
            case  2:
                if ( isdigit( *lptr ) )  {
                    if ( *value <= (MAXVALUE/10) )  {
                        *value *= 10;
                        *value += ( *lptr - '0' );
                        lptr++;
                    }
                    else  {
                        DisplayError( !FATAL, "Integer value too large, max = %d\n",
                                      MAXVALUE );
                        keywordcode = ERROR;  quit = 1;
                    }
                }
                else  {
                    keywordcode = INTCONST;  quit = 1;
                }
                break;
            default:
                DisplayError( FATAL, "Command line scanner, unknown state %d\n",
                              state );
        }
    }

    if ( keywordcode == KEYWORD )  {
        if ( *keybuffer == 'E' )             keywordcode = EXIT;
        else if ( *keybuffer == 'H' )        keywordcode = HELP;
        else if ( *keybuffer == 'L' )        keywordcode = LOG;
        else if ( *keybuffer == 'T' )        keywordcode = TRACE;
        else if ( *keybuffer == 'R' &&
                  *(keybuffer+1) == 'U' )    keywordcode = RUN;
        else if ( *keybuffer == 'R' &&
                  *(keybuffer+1) == 'E' )    keywordcode = RESET;
        else if ( *keybuffer == 'D' &&
                  *(keybuffer+1) == 'R' )    keywordcode = DISREGS;
        else if ( *keybuffer == 'D' &&
                  *(keybuffer+1) == 'D' )    keywordcode = DDMEM;
        else if ( *keybuffer == 'D' &&
                  *(keybuffer+1) == 'C' )    keywordcode = DCMEM;
        else if ( *keybuffer == 'S' &&
                  *(keybuffer+1) == 'P' )    keywordcode = SETPC;
        else if ( *keybuffer == 'S' &&
                  *(keybuffer+1) == 'F' )    keywordcode = SETFP;
        else if ( *keybuffer == 'S' &&
                  *(keybuffer+1) == 'S' )    keywordcode = SETSP;
        else if ( *keybuffer == 'S' &&
                  *(keybuffer+1) == 'D' )    keywordcode = SETDM;
        else if ( *keybuffer == 'S' &&
                  *(keybuffer+1) == 'B' )    keywordcode = SETBP;
        else if ( *keybuffer == 'D' &&
                  *(keybuffer+1) == 'B' )    keywordcode = SHOWBP;
        else if ( *keybuffer == 'C' &&
                  *(keybuffer+1) == 'B' )    keywordcode = CLEARBP;
        else  {
            DisplayError( !FATAL, "Unknown Command \"%s\"\n", keybuffer );
            keywordcode = ERROR;
        }
    }
    return  keywordcode;
}

static void DisplayHelp( void )
{
    WriteOut( " ?, HELP          Display this message\n" );
    WriteOut( " EXIT             Exit program\n" );
    WriteOut( " LOGGING          Toggle logging to file \"%s\"\n", LOGFILE );
    WriteOut( " RUN [<c>]        Run <c> instructions, starting from the\n" );
    WriteOut( "                  current PC\n" );
    WriteOut( " TRACE [<c>]      Trace <c> instructions, starting from the\n" );
    WriteOut( "                  current PC\n" );
    WriteOut( " RESET            PC = 0, SP = -1, FP = 0\n" );
    WriteOut( "                  Initialise stack to zeros\n" );
    WriteOut( " DR               Display Registers\n" );
    WriteOut( " DDM [<a> [<c>]]  Display data (stack) memory from <a>\n");
    WriteOut( "                  displaying <c> words\n" );
    WriteOut( " DCM [<a> [<c>]]  Display code (instruction) memory\n" );
    WriteOut( "                  from <a> displaying <c> words\n" );
    WriteOut( " SPC <n>          Set Program Counter to <n>\n" );
    WriteOut( " SSP <n>          Set Stack Pointer to <n>\n" );
    WriteOut( " SFP <n>          Set Frame Pointer to <n>\n" );
    WriteOut( " SDM <a> <v>      Set Data memory at <a> to <v>\n\n" );
    WriteOut( " SBP <a>          Set Breakpoint at code address <a>\n" );
    WriteOut( " DBP              Display all Breakpoints\n" );
    WriteOut( " CBP              Clear all Breakpoints\n\n" );
    WriteOut( " Commands may be abbreviated to shortest unique prefix\n");
}
