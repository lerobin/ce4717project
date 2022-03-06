/*

        sparse.c

        Scanner and parser routines for the stack machine simulator

*/

#include  <stdio.h>
#ifdef APOLLO
#include  "stdlib.h"
#else
#include  <stdlib.h>
#endif
#include  <string.h>
#include  <ctype.h>
#include  "sparse.h"
#include  "sim.h"

#define   INTCONST      -1      /* negative so as not to be confused    */
#define   ERROR         -2      /* with the opcode definitons in sim.h  */
#define   ENDLINE       -3
#define   ENDFILE       -4
#define   LOAD          -5      /* one of the 4 types of load inst      */
#define   STORE         -6      /* one of the 3 types of store          */
#define   PLUS          -7      /* '+' in assembly instruction          */
#define   MINUS         -8      /* '-'                                  */
#define   IMMEDIATE     -9      /* '#'                                  */
#define   KEYWORD      -10      /* An instruction  mnemonic or FP       */
#define   FP           -11      /* "FP"                                 */
#define   SP           -12      /* "[SP]"                               */

 
static FILE *InputFile = NULL;
static int   pushback  = 0;
static int   pbtoken   = 0;
static int   pboffset  = 0;


static int   GetToken( int *offset );
static void  UnGetToken( int token, int offset );
static void  ReadAddress( INSTRUCTION *instruction );
static void  ReadNatural( INSTRUCTION *instruction );
static void  ReadLoadStore( INSTRUCTION *instruction, int class );
static void  ReadOffset( INSTRUCTION *instruction );

void  InitParsing( FILE *inputfile )
{
    InputFile = inputfile;
}

/*
        Returns 1 if an instruction is found, 0 on end of file.
*/

int   ReadInstruction( INSTRUCTION *Instruction )
{
    int token, offset, status = 0; /* This initialisation required by Microsoft CL at W4 */

    token = GetToken( &offset );

    while ( token == ENDLINE )          /* scan off any blank lines     */
        token = GetToken( &offset );
    if ( token == INTCONST )  /* allow an instruction address at the    */
        token = GetToken( &offset );    /* beginning of the line        */

    switch ( token )  {
        case  ENDFILE:  status = 0;                             break;
        case  HALT:
        case  ADD:
        case  SUB:
        case  MULT:
        case  DIV:
        case  NEG:
        case  RET:
        case  BSF:
        case  RSF:
        case  PUSHFP:
        case  READ:
        case  WRITE:    Instruction->opcode = token;
                        Instruction->offset = 0;
                        status = 1;                             break;
        case  BR:
        case  BGZ:
        case  BG:
        case  BLZ:
        case  BL:
        case  BZ:
        case  BNZ:
        case  LDP:
        case  RDP:
        case  CALL:     Instruction->opcode = token;
                        ReadAddress( Instruction );
                        status = 1;                             break;
        case  INC:
        case  DEC:      Instruction->opcode = token;
                        ReadNatural( Instruction );
                        status = 1;                             break;
        case  LOAD:     ReadLoadStore( Instruction, LOAD );
                        status = 1;                             break;
        case  STORE:    ReadLoadStore( Instruction, STORE );
                        status = 1;                             break;
        case  ERROR:    DisplayError( FATAL, "Error, invalid token\n" );
                        break;
        default:        DisplayError( !FATAL, "Error, unknown token read  " );
                        DisplayError( FATAL,  "%d\n", token );
                        break;
    }

    while ( token != ENDLINE && token != ENDFILE )  /* scan to EOL      */
        token = GetToken( &offset );

    return status;
}

static int   GetToken( int *offset )
{
    int ch, token = 0, i = 0, state = 0, quit = 0, intvalue = 0;
    char buffer[30];

    if ( pushback )  {
        *offset = pboffset;  pushback = 0;  return  pbtoken;
    }

    while ( !quit )  {
        ch = fgetc( InputFile );
        switch ( state )  {
            case  0 :
                if ( ch == '\n' )  {
                    quit = 1;  token = ENDLINE;
                }
                else if ( ch == ';' )  state = 7;
                else if ( isspace( ch ) )  state = 0;
                else if ( isalpha( ch ) )  {
                    *(buffer+i) = (char)toupper( ch );  i++;
                    state = 1;
                }
                else if ( isdigit( ch ) )  {
                    intvalue *= 10;  intvalue += ( ch - '0' );
                    state = 2;
                }
                else if ( ch == '+' )  {
                    quit = 1;  token = PLUS;
                }
                else if ( ch == '-' )  {
                    quit = 1;  token = MINUS;
                }
                else if ( ch == '#' )  {
                    quit = 1;  token = IMMEDIATE;
                }
                else if ( ch == '[' )  {
                    state = 3;
                }
                else if ( ch == EOF )  {
                    quit = 1;  token = ENDFILE;
                }
                else  {
                    quit = 1;  token = ERROR;
                }
                break;
            case  1 :
                if ( isalpha( ch ) )  {
                    if ( i < 29 )  {
                        *(buffer+i) = (char)toupper( ch ); i++;
                    }
                }
                else  {
                    ungetc( ch, InputFile );  
                    *(buffer+i) = '\0';
                    token = KEYWORD;
                    quit = 1;
                }
                break;
            case  2 :
                if ( isdigit( ch ) )  {
                    intvalue *= 10;  intvalue += ( ch - '0' );
                }
                else  {
                    ungetc( ch, InputFile );  
                    token = INTCONST;
                    quit = 1;
                }
                break;
            case  3 :
                ch = toupper( ch );
                if ( ch == 'S' )  state = 4;  else  state = 6;
                break;
            case  4 :
                ch = toupper( ch );
                if ( ch == 'P' )  state = 5;  else  state = 6;
                break;
            case  5 :
                if ( ch == ']' )  {
                    token = SP;
                    quit = 1;
                }
                else  state = 6;
                break;
            case  6 :
                DisplayError( FATAL, "Error, illegal token scanned\n" );
                break;
            case  7 :
                if ( ch == '\n' )  {
                    token = ENDLINE;
                    quit = 1;
                }
                else if ( ch == EOF )  {
                    token = ENDFILE;
                    quit = 1;
                }
                else  state = 7;
                break;
            default :
                DisplayError( FATAL, "Error, illegal scanner state\n" );
                break;
        }
    }
    if ( token == KEYWORD )  {
        /* printf( "DEBUG: system has a keyword \"%s\"\n", buffer ); */
        if ( 0 == strncmp( buffer, "HALT", 4 ) )                     token = HALT;
        else if ( 0 == strncmp( buffer, "ADD", 3 ) )    token = ADD;
        else if ( 0 == strncmp( buffer, "SUB", 3 ) )    token = SUB;
        else if ( 0 == strncmp( buffer, "MULT", 4 ) )   token = MULT;
        else if ( 0 == strncmp( buffer, "DIV", 3 ) )    token = DIV;
        else if ( 0 == strncmp( buffer, "NEG", 3 ) )    token = NEG;
        else if ( 0 == strncmp( buffer, "BR", 2 ) )     token = BR;
        else if ( 0 == strncmp( buffer, "BGZ", 3 ) )    token = BGZ;
        else if ( 0 == strncmp( buffer, "BG", 2 ) )     token = BG;
        else if ( 0 == strncmp( buffer, "BLZ", 3 ) )    token = BLZ;
        else if ( 0 == strncmp( buffer, "BL", 2 ) )     token = BL;
        else if ( 0 == strncmp( buffer, "BZ", 2 ) )     token = BZ;
        else if ( 0 == strncmp( buffer, "BNZ", 3 ) )    token = BNZ;
        else if ( 0 == strncmp( buffer, "CALL", 4 ) )   token = CALL;
        else if ( 0 == strncmp( buffer, "RET", 3 ) )    token = RET;
        else if ( 0 == strncmp( buffer, "BSF", 3 ) )    token = BSF;
        else if ( 0 == strncmp( buffer, "RSF", 3 ) )    token = RSF;
        else if ( 0 == strncmp( buffer, "LDP", 3 ) )    token = LDP;
        else if ( 0 == strncmp( buffer, "RDP", 3 ) )    token = RDP;
        else if ( 0 == strncmp( buffer, "INC", 3 ) )    token = INC;
        else if ( 0 == strncmp( buffer, "DEC", 3 ) )    token = DEC;
        else if ( 0 == strncmp( buffer, "PUSH", 4 ) )   token = PUSHFP;
        else if ( 0 == strncmp( buffer, "LOAD", 4 ) )   token = LOAD;
        else if ( 0 == strncmp( buffer, "STORE", 5 ) )  token = STORE;
        else if ( 0 == strncmp( buffer, "READ", 4 ) )   token = READ;
        else if ( 0 == strncmp( buffer, "WRITE", 5 ) )  token = WRITE;
        else if ( 0 == strncmp( buffer, "FP", 2 ) )     token = FP;
        else  DisplayError( FATAL, "Unknown string \"%s\"\n", buffer );
    }
    *offset = intvalue;
    return token;
}

static void  UnGetToken( int token, int offset )
{
    pbtoken  = token;
    pboffset = offset;
    pushback = 1;
}
        
static void  ReadAddress( INSTRUCTION *instruction )
{
    int token, address;

    token = GetToken( &address );

    if ( token != INTCONST )  {
        DisplayError( FATAL, "Positve integer expected as address\n" );
    }
    else if ( address < 0 || address >= CODESIZE )  {
        DisplayError( FATAL, "Target address out of range in branch or call\n" );
    }
    else  instruction->offset = address;
}

static void  ReadNatural( INSTRUCTION *instruction )
{
    int token, natural;

    token = GetToken( &natural );

    if ( token != INTCONST )  {
        DisplayError( FATAL, "Positve integer expected as argument of INC or DEC\n" );
    }
    else if ( natural < 0 || natural >= CODESIZE )  {
        DisplayError( FATAL, "Argument of INC or DEC out of range\n" );
    }
    else  instruction->offset = natural;
}

static void  ReadLoadStore( INSTRUCTION *instruction, int class )
{
    int token, branch, negative;

    token = GetToken( &branch );

    if ( token == FP )  {
        if ( class == LOAD )  instruction->opcode = LOADFP;
        else  instruction->opcode = STOREFP;
        ReadOffset( instruction );
    }
    else if ( token == IMMEDIATE )  {
        if ( class == LOAD )  {
            instruction->opcode = LOADI;
            token = GetToken( &branch );
            if ( token != INTCONST && token != PLUS && token != MINUS )  {
                DisplayError( FATAL, "Reading LOAD #<datum>, integer expected\n" );
            }
            else  {
                if ( token == PLUS || token == MINUS )  {
		    if ( token == MINUS )  negative = 1; else negative = 0;
                    token = GetToken( &branch );
                    if ( token != INTCONST )  {
                        DisplayError( FATAL, 
                                 "Reading LOAD #<datum>, integer expected\n" );
                    }
                    if ( negative )  branch = -branch;
                }
                instruction->offset = branch;
            }
        }
        else  DisplayError( FATAL, "Immediate mode illgal with STORE\n" );
    }
    else if ( token == INTCONST )  {
        if ( class == LOAD )  instruction->opcode = LOADA;
        else  instruction->opcode = STOREA;
        if ( branch < 0 || branch >= STACKSIZE )  {
            DisplayError( FATAL, "Data address out of range\n" );
        }
        else instruction->offset = branch;
    }
    else if ( token == SP )  {
        if ( class == LOAD )  instruction->opcode = LOADSP;
        else  instruction->opcode = STORESP;
        ReadOffset( instruction );
    }
    else  {
        DisplayError( FATAL, "Unknown form of LOAD or STORE encountered\n" );
    }
}

static void  ReadOffset( INSTRUCTION *instruction )
{
    int token, negative, branch;

    token = GetToken( &branch );
    if ( token == PLUS || token == MINUS )  {
        if ( token == MINUS )  negative = 1; else negative = 0;
        token = GetToken( &branch );
        if ( token == INTCONST )  {
            if ( negative )  instruction->offset = -branch;
            else  instruction->offset = branch;
        }
        else  {
            DisplayError( FATAL, "Reading offset value, integer expected\n" );
        }
    }
    else if ( token == ENDLINE || token == ENDFILE )  {
        UnGetToken( token, branch );
        instruction->offset = 0;
    }
    else  {
        DisplayError( FATAL, "Reading offset value, + or - expected\n" );
    }
}

