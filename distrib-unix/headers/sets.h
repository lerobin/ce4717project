#ifndef  SETSHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      sets.h                                                               */
/*                                                                           */
/*      Header file for "sets.c", containing constant declarations, type     */
/*      definitions and function prototypes for the set manipulation         */
/*      routines.                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  SETSHEADER

#include <limits.h>
#include "global.h"

#define  SET_SIZE               64

#if ( INT_MAX == 32767 )
#define  BITS_PER_WORD          16      /* 16 bit word machine, ibm pc       */
#else
#define  BITS_PER_WORD          32      /* assume 32 bit word if not 16      */
#endif

#define  WORDS_PER_SET          (SET_SIZE/BITS_PER_WORD)

typedef  struct  {
    unsigned  word[WORDS_PER_SET];
}
    SET;

PUBLIC SET   *MakeSet( void );
PUBLIC void  ClearSet( SET *setptr );
PUBLIC void  AddElements( SET *setptr, int ElementCount, ... );
PUBLIC void  InitSet( SET *setptr, int ElementCount, ... );
PUBLIC void  AddElement( SET *setptr, int Element );
PUBLIC void  RemoveElement( SET *setptr, int Element );
PUBLIC int   InSet( SET *setptr, int Element );
PUBLIC SET   Union( int SetCount, ... );
PUBLIC SET   Intersection( int SetCount, ... );
#endif
