#ifndef  SYMBOLHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      symbol.h                                                             */
/*                                                                           */
/*      Header file for "symbol.c", containing constant declarations,        */
/*      type definitions and function prototypes for the symbol table        */
/*      routines.                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  SYMBOLHEADER

#include "global.h"

#define  HASHSIZE       997     /* Should be a prime for efficient hashing.  */
#define  MAXHASHLENGTH  100     /* Maximum number of characters taken into   */
                                /* account in a string when hashing.         */

#define  STYPE_PROGRAM    1     /* SYMBOL type for the program name.         */
#define  STYPE_VARIABLE   2     /* SYMBOL type for global variables.         */
#define  STYPE_PROCEDURE  3     /* SYMBOL type for procedures.               */
#define  STYPE_FUNCTION   4     /*                 functions.                */
#define  STYPE_LOCALVAR   5     /*                 local variables.          */
#define  STYPE_VALUEPAR   6     /*                 value parameters.         */
#define  STYPE_REFPAR     7     /*                 ref parameters.           */


typedef struct symboltype  {
    char *s;                    /* character string name of symbol           */
    int  scope;                 /* scope level of symbol                     */
    int  type;                  /* type of symbol (use one of the STYPEs)    */
    int  pcount;                /* if the symbol is a procedure, pcount      */
    int  ptypes;                /* is the number of parameters of the symbol */
                                /* and ptypes contains parameter types       */
    int  address;               /* address or offset of symbol               */
    struct symboltype *next;    /* pointer to next symbol in chain           */
}
    SYMBOL;

PUBLIC SYMBOL *Probe( char *String, int *hashindex );
PUBLIC SYMBOL *EnterSymbol( char *String, int hashindex );
PUBLIC void   DumpSymbols( int scope );
PUBLIC void   RemoveSymbols( int scope );

#endif
