/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      scanner.c                                                            */
/*                                                                           */
/*      Implementation file for the lexical analyser                         */
/*                                                                           */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "line.h"
#include "strtab.h"
#include "scanner.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Definitions of constants local to the module                         */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  MAXDISPLAYLENGTH                73 
#define  MAXERRORSPERLINE                 5

#define  ERRORTOKENSTRING               "Scanner Error"
#define  ILLEGALCHARTOKENSTRING         "Illegal Character"
#define  ENDOFINPUTTOKENSTRING          "End of File"
#define  SEMICOLONTOKENSTRING           ";"
#define  COMMATOKENSTRING               ","
#define  ENDOFPROGRAMTOKENSTRING        "."
#define  LEFTPARENTHESISTOKENSTRING     "("
#define  RIGHTPARENTHESISTOKENSTRING    ")"
#define  ASSIGNMENTTOKENSTRING          ":="
#define  ADDTOKENSTRING                 "+"
#define  SUBTRACTTOKENSTRING            "-"
#define  MULTIPLYTOKENSTRING            "*"
#define  DIVIDETOKENSTRING              "/"
#define  EQUALITYTOKENSTRING            "="
#define  LESSEQUALTOKENSTRING           "<="
#define  GREATEREQUALTOKENSTRING        ">="
#define  LESSTOKENSTRING                "<"
#define  GREATERTOKENSTRING             ">"
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
#define  IDENTIFIERTOKENSTRING          "Identifier"
#define  INTCONSTTOKENSTRING            "Integer Constant"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Function Prototypes for routines PRIVATE to this module              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE int  SearchKeywords( char *s );

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Data Structures for this module                                      */
/*                                                                           */
/*      "Tokens" is an array of strings representing the tokens. The         */
/*      strings for keywords are included here. It is important that the     */
/*      keyword strings be maintained in alphabetical order so that a        */
/*      binary search may be conducted on them.                              */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE char   *Tokens[] =  { 
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

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Public routines (globally accessable).                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetToken                                                             */
/*                                                                           */
/*      Reads the next token from the input stream and returns it in a       */
/*      TOKEN structure.                                                     */
/*                                                                           */
/*      Input(s):      None                                                  */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       TOKEN structure containing token just scanned from    */
/*                     the input stream.                                     */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      A state transiton diagram for the scanner is as follows:             */
/*                                                                           */
/*                                                                           */
/*          state 0                                                          */
/*            |                                                              */
/*            |                                                              */
/*            +--------<---------+------<-----                               */
/*            |                  |            |                              */
/*            v      whitespace  |            |                              */
/*          state 1 ---->--------             |                              */
/*            |                               |                              */
/*            v     '!'                 '\n'  |                              */
/*            +-----------> state 2 -+-------                                */
/*            |                ^     |                                       */
/*            |                |     |                                       */
/*            |                 -----                                        */
/*            |                 other                                        */
/*            |                                                              */
/*            v     ';'                                                      */
/*            +-----------> state 3   return  SEMICOLON                      */
/*            |                                                              */
/*            |                                                              */
/*            v     ','                                                      */
/*            +-----------> state 4   return  COMMA                          */
/*            |                                                              */
/*            |                                                              */
/*            v     '.'                                                      */
/*            +-----------> state 5   return  ENDOFPROGRAM                   */
/*            |                                                              */
/*            |                                                              */
/*            v     '('                                                      */
/*            +-----------> state 6   return  LEFTPARENTHESIS                */
/*            |                                                              */
/*            |                                                              */
/*            v     ')'                                                      */
/*            +-----------> state 7   return  RIGHTPARENTHESIS               */
/*            |                                                              */
/*            |                                                              */
/*            v     ':'                 '='                                  */
/*            +-----------> state 8  -+-------> state 9  return ASSIGNMENT   */
/*            |                       |                                      */
/*            |                       | other           *                    */
/*            |                        -------> state 10  return ERROR       */
/*            |                                                              */
/*            v     '+'                                                      */
/*            +-----------> state 11  return  ADD                            */
/*            |                                                              */
/*            |                                                              */
/*            v     '-'                                                      */
/*            +-----------> state 12  return  SUBTRACT                       */
/*            |                                                              */
/*            |                                                              */
/*            v     '*'                                                      */
/*            +-----------> state 13  return  MULTIPLY                       */
/*            |                                                              */
/*            |                                                              */
/*            v     '/'                                                      */
/*            +-----------> state 14  return  DIVIDE                         */
/*            |                                                              */
/*            |                                                              */
/*            v     '='                                                      */
/*            +-----------> state 15  return  EQUALITY                       */
/*            |                                                              */
/*            |                                                              */
/*            v     '<'                  '='                                 */
/*            +-----------> state 16 -+-------> state 17 return LESSEQUAL    */
/*            |                       |                                      */
/*            |                       | other           *                    */
/*            |                        -------> state 18 return LESS         */
/*            |                                                              */
/*            |                                                              */
/*            v     '>'                  '='                                 */
/*            +-----------> state 19 -+-------> state 20 return GREATEREQUAL */
/*            |                       |                                      */
/*            |                       | other           *                    */
/*            |                        -------> state 21 return GREATER      */
/*            |                                                              */
/*            v     EOF                                                      */
/*            +-----------> state 22  return  ENDOFINPUT                     */
/*            |                                                              */
/*            |                                                              */
/*            v '0' .. '9'              other           *                    */
/*            +-----------> state 23 -+-------> state 24 return INTCONST     */
/*            |                ^      |                                      */
/*            |                |      |                                      */
/*            |                 ------                                       */
/*            |                '0' .. '9'                                    */
/*            |                                                              */
/*            | 'A' .. 'Z'                                                   */
/*            v 'a' .. 'z'                              *                    */
/*            +-----------> state 25 -+-------> state 26 return IDENTIFIER   */
/*            |                ^      |                                      */
/*            |                |      |                                      */
/*            |                 ------                                       */
/*            |                'A' .. 'Z' 'a' .. 'z' '0' .. '9'              */
/*            |                                                              */
/*            v otherwise                                                    */
/*            +-----------> state 27  return  ILLEGALCHAR                    */
/*                                                                           */
/*                                                                           */
/*      Note, if this state machine finds an identifier, the keyword table   */
/*      is searched to see if the identifier is really a keyword.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC TOKEN  GetToken( void )
{
    int   state = 0, scanning = 1, ch;
    TOKEN token;

    /* The following two initialisations are only needed because 
     * Microsoft's CL compiler complains that the fields are
     * potentially unitialised if they are left out. (Only at the
     * strictest (/W4) level of error checking.)
     */

    token.code = ERROR; 
    token.value = 0; 

    while ( scanning )  {
        scanning = 0;
        switch ( state )  {
            case  0 :  
                token.value = 0;  
                NewString();  
                state = 1;         
                scanning = 1;                                         break;
            case  1 :  
                token.pos = CurrentCharPos();
                ch = ReadChar();
                switch ( ch )  {
                    case  '!' :  state =  2;                          break;
                    case  ';' :  state =  3;                          break;
                    case  ',' :  state =  4;                          break;
                    case  '.' :  state =  5;                          break;
                    case  '(' :  state =  6;                          break;
                    case  ')' :  state =  7;                          break;
                    case  ':' :  state =  8;                          break;  
                    case  '+' :  state = 11;                          break;  
                    case  '-' :  state = 12;                          break;   
                    case  '*' :  state = 13;                          break;   
                    case  '/' :  state = 14;                          break;   
                    case  '=' :  state = 15;                          break;   
                    case  '<' :  state = 16;                          break;   
                    case  '>' :  state = 19;                          break;   
                    case  EOF :  state = 22;                          break;  
                    default   :  
                        if ( isspace( ch ) )  state = 1;
                        else if ( isdigit( ch ) )  {
                            token.value *= 10;
                            token.value += ch - '0';
                            state = 23;
                        }
                        else if ( isalpha( ch ) )  {
                            AddChar( ch );
                            state = 25;
                        }
                        else  state = 27;                             break;
                }
                scanning = 1;                                         break;
            case  2 :  
                ch = ReadChar();
                if ( ch == '\n' || ch == EOF )  state = 1;  else  state = 2;
                scanning = 1;                                         break;
            case  3 :  token.code = SEMICOLON;                        break;
            case  4 :  token.code = COMMA;                            break;
            case  5 :  token.code = ENDOFPROGRAM;                     break; 
            case  6 :  token.code = LEFTPARENTHESIS;                  break; 
            case  7 :  token.code = RIGHTPARENTHESIS;                 break;
            case  8 :
                ch = ReadChar();
                if ( ch == '=' )  state = 9;  else  state = 10;
                scanning = 1;                                         break;
            case  9 :  token.code = ASSIGNMENT;                       break;
            case 10 :  token.code = ERROR;             UnReadChar();  break;
            case 11 :  token.code = ADD;                              break; 
            case 12 :  token.code = SUBTRACT;                         break; 
            case 13 :  token.code = MULTIPLY;                         break; 
            case 14 :  token.code = DIVIDE;                           break; 
            case 15 :  token.code = EQUALITY;                         break; 
            case 16 :
                ch = ReadChar();
                if ( ch == '=' )  state = 17;  else  state = 18;
                scanning = 1;                                         break;
            case 17 :  token.code = LESSEQUAL;                        break; 
            case 18 :  token.code = LESS;              UnReadChar();  break; 
            case 19 :
                ch = ReadChar();
                if ( ch == '=' )  state = 20;  else  state = 21;
                scanning = 1;                                         break;
            case 20 :  token.code = GREATEREQUAL;                     break;
            case 21 :  token.code = GREATER;           UnReadChar();  break;
            case 22 :  token.code = ENDOFINPUT;                       break;
            case 23 :
                ch = ReadChar();
                if ( isdigit( ch ) )  {
                    token.value *= 10;
                    token.value += ch - '0';
                    state = 23;
                }
                else  state = 24;
                scanning = 1;                                         break;
            case 24 :  token.code = INTCONST;          UnReadChar();  break;
            case 25 :
                ch = ReadChar();
                if ( isalnum( ch ) )  {
                    AddChar( ch );
                    state = 25;
                }
                else  state = 26;
                scanning = 1;                                         break;
            case 26 :  token.code = IDENTIFIER;        UnReadChar();  break;
            case 27 :  token.code = ILLEGALCHAR;                      break;
            default :  
                fprintf(stderr, "Error, GetToken, invalid state %d\n", state);
                exit( EXIT_FAILURE );                                 break;
        }
    }

    if ( token.code == IDENTIFIER )  {
        AddChar( '\0' );                /* null-terminate the string         */
        token.s = GetString();
        token.code = SearchKeywords( token.s );
        if ( token.code != IDENTIFIER )  token.s = NULL;
    }
    return  token;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      SyntaxError                                                          */
/*                                                                           */
/*      Generates a syntax error report.                                     */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          Expected   an integer representing the code of the token which   */
/*                     the parser expected to read.                          */
/*          CurrentToken                                                     */
/*                     A TOKEN structure containing the token which was read */
/*                     by the parser.                                        */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   SyntaxError( int Expected, TOKEN CurrentToken )
{
    char s[M_LINE_WIDTH+2];

    snprintf( s, sizeof(s), "Syntax: Expected %s, got %s\n", 
	     Tokens[Expected], Tokens[CurrentToken.code] );
    Error( s, CurrentToken.pos );
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      SyntaxError2                                                         */
/*                                                                           */
/*      Also generates a syntax error report, but one in which any one of a  */
/*      set of tokens was expected, instead of a single token.               */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */
/*          Expected   a SET of tokens, any one of which was expected by     */
/*                     the parser, but not read.                             */
/*          CurrentToken                                                     */
/*                     A TOKEN structure containing the token which was read */
/*                     by the parser.                                        */
/*                                                                           */
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Nothing                                               */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PUBLIC void   SyntaxError2( SET Expected, TOKEN CurrentToken )
{
    char s[2*M_LINE_WIDTH+2];
    int i, j, pos, w;
    snprintf( s, sizeof(s), "Syntax: Expected one of: " );  pos = 25;
    w = (int)(2*M_LINE_WIDTH - strlen( Tokens[CurrentToken.code] - 8 ));
    for ( i = 0; i < SET_SIZE; i++ )  {
	if ( InSet( &Expected, i ) )  {
	    j = (int)(strlen( Tokens[i] ) + 1);
	    if ( pos + j > w )  break;
	    else  {
		snprintf( s+pos, sizeof(s)-pos, "%s ", Tokens[i] );
		pos += j;
	    }
	}
    }
    snprintf( s+pos, sizeof(s)-pos, ": got %s\n", 
	      Tokens[CurrentToken.code] );
    Error( s, CurrentToken.pos );
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      Private routines (only accessable within this module).               */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      SearchKeywords                                                       */
/*                                                                           */
/*      Performs a binary search of a list of keywords to determine if the   */
/*      argument which it has been passed is amongst them.                   */
/*                                                                           */
/*      Input(s):                                                            */
/*                                                                           */ 
/*          s          pointer to a character string which is to be checked  */
/*                                                                           */ 
/*      Output(s):     None                                                  */
/*                                                                           */
/*      Returns:       Token code of the keyword, if the argument is a       */
/*                     keyword. If it is not a keyword, returns IDENTIFIER.  */
/*                                                                           */
/*---------------------------------------------------------------------------*/

PRIVATE int  SearchKeywords( char *s )
{
    int TopIndex, BottomIndex, MiddleIndex, OldMiddle, i, found;

    BottomIndex = BEGIN;  TopIndex = WRITE+1; OldMiddle = 0;
    found = 0;
    do  {
        MiddleIndex = (TopIndex+BottomIndex)/2;
        if ( MiddleIndex == OldMiddle )  break;
        else  OldMiddle = MiddleIndex;
        if ( 0 == ( i = strncmp( s, *(Tokens+MiddleIndex), 30 ) ) )  found = 1;
        else if ( i < 0 )  TopIndex = MiddleIndex;
        else  BottomIndex = MiddleIndex;
    }
    while ( !found );

    if ( !found )  return  IDENTIFIER;
    else  return  MiddleIndex;
}
