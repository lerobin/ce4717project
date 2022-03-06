/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      debug.c                                                              */
/*                                                                           */
/*      Implementation file debugging routines declared in "debug.h".        */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include <stddef.h>
#include "debug.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Definitions of constants local to the module                         */
/*                                                                           */
/*      These are the symbolic (i.e. print) names of the various TOKEN       */
/*      types.                                                               */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  ERRORTOKENSTRING               "Scanner Error"
#define  ILLEGALCHARTOKENSTRING         "Illegal Character"
#define  ENDOFINPUTTOKENSTRING          "$"
#define  SEMICOLONTOKENSTRING           "';'"
#define  COMMATOKENSTRING               "','"
#define  ENDOFPROGRAMTOKENSTRING        "'.'"
#define  LEFTPARENTHESISTOKENSTRING     "'('"
#define  RIGHTPARENTHESISTOKENSTRING    "')'"
#define  ASSIGNMENTTOKENSTRING          "':='"
#define  ADDTOKENSTRING                 "'+'"
#define  SUBTRACTTOKENSTRING            "'-'"
#define  MULTIPLYTOKENSTRING            "'*'"
#define  DIVIDETOKENSTRING              "'/'"
#define  EQUALITYTOKENSTRING            "'='"
#define  LESSEQUALTOKENSTRING           "'<='"
#define  GREATEREQUALTOKENSTRING        "'>='"
#define  LESSTOKENSTRING                "'<'"
#define  GREATERTOKENSTRING             "'>'"
#define  BEGINTOKENSTRING               "BEGIN"
#define  DOTOKENSTRING                  "DO"
#define  ELSETOKENSTRING                "ELSE"
#define  ENDTOKENSTRING                 "END"
#define  IFTOKENSTRING                  "IF"
#define  PROCEDURETOKENSTRING           "PROCEDURE"
#define  PROGRAMTOKENSTRING             "PROGRAM"
#define  READTOKENSTRING                "READ"
#define  REFTOKENSTRING                 "REF"
#define  THENTOKENSTRING                "THEN"
#define  VARTOKENSTRING                 "VAR"
#define  WHILETOKENSTRING               "WHILE"
#define  WRITETOKENSTRING               "WRITE"
#define  IDENTIFIERTOKENSTRING          "IDENTIFIER"
#define  INTCONSTTOKENSTRING            "INTCONST"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Data Structures for this module                                      */
/*                                                                           */
/*      "Tokens" is an array of strings representing the tokens. The         */
/*      strings for keywords are included here. It is important that the     */
/*      order of this array matches the order in which TOKEN codes are       */
/*      declared in "scanner.h", so that Tokens[INTCONST], for example,      */
/*      looks up the correct name matching the INTCONST ".code" field.       */
/*                                                                           */
/*                                                                           */
/*      printbuffer is a char array used by TokenCode2Str.                   */
/*      setprintbuffer is a char array used by TokenSet2Str.                 */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE char   *TokenNames[] =  { 
            ERRORTOKENSTRING, ILLEGALCHARTOKENSTRING, ENDOFINPUTTOKENSTRING,
            SEMICOLONTOKENSTRING, COMMATOKENSTRING, ENDOFPROGRAMTOKENSTRING,
            LEFTPARENTHESISTOKENSTRING, RIGHTPARENTHESISTOKENSTRING,
            ASSIGNMENTTOKENSTRING, ADDTOKENSTRING, SUBTRACTTOKENSTRING,
            MULTIPLYTOKENSTRING, DIVIDETOKENSTRING, EQUALITYTOKENSTRING,
            LESSEQUALTOKENSTRING, GREATEREQUALTOKENSTRING, LESSTOKENSTRING,
            GREATERTOKENSTRING, BEGINTOKENSTRING, DOTOKENSTRING,
            ELSETOKENSTRING, ENDTOKENSTRING, IFTOKENSTRING, 
            PROCEDURETOKENSTRING, PROGRAMTOKENSTRING, READTOKENSTRING,
            REFTOKENSTRING, THENTOKENSTRING, VARTOKENSTRING, 
            WHILETOKENSTRING, WRITETOKENSTRING, IDENTIFIERTOKENSTRING,
            INTCONSTTOKENSTRING
       };


#define DISBUFSIZE 4000

PRIVATE char printbuffer[DISBUFSIZE];
PRIVATE char setprintbuffer[DISBUFSIZE];

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      TokenCode2Str                                                        */
/*                                                                           */
/*      Returns a pointer to a null-terminated C-string representing a       */
/*      TOKEN.code "name".  Returns "Invalid" if the value passed is not a   */
/*      valid ".code" field value.                                           */
/*                                                                           */
/*      N.B. This is really just an accessor for the TokenNames array above. */
/*                                                                           */
/*                                                                           */
/*      Input(s):      An int representing a TOKEN code field value.         */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       A pointer to a null-terminated C-string (array of     */
/*                     chars).                                               */
/*                                                                           */
/*      N.B., This routine uses a fixed-size static character buffer, so     */
/*      it can't be called recursively and is not re-entrant.                */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC char *TokenCode2Str( int TokenCode ) 
{
    if ( TokenCode < ERROR || TokenCode > INTCONST )  {
	snprintf(printbuffer, sizeof(printbuffer), "Invalid (%d)", TokenCode);
	return printbuffer;
    }
    else  return TokenNames[TokenCode];
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      PrintTokenCode                                                       */
/*                                                                           */
/*      Prints the string version of a token code to standard output.        */
/*                                                                           */
/*      N.B. This is just a wrapper around "TokenCode2Str".                  */
/*                                                                           */
/*                                                                           */
/*      Input(s):      An int representing a TOKEN code field value.         */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void PrintTokenCode( int TokenCode )
{
    puts( TokenCode2Str( TokenCode ) );
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      TokenSet2Str                                                         */
/*                                                                           */
/*      Generates a string representing the contents of a SET of TOKEN       */
/*      codes.                                                               */
/*                                                                           */
/*      Input(s):      A SET object that should contain TOKEN codes.         */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       A pointer to a null-terminated C-string (array of     */
/*                     chars).                                               */
/*                                                                           */
/*      N.B., This routine uses a fixed-size static character buffer, so     */
/*      it can't be called recursively and is not re-entrant.                */
/*                                                                           */
/*      N.B., If the SET is too large (shouldn't happen with TOKEN code      */
/*      SETs), the final elements will be omitted so that the string buffer  */
/*      doesn't overflow.                                                    */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC char  *TokenSet2Str( SET TokenCodes )
{
    int tokcode, elements, pos, j; 
    char *tokname;

    setprintbuffer[0] = '{'; pos = 1;
    elements = 0;
    for ( tokcode = 0; tokcode < SET_SIZE; tokcode++ )  {
	if ( InSet( &TokenCodes, tokcode ) )  {
	    if ( pos >= DISBUFSIZE - 5)  break;
	    if ( elements > 0 )  setprintbuffer[pos++] = ',';
	    setprintbuffer[pos++] = ' ';
	    tokname = TokenCode2Str( tokcode );
	    for ( j = 0; tokname[j] != '\0'; j++ ) {
	        if ( pos >= DISBUFSIZE - 3)  break;
		setprintbuffer[pos++] = tokname[j];
	    }
	    elements++;
	}
    }
    setprintbuffer[pos] = ' ';
    setprintbuffer[pos+1] = '}';
    setprintbuffer[pos+2] = '\0';
    return setprintbuffer;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      PrintTokenSet                                                        */
/*                                                                           */
/*      Prints the string version of a SET of TOKEN codes to standard        */
/*      output.                                                              */
/*                                                                           */
/*      N.B. This is just a wrapper around "TokenCode2Str".                  */
/*                                                                           */
/*                                                                           */
/*      Input(s):      A SET object that should contain TOKEN codes.         */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing.                                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void PrintTokenSet( SET TokenCodes )
{
    puts( TokenSet2Str( TokenCodes ) );
}
