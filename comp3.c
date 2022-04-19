/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp1                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:         ID numbers                                  */
/*                                                                          */
/*       Ethan O'Brien          11134798                                    */
/*       Hammad Aljeddani       09003021                                    */
/*       Hristo Trifonov        11060905                                    */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"
#include "symbol.h"
#include "code.h"
#include "strtab.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int scope = 1;


PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void Synchronise(SET *F, SET *FB);
PRIVATE void SetupSets(void);


PRIVATE void MakeSymbolTableEntry();
PRIVATE SYMBOL *LookupSymbol();


PRIVATE SET ProgramStatementFS_aug1;
PRIVATE SET ProgramStatementFS_aug2;
PRIVATE SET ProcedureStatementFS_aug1;
PRIVATE SET ProcedureStatementFS_aug2;
PRIVATE SET BlockStatementFS_aug;
PRIVATE SET ProgramStatementFBS;
PRIVATE SET ProcedureStatementFBS;
PRIVATE SET BlockStatementFBS;


PRIVATE void ParseProgram(void);
PRIVATE void ParseDeclarations(void);
PRIVATE void ParseProcDeclaration(void);
PRIVATE void ParseParameterList(void);
PRIVATE void ParseFormalParameter(void);
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestOfStatement(SYMBOL *var);
PRIVATE void ParseProcCallList(void);
PRIVATE void ParseAssignment(void);
PRIVATE void ParseActualParameter(void);
PRIVATE void ParseWhileStatement(void);
PRIVATE void ParseIfStatement(void);
PRIVATE void ParseReadStatement(void);
PRIVATE void ParseWriteStatement(void);
PRIVATE void ParseExpression(void);
PRIVATE void ParseCompoundTerm(void);
PRIVATE void ParseTerm(void);
PRIVATE void ParseSubTerm(void);
PRIVATE int ParseBooleanExpression(void);
PRIVATE void ParseAddOp(void);
PRIVATE void ParseMultOp(void);
PRIVATE int ParseRelOp(void);
PRIVATE void ParseVariable(void);
PRIVATE void ParseIntConst(void);
PRIVATE void ParseIdentifier(void);


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )
    {
        InitCharProcessor( InputFile, ListFile );
        InitCodeGenerator(CodeFile); /*Initialize code generation*/
        CurrentToken = GetToken();
        SetupSets();
        ParseProgram();
        WriteCodeFile();  /*Write out assembly to file*/
        fclose( InputFile );
        fclose( ListFile );
        printf("Valid\n");
        return  EXIT_SUCCESS;
    }
    else
    {
        printf("Syntax Error\n");
        return EXIT_FAILURE;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*                                                                          */
/*  ParseProgram implements:                                                */
/*                                                                          */
/*       <Program>     :== "BEGIN" { <Statement> ";" } "END" "."            */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/



PRIVATE void ParseProgram(void)
{
    Accept(PROGRAM);
    /*Lookahead token it should be the Program's name, so we call the MakeSymbolTableEntery*/
    MakeSymbolTableEntry(STYPE_PROGRAM);

    Accept(IDENTIFIER);
    Accept(SEMICOLON);
    Synchronise(&ProgramStatementFS_aug1,&ProgramStatementFBS);
    if(CurrentToken.code == VAR)
        ParseDeclarations();
    Synchronise(&ProgramStatementFS_aug2,&ProgramStatementFBS);
    while (CurrentToken.code == PROCEDURE)
        ParseProcDeclaration();
    Synchronise(&ProgramStatementFS_aug2,&ProgramStatementFBS);
    ParseBlock();
    Accept(ENDOFPROGRAM);
    _Emit(I_HALT);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseDeclarations implements:                                           */
/*                                                                          */
/*      <Declarations> :== "VAR" <Variable> {"," <Variable>} ";"            */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseDeclarations(void)
{
    int vcount = 0;
    Accept(VAR);

    MakeSymbolTableEntry(STYPE_VARIABLE);
    ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
    vcount++;
    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        MakeSymbolTableEntry(STYPE_VARIABLE);
        ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
        vcount++;
    }
    Accept(SEMICOLON);
    Emit(I_INC,vcount);
    DumpSymbols(0);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcDeclaration implements:                                        */
/*                                                                          */
/*      <ProcDeclarations> :== "PRODEDURE" <Identifire> [<ParameterList>]   */
/*              ";" [<Declarations>] {<ProcDeclaration>}                    */
/*              <Block> ";"                                                 */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration(void)
{
    Accept(PROCEDURE);

    MakeSymbolTableEntry(STYPE_PROCEDURE);
    Accept(IDENTIFIER);

    scope++; /* To avoid multiple declarations */

    if(CurrentToken.code == LEFTPARENTHESIS)
        ParseParameterList();

    Accept(SEMICOLON);
    Synchronise(&ProcedureStatementFS_aug1,&ProcedureStatementFBS);
    if(CurrentToken.code == VAR)
        ParseDeclarations();
    Synchronise(&ProcedureStatementFS_aug2,&ProcedureStatementFBS);
    while (CurrentToken.code == PROCEDURE)
        ParseProcDeclaration();
    Synchronise(&ProcedureStatementFS_aug2,&ProcedureStatementFBS);
    ParseBlock();
    Accept(SEMICOLON);

    RemoveSymbols(scope);
    scope--;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseParameterList implements:                                          */
/*                                                                          */
/*      <ParameterList> :== "(" <FormalParameter> {"," <FormalParameter>}   */
/*                              ")"                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseParameterList(void)
{
    Accept(LEFTPARENTHESIS);
    ParseFormalParameter();

    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseFormalParameter();
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseFormalParameter implements:                                        */
/*                                                                          */
/*      <FormalParameter> :== ["REF"] <Variable>                            */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter(void)
{
    if(CurrentToken.code == REF){
        MakeSymbolTableEntry(STYPE_REFPAR); /* Question */
        Accept(REF);
    }

    MakeSymbolTableEntry(STYPE_VARIABLE);

    ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBlock implements:                                                  */
/*                                                                          */
/*      <Block> :== "BEGIN" {<Statement> ";"} "END"                         */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBlock(void)
{
Accept(BEGIN);

 Synchronise(&BlockStatementFS_aug,&BlockStatementFBS);
 while (CurrentToken.code == IDENTIFIER || CurrentToken.code == WHILE ||
    CurrentToken.code == IF || CurrentToken.code == READ ||
    CurrentToken.code == WRITE){
    ParseStatement();
Accept(SEMICOLON);
}
Synchronise(&BlockStatementFS_aug,&BlockStatementFBS);

Accept(END);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*      <Statement> :== <SimpleStatement> | <WhileStatement> |              */
/*                          <IfStatement> | <ReadStatement> |               */
/*                          <WriteStatement>                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseStatement(void)
{
    switch(CurrentToken.code)
    {
        case IDENTIFIER:ParseSimpleStatement(); break;
        case WHILE:ParseWhileStatement(); break;
        case IF:ParseIfStatement(); break;
        case READ:ParseReadStatement(); break;
        case WRITE:ParseWriteStatement(); break;
        default:
        SyntaxError( IDENTIFIER, CurrentToken );
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSimpleStatement implements:                                        */
/*                                                                          */
/*      <SimpleStatement> :== <Variable> <RestOfStatement>                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseSimpleStatement(void)
{
    SYMBOL *var;
    var = LookupSymbol();
    ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
    ParseRestOfStatement(var);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                        */
/*                                                                          */
/*      <ParseRestOfStatement> :== <ProcCallList> | <Assignment> | ϵ        */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseRestOfStatement(SYMBOL *var)
{
    switch(CurrentToken.code)
    {
        case LEFTPARENTHESIS:
            ParseProcCallList(); 
        case SEMICOLON: 
            if ( var != NULL ) {
                if ( var->type == STYPE_PROCEDURE ) {
                    Emit( I_CALL, var->address ); 
                }
                else { 
                    printf("error in parse rest of statement semicolon case");
                    KillCodeGeneration(); 
                } 
            }
            break; 
        case ASSIGNMENT:
        default: 
            ParseAssignment(); 
            if ( var != NULL ) {
                if ( var->type == STYPE_VARIABLE ) {
                    Emit( I_STOREA, var->address ); 
                }
                else {
                    printf("error in parse rest of statement default case");
                    KillCodeGeneration(); 
                } 
            break; 
        }
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcCallList implements:                                           */
/*                                                                          */
/*       <ProcCallList> :== "(" <ActualParameter> {"," <ActualParameter>}   */
/*                          ")"                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseProcCallList(void)
{
    Accept(LEFTPARENTHESIS);
    ParseActualParameter();

    while (CurrentToken.code == COMMA)
    {
        Accept(COMMA);
        ParseActualParameter();
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                             */
/*                                                                          */
/*       <Assignment> :== ":=" <Expression>                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseAssignment(void)
{
    Accept(ASSIGNMENT);
    ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseActualParameter implements:                                        */
/*                                                                          */
/*       <ActualParameter> :== <Variable> | <Expression>                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseActualParameter(void)
{
    ParseExpression();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatement implements:                                         */
/*                                                                          */
/*       <WhileStatement> :== "WHILE" <BooleanExpression> "DO" <Block>      */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseWhileStatement(void)
{
    int Label1, Label2, L2BackPatchLoc;
    Accept(WHILE);
    Label1 = CurrentCodeAddress();
    L2BackPatchLoc = ParseBooleanExpression();
    Accept(DO);
    ParseBlock();
    Emit(I_BR, Label1);
    Label2 = CurrentCodeAddress();
    BackPatch(L2BackPatchLoc,Label2);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                                            */
/*                                                                          */
/*       <IfStatement> :== "IF" <BooleanExpression> "THEN" <Block> ["ELSE"  */
/*                            <Block>]                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseIfStatement(void)
{
    Accept(IF);
    ParseBooleanExpression();
    Accept(THEN);
    ParseBlock();

    if (CurrentToken.code == ELSE)
    {
        Accept(ELSE);
        ParseBlock();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatement implements:                                          */
/*                                                                          */
/*       <ReadStatement> :== "READ" <ProcCallList>                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseReadStatement(void)
{
    _Emit(I_READ);
    Accept(READ);
    ParseProcCallList();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWriteStatement implements:                                         */
/*                                                                          */
/*       <WriteStatement> :== "WRITE" <ProcCallList>                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseWriteStatement(void)
{
    Accept(WRITE);
    ParseProcCallList();
    _Emit(I_WRITE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression> :== <CompountTerm> {<AddOp> <CompountTerm>}           */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseExpression(void)
{
    int op;

    ParseCompoundTerm();
    while((op = CurrentToken.code) == ADD || op == SUBTRACT)
    {
        ParseAddOp();
        ParseCompoundTerm();

        if(op == ADD) _Emit(I_ADD); else _Emit(I_SUB);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                                           */
/*                                                                          */
/*       <CompoundTerm> :== <Term> {<MultOp> <Term>}                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseCompoundTerm(void)
{
    int op;

    ParseTerm();
    while((op = CurrentToken.code) == MULTIPLY || op == DIVIDE)
    {
        ParseMultOp();
        ParseTerm();

        if(op == MULTIPLY) _Emit(I_MULT); else _Emit(I_DIV);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                                                   */
/*                                                                          */
/*       <Term> :== ["-"] <SubTerm>                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseTerm(void)
{
    int negative_flag = 0;

    if (CurrentToken.code == SUBTRACT){
        negative_flag = 1;
        Accept(SUBTRACT);
    }
    ParseSubTerm();

    if(negative_flag) _Emit(I_NEG);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                                                */
/*                                                                          */
/*       <SubTerm> :== <Variable> | <IntConst> | "(" <Expression> ")"       */
/*                                                                          */
/*--------------------------------------------------------------------------*/



PRIVATE void ParseSubTerm(void)
{
    SYMBOL *var;
    switch(CurrentToken.code)
    {
        case IDENTIFIER:
        default:
        var = LookupSymbol();
        if(var != NULL && var->type == STYPE_VARIABLE) Emit(I_LOADA,var->address);
        else printf("Name undeclared or not a variable..!!");
        ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
        break;

        case INTCONST:
        Emit(I_LOADI,CurrentToken.value);
        ParseIntConst(); /* ParseIntConst() --> Accept(INTCONST) */
        break;

        case LEFTPARENTHESIS:Accept(LEFTPARENTHESIS); ParseExpression(); Accept(RIGHTPARENTHESIS); break;

    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*       <BooleanExpression> :== <Expression> <RelOp> <Expression>          */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE int ParseBooleanExpression(void)
{
    int BackPatchAddr, RelOpInstruction;
    ParseExpression();
    RelOpInstruction = ParseRelOp();
    ParseExpression();
    _Emit( I_SUB );
    ParseRelOp();
    BackPatchAddr = CurrentCodeAddress( ); 
    Emit( RelOpInstruction, 0 ); 
    return BackPatchAddr; 
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                                  */
/*                                                                          */
/*       <AddOp> :== "+" | "-"                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseAddOp(void)
{
    switch(CurrentToken.code)
    {
        case ADD:  
        Accept(ADD); break;    /* Question: _Emit used here? */
        case SUBTRACT:  
        Accept(SUBTRACT); break;
        default:
        SyntaxError( IDENTIFIER, CurrentToken );
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                                 */
/*                                                                          */
/*       <MultOp> :== "*" | "/"                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseMultOp(void)
{
    switch(CurrentToken.code)
    {
        case MULTIPLY:Accept(MULTIPLY); break;
        case DIVIDE:Accept(DIVIDE); break;
        default:
        SyntaxError( IDENTIFIER, CurrentToken );
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                                  */
/*                                                                          */
/*       <RelOp> :== "=" | "<=" | ">=" | "<" | ">"                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE int ParseRelOp(void)
{
    int RelOpInstruction;
    switch(CurrentToken.code)
    {
        case EQUALITY: RelOpInstruction = I_BZ; Accept(EQUALITY); break;
        case LESSEQUAL: RelOpInstruction = I_BG; Accept(LESSEQUAL); break;
        case GREATEREQUAL: RelOpInstruction = I_BL; Accept(GREATEREQUAL); break;
        case LESS: RelOpInstruction = I_BGZ; Accept(LESS); break;
        case GREATER: RelOpInstruction = I_BLZ; Accept(GREATER); break;

    }
    return RelOpInstruction;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseVariable implements:                                               */
/*                                                                          */
/*       <Variable> :== <Identifier>                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseVariable(void)
{
    ParseIdentifier();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIntConst implements:                                               */
/*                                                                          */
/*       <IntConst> :== <Digit> { <Digit> }                                 */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseIntConst(void)
{
    Accept(INTCONST);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIdentifier implements:                                             */
/*                                                                          */
/*       <Identifier> :== <Alpha> { <AlphaNum> }                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/


PRIVATE void ParseIdentifier(void)
{
    Accept(IDENTIFIER);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept( int ExpectedToken )
{
    static int recovering = 0;

    if(recovering){
        while(CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT)
            CurrentToken = GetToken();
        recovering = 0;
    }

    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        recovering = 1;
    }
    else  CurrentToken = GetToken();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*                       S-Algol Error Recovery                             */
/*                                                                          */
/*  The Synchronise() function with two Arguments                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Synchronise(SET *F, SET *FB){

    SET S;

    S = Union(2,F,FB);
    if(!InSet(F,CurrentToken.code)){
        SyntaxError2(*F,CurrentToken);
        while(!InSet(&S,CurrentToken.code)){
            CurrentToken = GetToken();
        }
    }
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*                       S-Algol Error Recovery                             */
/*                                                                          */
/*  SetupSets() function                                                    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void SetupSets(void){

    /*Program First (1)*/
    ClearSet(&ProgramStatementFS_aug1);
    AddElements(&ProgramStatementFS_aug1,3,VAR,PROCEDURE,BEGIN);
    /*Program First (2)*/
    ClearSet(&ProgramStatementFS_aug2);
    AddElements(&ProgramStatementFS_aug2,2,PROCEDURE,BEGIN);
    /*Program Follow + Beacon*/
    ClearSet(&ProgramStatementFBS);
    AddElements(&ProgramStatementFBS,3,END,ENDOFPROGRAM,ENDOFINPUT);

    /*Procedure First (1)*/
    ClearSet(&ProcedureStatementFS_aug1);
    AddElements(&ProcedureStatementFS_aug1,3,VAR,PROCEDURE,BEGIN);
    /*Procedure First (2)*/
    ClearSet(&ProcedureStatementFS_aug2);
    AddElements(&ProcedureStatementFS_aug2,2,PROCEDURE,BEGIN);
    /*Procedure Follow + Beacon*/
    ClearSet(&ProcedureStatementFBS);
    AddElements(&ProcedureStatementFBS,3,END,ENDOFPROGRAM,ENDOFINPUT);

    /*Block First*/
    ClearSet(&BlockStatementFS_aug);
    AddElements(&BlockStatementFS_aug,6,END,IDENTIFIER,WHILE,IF,READ,WRITE);
    /*Block Follow + Beacons*/
    ClearSet(&BlockStatementFBS);
    AddElements(&BlockStatementFBS,4,SEMICOLON,ELSE,ENDOFPROGRAM,ENDOFINPUT);

}




/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine mmodifies the globals "InputFile" and          */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] )
{


    if ( argc != 4 )  {
        fprintf( stderr, "%s <inputfile> <listfile>\n", argv[0] );
        return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
        return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
        fclose( InputFile );
        return 0;
    }

    if ( NULL == ( CodeFile = fopen( argv[3], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[3] );
        fclose( InputFile );
        fclose( ListFile );
        return 0;
    }

    return 1;
}









PRIVATE void MakeSymbolTableEntry( int symtype )
{
   /*〈Variable Declarations here〉*/
    SYMBOL *newsptr;
    SYMBOL *oldsptr;
    char *cptr;
    int hashindex;
    static int varaddress = 0;

    if( CurrentToken.code == IDENTIFIER )
    {
       if ( NULL == ( oldsptr = Probe( CurrentToken.s, &hashindex )) || oldsptr->scope < scope ) {
           if ( oldsptr == NULL ) cptr = CurrentToken.s; else cptr = oldsptr->s;
           if ( NULL == ( newsptr = EnterSymbol( cptr, hashindex ))) {
               /*〈Fatal internal error in EnterSymbol, compiler must exit: code for this goes here〉*/
               printf("Fatal internal error in EnterSymbol..!!\n");
               exit(EXIT_FAILURE);
           }
           else {
               if ( oldsptr == NULL ) PreserveString();
               newsptr->scope = scope;
               newsptr->type = symtype;
               if ( symtype == STYPE_VARIABLE ) {
                   newsptr->address = varaddress; varaddress++;
               }
               else newsptr->address = -1;
           }
       }
       else { /*〈Error, variable already declared: code for this goes here〉*/
           printf("Error, variable already declared...!!\n");
       }
   }
}









PRIVATE SYMBOL *LookupSymbol(void){

    SYMBOL *sptr;

    if(CurrentToken.code == IDENTIFIER){
        sptr = Probe(CurrentToken.s,NULL);

        if(sptr == NULL){
            Error("Identifier not declared..",CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else sptr = NULL;
    return sptr;
}
