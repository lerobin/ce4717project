/*
        simparse.c

        A very simple, top-down LL(1) parser for the language L(G) defined
        by the grammar G

            G = {{S,A},{a,b,z},S,P}

            P = {  S --> aA,                    (i)
                   A --> z,                     (ii)
                   A --> aA,                    (iii)
                   A --> bA }                   (iv)

       This parser accepts a string of characters from the keyboard and 
       checks to see if it is a valid sentence from L(G). If so the
       parser prints "Valid sentence", if not "Syntax error".

       The handling of terminals is crude. Each terminal is a single lowercase
       letter. No spaces are allowed. 

       The techniques of recursive descent are used, each production has
       a function of its own to handle it, and these may call each other
       recursively. This parser is of the same form as the one described in 
       the notes, the few differences simply serve to make it easier to
       use. In particular, it prompts for multiple sentences, continuing until
       the user unters a null string or a parse error is encountered. Also, 
       the program prints information about what production is currently being 
       applied to the sentence, thus showing the progress of the parse.

*/

#include <stdio.h>
#include <stdlib.h>

#define  ENDLINE   '\n'

int  CurrentToken;

void GetToken( void );
void SyntaxError( void );
void ReadToEOL( void );
void Production1( void );
void Production2( void );
void Production3( void );
void Production4( void );

int main( void )
{
    do  {
        printf( "Enter sentence ==> " );
        GetToken();
        if ( CurrentToken != ENDLINE && CurrentToken != EOF )  {
            Production1();
            printf( "Valid sentence\n" );
            ReadToEOL();
        }
        else  printf( "Goodbye\n" );
    }  while ( CurrentToken != ENDLINE && CurrentToken != EOF );
    return  EXIT_SUCCESS;
}

void GetToken( void )
{
    CurrentToken = getchar();
}

void SyntaxError( void )
{
    printf( "Syntax error\n" );
    exit( EXIT_FAILURE );
}

/*
        clear the input buffer to the end of the current line
*/

void ReadToEOL( void )
{
    int c;

    if ( CurrentToken != EOF && CurrentToken != ENDLINE )  {
        while ( EOF != ( c = getchar() ) && c != ENDLINE ) ;
    }
}

/*
        Parse  S --> aA         (i.e., production 1)

        This involves checking (i)  that the first character is 'a'
                               (ii) choosing the next appropriate production
*/

void Production1( void )
{
    printf( "Applying production 1, S --> aA\n" );
    if ( CurrentToken != 'a' )  SyntaxError();
    else  {
        GetToken();
        switch ( CurrentToken )  {
            case 'z' :  Production2();  break;
            case 'a' :  Production3();  break;
            case 'b' :  Production4();  break;
            default  :  SyntaxError();  break;
        }
    }
}

/*
        Parse  A --> z          (i.e., production 2)

        This involves checking that the first character is 'z'
*/

void Production2( void )
{
    printf( "Applying production 2, A --> z\n" );
    if ( CurrentToken != 'z' )  SyntaxError();
}

/*
        Parse  A --> aA         (i.e., production 3)

        This involves checking (i)  that the first character is 'a'
                               (ii) choosing the next appropriate production
*/

void Production3( void )
{
    printf( "Applying production 3, A --> aA\n" );
    if ( CurrentToken != 'a' )  SyntaxError();
    else  {
        GetToken();
        switch ( CurrentToken )  {
            case 'z' :  Production2();  break;
            case 'a' :  Production3();  break;
            case 'b' :  Production4();  break;
            default  :  SyntaxError();  break;
        }
    }
}

/*
        Parse  A --> bA         (i.e., production 4)

        This involves checking (i)  that the first character is 'b'
                               (ii) choosing the next appropriate production
*/

void Production4( void )
{
    printf( "Applying production 4, A --> bA\n" );
    if ( CurrentToken != 'b' )  SyntaxError();
    else  {
        GetToken();
        switch ( CurrentToken )  {
            case 'z' :  Production2();  break;
            case 'a' :  Production3();  break;
            case 'b' :  Production4();  break;
            default  :  SyntaxError();  break;
        }
    }
}
