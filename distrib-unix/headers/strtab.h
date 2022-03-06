#ifndef  STRINGTABLEHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      strtab.h                                                             */
/*                                                                           */
/*      Header file for "strtab.c", containing constant declarations,        */
/*      type definitions and function prototypes for the string table        */
/*      routines.                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  STRINGTABLEHEADER

#include "global.h"

PUBLIC void   NewString( void );
PUBLIC void   AddChar( int ch );
PUBLIC char   *GetString( void );
PUBLIC void   PreserveString( void );
#endif
