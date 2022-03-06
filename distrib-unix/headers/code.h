#ifndef  CODEHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      code.h                                                               */
/*                                                                           */
/*      Header file for "code.c", containing constant declarations, type     */
/*      definitions and function prototypes for code generator.              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  CODEHEADER

#include <stdio.h>
#include "global.h"

#define  I_ADD           0      /* 0-"address" instructions                  */
#define  I_SUB           1      /* Sub                                       */
#define  I_MULT          2      /* Mult                                      */
#define  I_DIV           3      /* Div                                       */
#define  I_NEG           4      /* Neg                                       */
#define  I_RET           5      /* Ret                                       */
#define  I_BSF           6      /* Bsf                                       */
#define  I_RSF           7      /* Rsf                                       */
#define  I_PUSHFP        8      /* Push FP                                   */
#define  I_READ          9      /* Read                                      */
#define  I_WRITE        10      /* Write                                     */
#define  I_HALT         11      /* Halt                                      */

#define  I_BR           12      /* 1-"address" instructions                  */
#define  I_BGZ          13      /* Bgz  <addr>                               */
#define  I_BG           14      /* Bg   <addr>                               */
#define  I_BLZ          15      /* Blz  <addr>                               */
#define  I_BL           16      /* Bl   <addr>                               */
#define  I_BZ           17      /* Bz   <addr>                               */
#define  I_BNZ          18      /* Bnz  <addr>                               */
#define  I_CALL         19      /* Call <addr>                               */
#define  I_LDP          20      /* Ldp  <addr>                               */
#define  I_RDP          21      /* Rdp  <addr>                               */
#define  I_INC          22      /* Inc  <words>                              */
#define  I_DEC          23      /* Dec  <words>                              */
#define  I_LOADI        24      /* Load #<datum>                             */
#define  I_LOADA        25      /* Load <addr>                               */
#define  I_LOADFP       26      /* Load FP+<offset>                          */
#define  I_LOADSP       27      /* Load [SP]+<offset>                        */
#define  I_STOREA       28      /* Store <addr>                              */
#define  I_STOREFP      29      /* Store FP+<offset>                         */
#define  I_STORESP      30      /* Store [SP]+<offset>                       */

PUBLIC void   InitCodeGenerator( FILE *codefile );
PUBLIC void   WriteCodeFile( void );
PUBLIC void   KillCodeGeneration( void );
PUBLIC void   Emit( int opcode, int offset );
PUBLIC int    CurrentCodeAddress( void );
PUBLIC void   BackPatch( int codeaddr, int value );

#define _Emit(opcode)  Emit((opcode),0)
#endif
