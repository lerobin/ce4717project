/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      strtab.c                                                             */
/*                                                                           */
/*      Implementation file for the string table.                            */
/*                                                                           */
/*      The string table is a centralised storage mecahnism for the strings  */ 
/*      belonging to all the symbols handled by the program. It is           */ 
/*      implemented as a number of "chunks" or blocks of storage. When a     */ 
/*      chunk is filled up by strings, a new chunk is dynamically allocated  */ 
/*      from store.                                                          */
/*                                                                           */ 
/*      Four routines are provided by this module.                           */
/*                                                                           */ 
/*          "NewString"  -- this is used to prepare the string table to      */
/*          accept a new string. It must be called before any of the other   */
/*          string table routines defined in this module.                    */
/*                                                                           */ 
/*          "AddChar" -- this is the routine which builds a new string       */
/*          character by character in the string table. Note, it is          */
/*          important to remember to null terminate each string by           */
/*          explicitly calling "AddChar( '\0' );" after all the other        */
/*          characters in the string have been entered. Most other           */
/*          routines will assume this C convention, but it is up to the      */
/*          user to ensure that the strings in the string table are          */
/*          correctly terminated.                                            */
/*                                                                           */ 
/*          "GetString" -- this routine returns a pointer to the string      */ 
/*          most recently added to the table.                                */ 
/*                                                                           */ 
/*          "PreserveString" -- this is used to preserve the most recently   */
/*          entered string in the table. If this is not called, the string   */
/*          subsequently overwritten. The idea behind this is to only        */
/*          preserve the string if it represents, for example, a new         */
/*          symbol.                                                          */
/*                                                                           */ 
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "strtab.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Definitions of constants local to the module                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  CHUNKSIZE                     1024   /* see AddChar                 */

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Data Structures for this module                                      */
/*                                                                           */
/*      The string table is maintained in blocks, called "chunks", which     */
/*      are dynamically allocated from store as needed. The pointer          */
/*      "TopOfTable" points to a position just beyond the last string        */
/*      which has been presered in the current chunk. The pointer            */
/*      "InsertionPoint" points to the next location where a character is    */
/*      to be inserted in the string currently being assembled in the chunk. */
/*      The integer "SpaceLeftInChunk" gives how many characters are left    */
/*      in the current chunk.                                                */
/*                                                                           */
/*      For Example, if the chunk size is 32, a typical arrangement might    */
/*      be:                                                                  */
/*                                                                           */
/*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    */
/*      |s|t|r|i|n|g|1| |s|t|r|i|n|g|2| |c|u|r|r| | | | | | | | | | | | |    */
/*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    */
/*                                     ^         ^                           */
/*                                     |         |                           */
/*                                TopOfTable    InsertionPoint               */
/*                                                                           */
/*                                SpaceLeftInChunk = 12                      */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE char  *TopOfTable = NULL; 
PRIVATE char  *InsertionPoint = NULL;
PRIVATE int   SpaceLeftInChunk = 0;

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      NewString                                                            */
/*                                                                           */
/*      Resets the "InsertionPoint" to the current "TopOfTable".             */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   NewString( void )
{
    if ( SpaceLeftInChunk <= 0 )  {
        if ( NULL == ( TopOfTable = malloc( CHUNKSIZE ) ) )  {
            fprintf( stderr, "Error, \"NewString\", malloc failure\n" );
            exit( EXIT_FAILURE );
        }
        SpaceLeftInChunk = CHUNKSIZE;
    }
    else  SpaceLeftInChunk += (int)(InsertionPoint-TopOfTable);
    InsertionPoint = TopOfTable;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      AddChar                                                              */
/*                                                                           */
/*      Inserts a new character into the string table at the current         */
/*      insertion point and increments this pointer. If the current chunk    */
/*      contains no more space, allocates a new chunk and copies the         */
/*      partially entered string to this new chunk, updating pointers        */
/*      appropriately.                                                       */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   AddChar( int ch )
{
    char *NewChunk, *p;

    if ( SpaceLeftInChunk <= 1 )  {
        if ( NULL == ( NewChunk = p  = malloc( CHUNKSIZE ) ) )  {
            fprintf( stderr, "Error, \"AddChar\", malloc failure\n" );
            exit( EXIT_FAILURE );
        }
        SpaceLeftInChunk = CHUNKSIZE;
        while ( TopOfTable != InsertionPoint )  {
            *p++ = *TopOfTable++;
            SpaceLeftInChunk--;
        }
        TopOfTable = NewChunk;
        InsertionPoint = p;
    }
    *InsertionPoint++ = (char)( ch & 0x7f );  
    SpaceLeftInChunk--;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetString                                                            */
/*                                                                           */
/*      Returns the current "TopOfTable" pointer, this being the pointer     */
/*      to the string most recently entered in the table. Should be called   */
/*      before "PreserveString" is called, as the latter routine modifies    */
/*      "TopOfTable" to point beyond the end of this string, and makes the   */
/*      "most recently entered string" an undefined quantity.                */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Pointer to character representing the address of      */
/*                     the most recently entered string.                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC char   *GetString( void )
{
    return TopOfTable;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      PreserveString                                                       */
/*                                                                           */
/*      "Fix" or "freeze" the string into the string table so that a call    */
/*      to "NewString", followed by subsequent calls to "AddChar" does not   */
/*      cause the string to be overwritten. Note, this makes the concept     */
/*      "most recently entered string" undefined.                            */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   PreserveString( void )
{
    TopOfTable = InsertionPoint;
}
