/*

        simulate.c

        file containing routines to implement the instruction set 
        interpreter.

*/

#define   SIMULATEMODULE
#include  <stdio.h>
#ifdef APOLLO
#include  "stdlib.h"
#else
#include  <stdlib.h>
#endif
#include  "simulate.h"
#include  "sdis.h"
#include  "sim.h"

static void AddressError( char *ErrorString );
static int  CodeAddr( int addr );
static int  DataAddr( int addr );
static void Push( int value );
static int  Pop( void );
static int  Decrement( int currentSP, int amount );
static void BinaryOp( int opcode );
static int  Branch( int opcode );
static void BuildStackFrame( void );
static void RemoveStackFrame( void );
static void LoadDisplayPointer( void );
static void RestoreDisplayPointer( void );
static void ReadInteger( void );
static void WriteInteger( void );

void  Interpret( void )
{
    INSTRUCTION current;
    int addr, nextaddr;

    current = Pmem[PC];
    nextaddr = PC+1;
    switch ( current.opcode )  {
        case  HALT:    WriteOut( "!! HALT encountered\n\n" );
                       Running = 0;                             break;
        case  ADD:     BinaryOp( ADD );                         break;
        case  SUB:     BinaryOp( SUB );                         break;
        case  MULT:    BinaryOp( MULT );                        break;
        case  DIV:     BinaryOp( DIV );                         break;
        case  NEG:     Dmem[SP] = -Dmem[SP];                    break;
        case  BR:      nextaddr = Branch( BR );                 break;
        case  BGZ:     nextaddr = Branch( BGZ );                break;
        case  BG:      nextaddr = Branch( BG );                 break;
        case  BLZ:     nextaddr = Branch( BLZ );                break;
        case  BL:      nextaddr = Branch( BL );                 break;
        case  BZ:      nextaddr = Branch( BZ );                 break;
        case  BNZ:     nextaddr = Branch( BNZ );                break;
        case  CALL:    Push( nextaddr );
                       nextaddr = CodeAddr( current.offset );   break;
        case  RET:     nextaddr = CodeAddr( Pop() );            break;
        case  BSF:     BuildStackFrame();                       break;
        case  RSF:     RemoveStackFrame();                      break;
        case  LDP:     LoadDisplayPointer();                    break;
        case  RDP:     RestoreDisplayPointer();                 break;
        case  INC:     SP = DataAddr( SP + current.offset );    break;
        case  DEC:     SP = Decrement( SP, current.offset );    break;    
        case  PUSHFP:  Push( FP );                              break;
        case  LOADI:   Push( current.offset );                  break;
        case  LOADA:   addr = DataAddr( current.offset );
                       Push( Dmem[addr] );                      break;
        case  LOADFP:  addr = DataAddr( FP + current.offset );
                       Push( Dmem[addr] );                      break;
        case  LOADSP:  addr = DataAddr( Dmem[SP] + current.offset );
                       Dmem[SP] = Dmem[addr];                   break;
        case  STOREA:  addr = DataAddr( current.offset );
                       Dmem[addr] = Pop();                      break;
        case  STOREFP: addr = DataAddr( FP + current.offset );
                       Dmem[addr] = Pop();                      break;
        case  STORESP: addr = DataAddr( Pop() );
                       addr = DataAddr( addr + current.offset );
                       Dmem[addr] = Pop();                      break;
        case  READ:    ReadInteger();                           break;
        case  WRITE:   WriteInteger();                          break;
        default:
            DisplayError( !FATAL, "Error, unknown opcode encountered\n" );
            DisplayError( FATAL,  "PC = %d, SP = %d, FP = %d, opcode = %d\n",
                          PC, SP, FP, current.opcode );
            break;
    }
    PC = nextaddr;
}

static void AddressError( char *ErrorString )
{
    char buffer[80];

    Disassemble( Pmem+PC, buffer );
    DisplayError( !FATAL, "Error: %s, PC = %d, SP = %d, FP = %d\n", 
                  ErrorString, PC, SP, FP );
    DisplayError( !FATAL, "       Instruction \"%s\"\n", buffer );
}

static int  CodeAddr( int addr )
{
    char buffer[80];

    if ( addr < 0 || addr > CODESIZE )  {
        snprintf( buffer, sizeof(buffer), "Illegal code address %d", addr );
        AddressError( buffer );
        exit( EXIT_FAILURE );
    }
    return addr;
}

static int  DataAddr( int addr )
{
    char buffer[80];

    if ( addr < 0 || addr > STACKSIZE )  {
        snprintf( buffer, sizeof(buffer), "Illegal data address %d", addr );
        AddressError( buffer );
        exit( EXIT_FAILURE );
    }
    return addr;
}

static void Push( int value )
{
    if ( SP >= STACKSIZE-1 )  {
        AddressError( "Stack Overflow" );
        exit( EXIT_FAILURE );
    }
    else  {
        SP++;
        Dmem[SP] = value;
    }
}

static int  Pop( void )
{
    int temp;

    if ( SP < 0 )  {
        AddressError( "Stack Underflow" );
        exit( EXIT_FAILURE );
    }
    else  {
        temp = Dmem[SP];
        SP--;
    }
    return temp;
}

static int  Decrement( int currentSP, int amount )
{
    int newSP;
    char buffer[80];

    newSP = currentSP - amount;
    if ( newSP < -1 ) {
        snprintf( buffer, sizeof(buffer), "Illegal stack pointer address %d", newSP );
        AddressError( buffer );
        exit( EXIT_FAILURE );
    }
    return newSP;
}

static void BinaryOp( int opcode )
{
    int tos, tos_m1;

    tos = Pop();  tos_m1 = Pop();
    switch ( opcode )  {
        case ADD   :  tos_m1 += tos;                            break;
        case SUB   :  tos_m1 -= tos;                            break;
        case MULT  :  tos_m1 *= tos;                            break;
        case DIV   :  tos_m1 /= tos;                            break;
        default:
            DisplayError( !FATAL, "Error, illegal opcode in BinaryOp, %d\n",
                          opcode );
            break;
    }
    Push( tos_m1 );
}

static int  Branch( int opcode )
{
    int flag = 0, /* This initialisation required by Microsoft CL at /W4 */
        nextaddr;

    nextaddr = Pmem[PC].offset;
    if ( opcode != BR )  flag = Pop();
    switch ( opcode )  {
        case  BR  :                                             break;
        case  BGZ : if ( flag <  0 )  nextaddr = PC+1;          break;
        case  BG  : if ( flag <= 0 )  nextaddr = PC+1;          break;
        case  BLZ : if ( flag >  0 )  nextaddr = PC+1;          break;
        case  BL  : if ( flag >= 0 )  nextaddr = PC+1;          break;
        case  BZ  : if ( flag != 0 )  nextaddr = PC+1;          break;
        case  BNZ : if ( flag == 0 )  nextaddr = PC+1;          break;
        default:
            DisplayError( !FATAL, "Error, illegal opcode in Branch, %d\n",
                          opcode );
            break;
    }
    return  nextaddr;
}

static void BuildStackFrame( void )
{
    int temp, oldsp;

    temp = FP;
    oldsp = SP;
    Push( temp );
    FP = oldsp;
}

static void RemoveStackFrame( void )
{
    int addr;

    addr = DataAddr( FP );
    SP   = addr - 1;
    addr = DataAddr( FP + 1 );
    FP   = DataAddr( Dmem[ addr ] );
}

static void LoadDisplayPointer( void )
{
    int addr, temp;

    addr = DataAddr( Pmem[PC].offset );
    temp = Dmem[ addr ];
    Dmem[ DataAddr( FP ) ] = temp;
    Dmem[ addr ] = FP;
}

static void RestoreDisplayPointer( void )
{
    int addr;

    addr = DataAddr( Pmem[PC].offset );
    Dmem[ addr ] = Dmem[ DataAddr( FP ) ];
}

static void ReadInteger( void )
{
    char *buffer;
    int value;

    WriteOut( "\n!! Read instruction, Enter integer --> " );
    buffer = ReadString();
    while ( 1 != sscanf( buffer, " %d", &value ) )  {
        WriteOut( "   Invalid integer, try again --> " );
        buffer = ReadString();
    }
    Push( value );
}

static void WriteInteger( void )
{
    WriteOut( "\n!! Write instruction, Top of Stack --> \"%d\"\n\n", Pop() );
}
