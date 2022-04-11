/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      sets.c                                                               */
/*                                                                           */
/*      Implementation file for the set manipulation routines.               */
/*                                                                           */
/*      A set is organised as an array of 1 bit values. Each of these acts   */
/*      to indicate the presence or absence of a particular value in the     */
/*      set. Thus, if a set contains elements 7 and 23, then bits 7 and 32   */
/*      that set's bit array will be set to '1's and all the other bits its  */
/*      array will be cleared to '0's. The implementation is not fully       */
/*      general, it is designed to handle sets of small positive values,     */
/*      between 0 and SET_SIZE-1, such as TOKEN values or character codes.   */
/*                                                                           */
/*      The set implementation is designed to facilitate the speedy          */
/*      processing of set operations using the ability of the underlying     */
/*      hardware to manipulate bits in 32 or 64 element units (the natural   */
/*      word size of whatever machine the program is running on). Hence,     */
/*      the bit array is packed as densely as possible into machine words.   */
/*      On a 32-bit machine the set is organised into 32-element chunks,     */
/*      each being held in a machine word. On bigger machines 64 or 128      */
/*      1-bit set elements are packed in each word.                          */
/*                                                                           */ 
/*      For example, here is a set with SET_SIZE = 32, so it may hold        */
/*      elements from 0 to 31. It contains elements 3 and 14.                */
/*                                                                           */ 
/*                           1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3     */ 
/*       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1     */ 
/*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    */
/*      |0|0|0|1|0|0|0|0|0|0|0|0|0|0|1|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|0|    */
/*      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    */
/*                                                                           */ 
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sets.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      MakeSet                                                              */
/*                                                                           */
/*      Generates an empty set and returns a pointer to it. Space for the    */
/*      set is generated using malloc.                                       */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:                                                             */
/*                                                                           */
/*          Pointer to the set which has been generated.                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC SET   *MakeSet( void )
{
    SET    *setptr;

    if ( NULL == ( setptr = malloc( sizeof( SET ) ) ) )  {
        fprintf( stderr, "MakeSet: malloc failure\n" );
        exit( EXIT_FAILURE );
    }
    ClearSet( setptr );
    return  setptr;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      ClearSet                                                             */
/*                                                                           */
/*      Takes an already existing set and turns it into the empty set.       */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to be cleared.                         */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void  ClearSet( SET *setptr )
{
    int i;

    if ( setptr != NULL )  {
        for ( i = 0; i < WORDS_PER_SET; i++ )  *((setptr->word)+i) = 0;
    }
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      AddElements                                                          */
/*                                                                           */
/*      Add a number of elements to a set. This routine works by calling     */
/*      AddElement, which adds a single element.                             */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to which the elements are to be added  */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          ElementCount  Number of elements to be added                     */
/*                                                                           */
/*          ...        Variable number of integer parameters which will be   */
/*                     handled by using C's facility for handling varying    */
/*                     numbers of arguments. This facility is quite fragile  */
/*                     so it is important to ensure that ElementCount is     */
/*                     correct and that all the parameters are integers.     */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void  AddElements( SET *setptr, int ElementCount, ... )
{
    va_list ap;
    int     i, element;

    va_start(ap, ElementCount);
    for ( i = 0; i < ElementCount; i++ )  {
        element = va_arg(ap, int);
        AddElement( setptr, element );
    }
    va_end(ap);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      InitSet                                                              */
/*                                                                           */
/*      Does the same job as AddElements, but clears the set first.          */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to which the elements are to be added  */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          ElementCount  Number of elements to be added                     */
/*                                                                           */
/*          ...        Variable number of integer parameters which will be   */
/*                     handled by using C's facility for handling varying    */
/*                     numbers of arguments. This facility is quite fragile  */
/*                     so it is important to ensure that ElementCount is     */
/*                     correct and that all the parameters are integers.     */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void  InitSet( SET *setptr, int ElementCount, ... )
{
    va_list ap;
    int     i, element;

    ClearSet( setptr );
    va_start(ap, ElementCount);
    for ( i = 0; i < ElementCount; i++ )  {
        element = va_arg(ap, int);
        AddElement( setptr, element );
    }
    va_end(ap);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      AddElement                                                           */
/*                                                                           */
/*      Add a single element to a set. If the element to be added is outside */
/*      the allowed range of set values, signal an error to stderr and stop. */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to which the element is to be added    */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          Element    Integer, element to be added.                         */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void  AddElement( SET *setptr, int Element )
{
    unsigned mask, index;

    if ( Element < 0 || Element >= SET_SIZE )  {
        fprintf( stderr, "AddElement: attempt to insert an out of range " );
        fprintf( stderr, "element (%d) into a set\n", Element );
        exit( EXIT_FAILURE );
    }
    else  {
        mask = 1 << ( Element % BITS_PER_WORD );
        index = Element / BITS_PER_WORD;
        *((setptr->word)+index) |= mask;                /* bitwise OR        */
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      RemoveElement                                                        */
/*                                                                           */
/*      Remove a single element to a set. If the element to be added is      */
/*      outside the allowed range of set values, signal an error to stderr   */
/*      and stop.                                                            */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to which the element is to be added    */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          Element    Integer, element to be removed.                       */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void  RemoveElement( SET *setptr, int Element )
{
    unsigned mask, index;

    if ( Element < 0 || Element >= SET_SIZE )  {
        fprintf( stderr, "RemoveElement: attempt to delete an out of range " );
        fprintf( stderr, "element (%d) from a set\n", Element );
        exit( EXIT_FAILURE );
    }
    else  {
        mask = 1 << ( Element % BITS_PER_WORD );
        index = Element / BITS_PER_WORD;
        *((setptr->word)+index) &= ~mask;       /* bitwise AND & negate      */
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      InSet                                                                */
/*                                                                           */
/*      Checks to see if an Element is a member of a set. Returns non zero   */
/*      if so, zero if not. If the element is out of range, signal an error  */
/*      and stop.                                                            */
/*                                                                           */
/*      Input/Output:                                                        */
/*                                                                           */
/*          setptr     Pointer to set to which the element is to be added    */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          Element    Integer, element to be checked for set membership     */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC int   InSet( SET *setptr, int Element )
{
    unsigned mask, index, membership;

    if ( Element < 0 || Element >= SET_SIZE )  {
        fprintf( stderr, "RemoveElement: attempt to delete an out of range " );
        fprintf( stderr, "element (%d) from a set\n", Element );
        exit( EXIT_FAILURE );
    }
    else  {
        mask = 1 << ( Element % BITS_PER_WORD );
        index = Element / BITS_PER_WORD;
        membership = *((setptr->word)+index) & mask;
    }
    return membership;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Union                                                                */
/*                                                                           */
/*      Generate and return that set which is the set union of the N sets    */
/*      pointers to which are passed as parameters.                          */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          SetCount   Integer. Number of sets whose union is to be formed.  */
/*                                                                           */
/*          ...        Variable number of pointer to SET parameters which    */
/*                     will be used to form the returned set.                */
/*                     The variable argument list is processed using C's     */
/*                     facility for handling varying numbers of arguments.   */
/*                     This facility is quite fragile so it is important     */
/*                     to ensure that SetCount is correct and that all the   */
/*                     parameters are pointers to sets.                      */
/*                                                                           */
/*      Returns:       A set, the union of the inputs.                       */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC SET  Union( int SetCount, ... )
{
    va_list ap;
    int     i, j;
    SET     setunion, *sp;

    va_start(ap, SetCount);
    ClearSet( &setunion );
    for ( i = 0; i < SetCount; i++ )  {
        sp = va_arg(ap, SET *);
        for ( j = 0; j < WORDS_PER_SET; j++ )  setunion.word[j] |= sp->word[j];
    }
    va_end(ap);
    return  setunion;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Intersection                                                         */
/*                                                                           */
/*      Generate and return that set which is the set intersection of the N  */
/*      sets pointers to which are passed as parameters.                     */
/*                                                                           */
/*      Input:                                                               */
/*                                                                           */
/*          SetCount   Integer. Number of sets whose intersetion is to be    */
/*                     formed.                                               */
/*                                                                           */
/*          ...        Variable number of pointer to SET parameters which    */
/*                     will be used to form the returned set.                */
/*                     The variable argument list is processed using C's     */
/*                     facility for handling varying numbers of arguments.   */
/*                     This facility is quite fragile so it is important     */
/*                     to ensure that SetCount is correct and that all the   */
/*                     parameters are pointers to sets.                      */
/*                                                                           */
/*      Returns:       A set, the intersection of the inputs.                */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC SET  Intersection( int SetCount, ... )
{
    va_list ap;
    int     i, j;
    SET     setinter, *sp;

    va_start(ap, SetCount);
    ClearSet( &setinter );
    if ( SetCount > 0 )  {
        sp = va_arg(ap, SET *);
        for ( j = 0; j < WORDS_PER_SET; j++ )  setinter.word[j] = sp->word[j];
    }
    for ( i = 1; i < SetCount; i++ )  {
        sp = va_arg(ap, SET *);
        for ( j = 0; j < WORDS_PER_SET; j++ )  setinter.word[j] &= sp->word[j];
    }
    va_end(ap);
    return  setinter;
}
