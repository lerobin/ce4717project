/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      line.c                                                               */
/*                                                                           */
/*      Implementation file for the character processor. This is responsible */
/*      for reading the program text from the input file, assembling it into */
/*      lines to place on the listing file and hadling the 1-character       */
/*      pushback which is needed by certain scanner states. Since it may be  */
/*      necessary to push back over a line boundary, it it necessary to      */
/*      buffer up to two lines internally, so that the previous line is      */
/*      always available. A lot of messy details concerning the handling     */
/*      of this buffering are handled internally in this module.             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "line.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Definitions of constants local to the module                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  DISPLAY_LINE_NUMBER              1     /* see DisplayLine           */
#define  NO_DISPLAY_LINE_NUMBER           0     /* see DisplayLine           */

#define  DEFAULT_TAB_WIDTH                8     /* Default tab size          */

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Data Structures for this module                                      */
/*                                                                           */
/*      LINE is a data structure which gathers together all the information  */
/*      pertinent to a line of input, i.e., the text of the line, the        */
/*      position where the next character is to be inserted, the number of   */
/*      error messages for the line and their positions and text strings.    */
/*                                                                           */
/*      CurrentLine is a pointer to the current line being processed,        */
/*      PreviousLine to the previous one. It is necessary to keep tis line   */
/*      to allow for UnReadChar to read back over an end of line marker.     */
/*                                                                           */
/*      InputFile is the current input, it defaults to stdin (see ReadChar). */
/*      ListFile is where the listing is being written. If it is NULL, no    */
/*      listing is being produced. Defaults to NULL.                         */
/*                                                                           */
/*      CurrentLineNum is the line number of the current line.               */
/*                                                                           */
/*      PushBack is a flag which is true when UnReadChar has been called     */
/*      to push back a character onto the input stream.                      */
/*                                                                           */
/*      ReadEOF is a flag which is set once EOF has been read from the       */
/*      input stream. It ensures that no further reads take place.           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

typedef struct  {
    int  valid;                 /* =1 if the line contains meaningful data   */
    int  cpos;                  /* current character position                */
    char s[M_LINE_WIDTH+2];     /* text of line                              */
    int  cerrs;                 /* number of errors in line                  */
    int  epos[M_ERRS_LINE];     /* positions of the errors                   */
    char e[M_ERRS_LINE][M_LINE_WIDTH+2];        /* array of error message    */
}                                               /* strings                   */
    LINE;

PRIVATE LINE *CurrentLine  = NULL,
             *PreviousLine = NULL;

PRIVATE FILE *InputFile = NULL,
             *ListFile  = NULL;

PRIVATE int  CurrentLineNum = 1;
PRIVATE int  PushBack       = 0;
PRIVATE int  ReadEOF        = 0;

PRIVATE int  TabWidth       = DEFAULT_TAB_WIDTH;

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Function Prototypes for routines PRIVATE to this module              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void DisplayLine( int number, LINE *line );
PRIVATE LINE *NewLine( void );
PRIVATE void SwapLines( LINE **a, LINE **b );
PRIVATE void DisplayErrorMessage( int indent, char *message );

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      InitCharProcessor                                                    */
/*                                                                           */
/*      Establishes the input and listing files.                             */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          inputfile  pointer to a FILE structure which must be opened on   */
/*                     the file to be used for input. This argument MUST     */
/*                     represent a valid open file, NULL is not acceptable.  */
/*                                                                           */
/*          listfile   pointer to a FILE structure which should be opened    */
/*                     for output on the file to be used for listing the     */
/*                     program. If this pointer is NULL, no listing will     */
/*                     be produced.                                          */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   InitCharProcessor( FILE *inputfile, FILE *listfile )
{
    if ( inputfile == NULL ) {
	fprintf( stderr, "Fatal Error: InitCharProcessor: attempt to\n");
	fprintf( stderr, "use an invalid file handle (NULL) for input\n");
	exit( EXIT_FAILURE );
    }
    else InputFile = inputfile;
    ListFile  = listfile;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Error                                                                */
/*                                                                           */
/*      Places an error message in the current error buffer of the current   */
/*      line. If the line already contains M_ERRS_LINE error messages the    */
/*      message is ignored. If the listing file is other than stderr or      */
/*      stdin, the error is reported to stderr also.                         */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          ErrorString  pointer to a error message string.                  */
/*                                                                           */
/*          PositionInLine                                                   */
/*                       position in the current input line where the        */
/*                       error occurred.                                     */
/*                                                                           */
/*      Output(s):       None                                                */
/*                                                                           */
/*      Returns:         Nothing                                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   Error( char *ErrorString, int PositionInLine )
{
    if ( CurrentLine == NULL || !(CurrentLine->valid) )  {
        if ( ListFile != NULL ) 
            DisplayErrorMessage( PositionInLine, ErrorString );
    }
    else  {
        if ( CurrentLine->cerrs < M_ERRS_LINE && ListFile != NULL )  {
            strncpy( CurrentLine->e[CurrentLine->cerrs], ErrorString, 
                     M_LINE_WIDTH );
            CurrentLine->epos[CurrentLine->cerrs] = PositionInLine;
            (CurrentLine->cerrs)++;
        }
    }
    if ( ListFile != stderr && ListFile != stdin )
        fprintf( stderr, "Error: %s\n", ErrorString );
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      ReadChar                                                             */
/*                                                                           */
/*      Reads the next character from the input file and places in the       */
/*      line buffer as well as returning it to the caller. If the character  */
/*      is end-of-line, write the line buffer out if listing is enabled.     */
/*      If the character is a tab, expand it as spaces in the listing file.  */
/*      The amount of expansion is controlled by the "TabWidth" module       */
/*      variable, this is normally 8 (setting tabs every 8 characters), but  */
/*      may be changed by the user.                                          */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Next character from the input stream.                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC int  ReadChar( void )
{
    int ch, i, j;

    if ( ReadEOF )  return EOF;

    if ( PushBack )  {
        if ( CurrentLine == NULL )  {
            fprintf( stderr, "No current line, but PushBack true\n" );
            exit( EXIT_FAILURE );
        }
        ch = *(CurrentLine->s+CurrentLine->cpos);
        PushBack = 0;
        (CurrentLine->cpos)++;
    }
    else  {
        if ( CurrentLine == NULL )  CurrentLine = NewLine();
	if ( InputFile == NULL ) InputFile = stdin;
	ch = fgetc( InputFile );
	if ( ch == '\t' ) {
            CurrentLine->valid = 1;
	    i = CurrentLine->cpos;
            for ( j = TabWidth; j <= i; j += TabWidth ) ;
	    for ( ; i < j && i < M_LINE_WIDTH; i++ )
                *(CurrentLine->s+i) = ' ';
	    CurrentLine->cpos = i;
	    ch = ' ';
	}
        else if ( ch != EOF )  {
            CurrentLine->valid = 1;
            *(CurrentLine->s+CurrentLine->cpos) = (char)ch;
            (CurrentLine->cpos)++;
        }
    }

    if ( ch == '\n' )  {
        DisplayLine( DISPLAY_LINE_NUMBER, PreviousLine );
        SwapLines( &CurrentLine, &PreviousLine );
        if ( CurrentLine != NULL )  {
            CurrentLine->valid = 0;  CurrentLine->cpos = 0;
        }
    }
    else if ( CurrentLine->cpos > M_LINE_WIDTH )  {
        DisplayLine( NO_DISPLAY_LINE_NUMBER, PreviousLine );
        SwapLines( &CurrentLine, &PreviousLine );
        if ( CurrentLine != NULL )  {
            CurrentLine->valid = 0;  CurrentLine->cpos = 0;
        }
    }
    else if ( ch == EOF )  {
        if ( CurrentLine->valid && CurrentLine->cpos != 0 )  {
	    *(CurrentLine->s+(CurrentLine->cpos)) = '\n';
            (CurrentLine->cpos)++;
	}
        DisplayLine( DISPLAY_LINE_NUMBER, PreviousLine );
        DisplayLine( DISPLAY_LINE_NUMBER, CurrentLine );
	ReadEOF = 1;
    }

    return ch;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      UnReadChar                                                           */
/*                                                                           */
/*      "Unreads" the current input character by pushing it back onto the    */
/*      input stream. The effect of this is to return this character on the  */
/*      next call to "ReadChar".                                             */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing.                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void UnReadChar( void )
{
    if ( PushBack )  {
        fprintf( stderr, "Attempt to unread more than one character\n" );
        exit( EXIT_FAILURE );
    }
    else  {
	if (!ReadEOF)  {
            if ( CurrentLine == NULL || !(CurrentLine->valid) || 
                CurrentLine->cpos == 0 )  {
                if ( PreviousLine == NULL )  {
                    fprintf( stderr, "Attempt to push back character " );
                    fprintf( stderr, "before start of file\n" );
                    exit( EXIT_FAILURE );
                }
                else  {
                    SwapLines( &CurrentLine, &PreviousLine );
                    if ( PreviousLine != NULL )  {
                        PreviousLine->valid = 0;  PreviousLine->cpos = 0;
                        PreviousLine->cerrs = 0;
                    }
                }
            }
            (CurrentLine->cpos)--;
	}
        PushBack = 1; 
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      CurrentCharPos                                                       */
/*                                                                           */
/*      Returns the "cpos" field of the current input line, this may be      */
/*      used to determine where a token begins.                              */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       "cpos" field of current input line. If this is NULL   */
/*                     or not valid, return 0.                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC int  CurrentCharPos( void )
{
    int i;

    if ( CurrentLine == NULL || !(CurrentLine->valid) )  i = 0;
    else i = CurrentLine->cpos;

    return i;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      SetTabWidth                                                          */
/*                                                                           */
/*      Sets the width of the tab field. This is 8 characters by default     */
/*      but may be adjusted by this routine to any sensible value between    */
/*      3 and 8 spaces (inclusive).                                          */
/*                                                                           */
/*      Input(s):      "NewTabWidth": integer. New tab width. Must be in     */
/*                     the range 3 .. 8. A fatal error is generated if the   */
/*                     user attempts to set a tab width outside this range.  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing.                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void SetTabWidth( int NewTabWidth )
{
    if ( NewTabWidth >= 3 && NewTabWidth <= 8 )  TabWidth = NewTabWidth;
    else {
	fprintf(stderr,"Fatal Error: SetTabWidth: attempt to set an ");
	fprintf(stderr,"illegal tab size (%1d).\n", NewTabWidth);
	fprintf(stderr,"Legal range is 3 to 8\n");
        exit(EXIT_FAILURE);
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetTabWidth                                                          */
/*                                                                           */
/*      Returns the "TabWidth" private variable's value.                     */
/*                                                                           */
/*      Input(s):      None.                                                 */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       An integer, the current tab field width.              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC int GetTabWidth( void )
{
    return TabWidth;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Private routines (only accessable within this module).               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      NewLine                                                              */
/*                                                                           */
/*      Creates a new line and initialises its structures.                   */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */ 
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Pointer to LINE structure                             */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE LINE *NewLine( void )
{
    LINE *p;
    
    if ( NULL == ( p = malloc( sizeof(LINE) ) ) )  {
        fprintf( stderr, "error, failed to allocate memory for LINE\n" );
        exit( EXIT_FAILURE );
    }
    p->valid = 0;
    p->cpos = 0;
    p->cerrs = 0;

    return p;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      SwapLines                                                            */
/*                                                                           */
/*      Swaps the pointers to two LINE structures.                           */
/*                                                                           */
/*      Input/Outputs(s):                                                    */
/*                                                                           */ 
/*          a, b       both pointers to pointers to LINE structures.E_NUMBER */ 
/*                                                                           */ 
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void SwapLines( LINE **a, LINE **b )
{
    LINE *temp;

    temp = *a;
    *a = *b;
    *b = temp;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      DisplayLine                                                          */
/*                                                                           */
/*      Displays the characters of the line passed to it as an argument.     */
/*      The line is only displayed if the current ListFile is non-NULL and   */
/*      if the line itself has a "valid" flag associated with it.            */
/*      This routine has significant side effects, the line becomes          */
/*      invalid, the character position is reset to zero and the error       */
/*      count is similarly reset.                                            */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */ 
/*          number     integer. if equal to the constant DISPLAY_LINE_NUMBER */ 
/*                     this is an instruction to display the current line    */ 
/*                     number and increment it.                              */ 
/*                                                                           */ 
/*          line       Pointer to LINE structure                             */ 
/*                                                                           */ 
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void DisplayLine( int number, LINE *line )
{
    int i;

    if ( line != NULL && line->valid && ListFile != NULL )  {
        i = line->cpos;
        *((line->s)+i) = '\0';
        if ( number == DISPLAY_LINE_NUMBER )  {
            fprintf( ListFile, "%3d ", CurrentLineNum );
            CurrentLineNum++;
        }
        else  fprintf( ListFile, "    " );
        fputs( line->s, ListFile );
        for ( i = 0; i < line->cerrs; i++ )
            DisplayErrorMessage( line->epos[i], line->e[i] );
        line->valid = 0;
        line->cpos = 0;
        line->cerrs = 0;
    }
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      DisplayErrorMessage                                                  */
/*                                                                           */
/*      Displays an error message to the listing file. It is assumed that    */
/*      the listing file is valid when the routine is called.                */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */ 
/*          indent     integer, the location of the error on the line        */
/*                                                                           */ 
/*          message    pointer to null-terminated character string, the      */
/*                     error report.                                         */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE void DisplayErrorMessage( int indent, char *message )
{
    int i;

    fprintf( ListFile, "    " );
    for ( i = 0; i < indent; i++ )  fputc( ' ', ListFile );
    fprintf( ListFile, "^\n%s\n", message );
}
