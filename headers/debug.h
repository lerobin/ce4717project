#ifndef  DEBUGHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      debug.h                                                              */
/*                                                                           */
/*      Some routines to aid in debugging:                                   */
/*                                                                           */
/*                                                                           */
/*      TokenCode2Str: Convert a integer representing a TOKEN's code to its  */
/*                     string representation.                                */
/*                                                                           */
/*      PrintToken:    Display a string representation of a TOKEN's ".code"  */
/*                     field on the standard output.                         */
/*                                                                           */
/*      TokenSet2Str:  Convert a SET of TOKEN codes to a string              */
/*                     representation.                                       */
/*                                                                           */
/*      PrintTokenSet: Display a SET of TOKEN codes on the standard output.  */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  DEBUGHEADER
#include "global.h"
#include "sets.h"
#include "scanner.h"
#include "stdio.h"

PUBLIC char  *TokenCode2Str( int TokenCode );
PUBLIC void   PrintTokenCode( int TokenCode );
PUBLIC char  *TokenSet2Str( SET TokenCodes );
PUBLIC void   PrintTokenSet( SET TokenCodes );

#endif
