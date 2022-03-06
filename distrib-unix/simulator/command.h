#ifndef  COMMANDHEADER
/*
    command.h

    header file for the command line processor
*/

#define  COMMANDHEADER
#define  EXIT       0
#define  DISREGS    1
#define  DDMEM      2
#define  DCMEM      3
#define  RUN        4
#define  TRACE      5
#define  SETPC      6
#define  SETSP      7
#define  SETFP      8
#define  SETDM      9
#define  SETBP     10
#define  SHOWBP    11
#define  CLEARBP   12
#define  RESET     13
#define  LOG       14

int  ReadCommand( int *p1, int *p2 );
#endif
