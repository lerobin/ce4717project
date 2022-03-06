/*
        simpars2.c

        An alternative very simple parser for the language L(G).

            G = {{S,A},{a,b,z},S,P}

            P = {  S --> aA,                    (i)
                   A --> z,                     (ii)
                   A --> aA,                    (iii)
                   A --> bA }                   (iv)

       This parser accepts a line of input from ther terminal (using gets)
       and checks to see if it is a valid sentence from L(G). If so the
       parser prints "Valid sentence", if not "Syntax error".

       The handling of terminals is crude. Each terminal is a single lowercase
       letter. No spaces are allowed. The string is terminated (in standard
       C fashion) with a null (`\0`) character.

       The techniques of recursive descent are used, each production has
       a function of its own to handle it, and these may call each other
       recursively.

*/

#include <stdio.h>
#include <stdlib.h>

#define  BUFSIZE      120
#define  ENDLINE      '\0'

int  Production1( char *sentence );
int  Production2( char *sentence );
int  Production3( char *sentence );
int  Production4( char *sentence );
void readline(char *buffer, int maxchars);

int main( void )
{
    char sentence[BUFSIZE];

    do  {
        printf( "Enter sentence ==> " );
        readline(sentence, BUFSIZE);
        if ( *sentence != ENDLINE )  {
            if ( Production1( sentence ) )  printf( "Valid sentence\n" );
            else  printf( "Syntax Error\n" );
        }
        else  printf( "Goodbye\n" );
    }  while ( *sentence != ENDLINE );
    return EXIT_SUCCESS;
}

/*
        Parse  S --> aA         (i.e., production 1)

        This involves checking (i)  that the first character is 'a'
                               (ii) choosing the next appropriate production
*/

int  Production1( char *sentence )
{
    int status;

    printf( "Applying production 1, S --> aA, to string \"%s\"\n", sentence );
    status = ( *sentence == 'a' );
    if ( status )  {
        switch ( *(sentence+1) )  {
            case 'z' :  status = Production2( sentence+1 );   break;
            case 'a' :  status = Production3( sentence+1 );   break;
            case 'b' :  status = Production4( sentence+1 );   break;
            default  :  status = 0;                           break;
        }
    }
    return  status;
}

/*
        Parse  A --> z          (i.e., production 2)

        This involves checking that the first character is 'z'
*/

int  Production2( char *sentence )
{

    printf( "Applying production 2, A --> z,  to string \"%s\"\n", sentence );
    return ( *sentence == 'z' );
}

/*
        Parse  A --> aA         (i.e., production 3)

        This involves checking (i)  that the first character is 'a'
                               (ii) choosing the next appropriate production
*/

int  Production3( char *sentence )
{
    int status;

    printf( "Applying production 3, A --> aA, to string \"%s\"\n", sentence );
    status = ( *sentence == 'a' );
    if ( status )  {
        switch ( *(sentence+1) )  {
            case 'z' :  status = Production2( sentence+1 );   break;
            case 'a' :  status = Production3( sentence+1 );   break;
            case 'b' :  status = Production4( sentence+1 );   break;
            default  :  status = 0;                           break;
        }
    }
    return  status;
}

/*
        Parse  A --> bA         (i.e., production 4)

        This involves checking (i)  that the first character is 'b'
                               (ii) choosing the next appropriate production
*/

int  Production4( char *sentence )
{
    int status;

    printf( "Applying production 4, A --> bA, to string \"%s\"\n", sentence );
    status = ( *sentence == 'b' );
    if ( status )  {
        switch ( *(sentence+1) )  {
            case 'z' :  status = Production2( sentence+1 );   break;
            case 'a' :  status = Production3( sentence+1 );   break;
            case 'b' :  status = Production4( sentence+1 );   break;
            default  :  status = 0;                           break;
        }
    }
    return  status;
}


/*
        Simulate gets behaviour using fgets (gets deprecated but
        fgets, unlike gets, leaves '\n' at the end of the string.
        This routine simply removes the first '\n' and replaces
        it with '\0'.
*/

void readline(char *buffer, int maxchars)
{
    int i;
    char ch;

    fgets( buffer, maxchars, stdin );
    for ( i = 0; (ch = buffer[i]) != ENDLINE; i++ )
        if ( ch == '\n' ) {
            buffer[i] = ENDLINE; break;
        }
}     
