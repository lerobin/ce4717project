#ifndef  SCANNERHEADER
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      scanner.h                                                            */
/*                                                                           */
/*      Header file for "scanner.c", containing constant declarations,       */
/*      type definitions and function prototypes for the lexical analysis    */
/*      routines.                                                            */
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define  SCANNERHEADER

#define  ERROR              0   /* Returned if scanner encounters an error   */
#define  ILLEGALCHAR        1   /* Returned if an illegal character in input */
#define  ENDOFINPUT         2   /* End of file                               */
#define  SEMICOLON          3   /* ';'                                       */
#define  COMMA              4   /* ','                                       */
#define  ENDOFPROGRAM       5   /* '.'                                       */
#define  LEFTPARENTHESIS    6   /* '('                                       */
#define  RIGHTPARENTHESIS   7   /* ')'                                       */
#define  ASSIGNMENT         8   /* ":="                                      */
#define  ADD                9   /* '+'                                       */
#define  SUBTRACT          10   /* '-'                                       */
#define  MULTIPLY          11   /* '*'                                       */
#define  DIVIDE            12   /* '/'                                       */
#define  EQUALITY          13   /* '='                                       */
#define  LESSEQUAL         14   /* "<="                                      */
#define  GREATEREQUAL      15   /* ">="                                      */
#define  LESS              16   /* '<'                                       */
#define  GREATER           17   /* '>'                                       */
#define  BEGIN             18   /* "BEGIN"                                   */
#define  DO                19   /* "DO"                                      */
#define  ELSE              20   /* "ELSE"                                    */
#define  END               21   /* "END"                                     */
#define  IF                22   /* "IF"                                      */
#define  PROCEDURE         23   /* "PROCEDURE"                               */
#define  PROGRAM           24   /* "PROGRAM"                                 */
#define  READ              25   /* "READ"                                    */
#define  REF               26   /* "REF"                                     */
#define  THEN              27   /* "THEN"                                    */
#define  VAR               28   /* "VAR"                                     */
#define  WHILE             29   /* "WHILE"                                   */
#define  WRITE             30   /* "WRITE"                                   */
#define  IDENTIFIER        31   /* <Identifier>                              */
#define  INTCONST          32   /* <IntConst>                                */

#include <stdio.h>
#include "global.h"
#include "sets.h"

typedef struct  {       /*  Definiton of a TOKEN, the structure which is     */
    int  code;          /*  returned by a call to "GetToken". "code" is the  */
    int  value;         /*  identification code for the TOKEN, "value" is    */
    int  pos;           /*  its value if it is an INTCONST, "pos" is the     */
    char *s;            /*  position in the input line where the token       */
}                       /*  begins, "s" is a pointer to the token string.    */
    TOKEN;              /*  "s" is always NULL unless the token code is      */
                        /*  IDENTIFIER.                                      */

PUBLIC TOKEN  GetToken( void );
PUBLIC void   SyntaxError( int Expected, TOKEN CurrentToken );
PUBLIC void   SyntaxError2( SET Expected, TOKEN CurrentToken );

#endif
