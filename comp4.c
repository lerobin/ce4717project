/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser1                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Dawid Denys            19269048                                */
/*           Eoin Moore             19274912                                */
/*           Simon Krahé            21180644                                */
/*                                                                          */
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"
#include "code.h"
#include "symbol.h"
#include "strtab.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;            /*  For generated assembly code.         */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */
PRIVATE int scope;
PRIVATE int varaddress;
PRIVATE SYMBOL *procedureNamesCalled[100];


PRIVATE SYMBOL *MakeSymbolTableEntry( int symtype, int *varaddress );
PRIVATE SYMBOL *LookupSymbol();
/*------------------------------SYMBOL--------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );

PRIVATE void ParseProgram( void );
PRIVATE void ParseDeclarations( void );
PRIVATE void ParseProcDeclaration( void );
PRIVATE void ParseParameterList( void );
SYMBOL* ParseFormalParameter( void );
PRIVATE void ParseBlock( void );
PRIVATE void ParseStatement( void );
PRIVATE void ParseSimpleStatement( void );
PRIVATE void ParseRestOfStatement( SYMBOL *target );
PRIVATE void ParseProcCallList( SYMBOL *target );
PRIVATE void ParseAssignment( void );
PRIVATE void ParseActualParameter( void );
PRIVATE void ParseWhileStatement( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void ParseReadStatement( void);
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseExpression( void );
PRIVATE void ParseCompoundTerm( void );
PRIVATE void ParseTerm( void );
PRIVATE void ParseSubTerm( void );
PRIVATE int  ParseBooleanExpression( void );
PRIVATE void ParseAddOp( void );
PRIVATE void ParseMultOp( void );
PRIVATE int  ParseRelOp( void );
PRIVATE void ParseVariable( void );
PRIVATE void ParseVarOrProcName( void );
PRIVATE void Synchronise(SET *F, SET *FB);



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* declaring sets in advance */
SET *ProgramSet1, *ProgramSet2, *ProgramSetFollowers, *ProgramSetBeacons, *ProcDeclarationSet1, *ProcDeclarationSet2, *ProcDeclarationSetBeacons, *BlockSet1_aug, *BlockSetFollowers, *BlockSetBeacons;
SET *ProgramSetFBS, *BlockSetFBS;
SET temp1, temp2;

PRIVATE void SetupSets(void){
    /* <Program> :== "PROGRAM" <Identifier> ";" [<Declarations>] { <ProcDeclaration> } <Block> "." */
    ProgramSet1 = MakeSet();
    InitSet(ProgramSet1, 3, VAR, PROCEDURE, BEGIN);
    ProgramSet2 = MakeSet();
    InitSet(ProgramSet2, 2, PROCEDURE, BEGIN);
    ProgramSetFollowers = MakeSet();
    InitSet(ProgramSetFollowers, 1, ENDOFPROGRAM);
    ProgramSetBeacons = MakeSet();
    InitSet(ProgramSetBeacons, 2, END, ENDOFINPUT);
    temp1 = Union(2, ProgramSetFollowers, ProgramSetBeacons);
    ProgramSetFBS = &temp1;

    ProcDeclarationSet1 = MakeSet();
    InitSet(ProcDeclarationSet1, 3, VAR, PROCEDURE, BEGIN);
    ProcDeclarationSet2 = MakeSet();
    InitSet(ProcDeclarationSet2, 2, PROCEDURE, BEGIN);
    ProcDeclarationSetBeacons = MakeSet();
    InitSet(ProcDeclarationSetBeacons, 3, ENDOFINPUT, ENDOFPROGRAM, END);

    BlockSet1_aug = MakeSet();
    InitSet(BlockSet1_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);
    BlockSetFollowers = MakeSet();
    InitSet(BlockSetFollowers, 3, ENDOFINPUT, SEMICOLON, ELSE);
    BlockSetBeacons = MakeSet();
    InitSet(BlockSetBeacons, 1, ENDOFINPUT);
    temp2 = Union(2, BlockSetFollowers, BlockSetBeacons);
    BlockSetFBS = &temp2;
}

PUBLIC int main ( int argc, char *argv[] )
{
    SetupSets();
    if ( OpenFiles( argc, argv ) )  {
        InitCodeGenerator(CodeFile);
        InitCharProcessor( InputFile, ListFile );
        CurrentToken = GetToken();
        ParseProgram();
        WriteCodeFile();
        fclose( InputFile );
        fclose( ListFile );
        return  EXIT_SUCCESS;
    }
    else 
        return EXIT_FAILURE;
}


/* ParseProgram */
/* <Program> :== "PROGRAM" <Identifier> ";" [<Declarations>] { <ProcDeclaration> } <Block> "." */

PRIVATE void ParseProgram( void )
{
    Accept( PROGRAM );
    MakeSymbolTableEntry(STYPE_PROGRAM, &varaddress);
    Accept( IDENTIFIER );
    Accept( SEMICOLON );
    Synchronise(ProgramSet1, ProgramSetFBS);
    
    if ( CurrentToken.code == VAR ){
        ParseDeclarations();

    }
    Synchronise(ProgramSet2, ProgramSetFBS);

    while ( CurrentToken.code == PROCEDURE ) {
        ParseProcDeclaration();
        Synchronise(ProgramSet2, ProgramSetFBS);
    }

    ParseBlock();
    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
}


/* ParseDeclarations */
/* <Declarations> :== "VAR" <Variable> { "," <Variable> } ";" */
PRIVATE void ParseDeclarations() {
    Accept( VAR );


    MakeSymbolTableEntry( STYPE_VARIABLE, &varaddress );
    ParseVariable();
    while ( CurrentToken.code == COMMA )  {
        Accept( COMMA );

        MakeSymbolTableEntry( STYPE_VARIABLE, &varaddress );
        ParseVariable();
    }
    Accept ( SEMICOLON );
}

/* ParseProcDeclaration */
/* <ProcDeclaration> :== "PROCEDURE" <Identifier> [ <ParameterList> ] ";" [ <Declarations> ] { <ProcDeclaration> } <Block> ";" */
PRIVATE void ParseProcDeclaration() {
    int backpatch_addr;
    SYMBOL *procedure;


    Accept( PROCEDURE );
    procedure = MakeSymbolTableEntry(STYPE_PROCEDURE, &varaddress);
    printf("test1\n");
    Accept( IDENTIFIER );
    printf("test2\n");
    backpatch_addr = CurrentCodeAddress();
    Emit( I_BR, 0);
    procedure->address = CurrentCodeAddress();
    scope++;
    procedureNamesCalled[scope] = procedure;
    

    printf("test3\n");
    if ( CurrentToken.code == LEFTPARENTHESIS ) {
        printf("test4\n");
        ParseParameterList();
        printf("test5\n");
    }
    Accept( SEMICOLON );
    Synchronise(ProcDeclarationSet1,ProcDeclarationSetBeacons);
    if ( CurrentToken.code == VAR ) {
        ParseDeclarations();
    }
    Synchronise(ProcDeclarationSet2,ProcDeclarationSetBeacons);
    while ( CurrentToken.code == PROCEDURE )  {
        ParseProcDeclaration();
        Synchronise(ProcDeclarationSet2,ProcDeclarationSetBeacons);
    }
    ParseBlock();
    Accept( SEMICOLON );
    _Emit( I_RET );
    BackPatch( backpatch_addr, CurrentCodeAddress());
    RemoveSymbols(scope);
    scope--;
   
}

/* ParseParameterList */
/* <ParameterList> :== "(" <FormalParameter> { "," <FormalParameter> } ")" */
PRIVATE void ParseParameterList() {
    SYMBOL* stack[100];
    int stackI;
    /* int i; */
    stackI = 0;
    Accept( LEFTPARENTHESIS );
    printf("test6\n");
    stack[stackI] = ParseFormalParameter();
    stackI++;
    printf("test7\n");
    while ( CurrentToken.code == COMMA )  {
        printf("test8\n");
        Accept(COMMA);
        printf("test9\n");
        stack[stackI] = ParseFormalParameter();
        stackI++;
        printf("test10\n");
    }
    Accept( RIGHTPARENTHESIS );
    for(int i=0; i<= stackI; i++){
        int offset = (stackI - i) * -1 -1;
        BackPatch(stack[i]->address, CurrentCodeAddress() - offset);
    }
}

/* FormalParameter */
/* <FormalParameter> :== [ "REF"], <Variable> */
SYMBOL* ParseFormalParameter() {
    SYMBOL* temp;
    printf("new1\n");
    if (CurrentToken.code == REF){
        printf("new2\n");
        MakeSymbolTableEntry( STYPE_REFPAR, &varaddress );
        printf("new3\n");
        Accept(REF);
        printf("new4\n");
    }
    printf("new5\n");
    ParseExpression();
    temp = MakeSymbolTableEntry( STYPE_VARIABLE, &varaddress );
    printf("new6\n");
    ParseVariable();
    printf("new7\n");
    return temp;
}

/* ParseBlock */
/* <Block> :== "BEGIN" { <Statement> ";"} "END" */
PRIVATE void ParseBlock(){
    Accept( BEGIN );
    Synchronise(BlockSet1_aug, BlockSetFBS);
    while ( CurrentToken.code == IDENTIFIER || CurrentToken.code == WHILE || CurrentToken. code == IF || CurrentToken.code == READ || CurrentToken.code == WRITE)  {
        ParseStatement();
        Accept ( SEMICOLON );
        Synchronise(BlockSet1_aug, BlockSetFBS);
    }
    Accept( END );
}

/* ParseStatement */
/* <Statement> :== <SimpleStatement> | <WhileStatement> | <IfStatement> | <ReadStatement> | <WriteStatement> */
PRIVATE void ParseStatement() {
    switch (CurrentToken.code)
    {
    default:
    case IDENTIFIER:
        ParseSimpleStatement();
        break;
    case WHILE:
        ParseWhileStatement();
        break;
    case IF:
        ParseIfStatement();
        break;
    case READ:
        ParseReadStatement();
        break;
    case WRITE:
        ParseWriteStatement();
        break;
    }
}

/* ParseSimpleStatement */
/* <VarOrProcName> <RestOfStatement> */
PRIVATE void ParseSimpleStatement(){
    SYMBOL *target;
    target = LookupSymbol();

    ParseVarOrProcName();
    ParseRestOfStatement(target);
}

/* <RestOfStatement>  :== <ProcCallList> | <Assignment> | ENDOFINPUT */
PRIVATE void ParseRestOfStatement(SYMBOL* target){
    int sigma;
    int i;
    int j = scope - 1;
    int boolFound = 0;

    if (scope == 0){
        sigma = 0;
    }
    else{
        printf("Finding sigma\n");
        for(; j>-1 && boolFound==0; j--){
            SYMBOL* target2 = target;
            printf("Got this far\n");
            char* pointer = target->s;
            printf("target character: %c", *pointer);
            printf("procedure name called: %p", procedureNamesCalled[j]->s);
            if (strcmp(target->s, procedureNamesCalled[j]->s)){
                printf("Sigma found\n");
                boolFound = 1;
                sigma = scope - (j); /*note: doublecheck this when possible with execution*/
            }
        }
        if(boolFound == 0){
            sigma = 0;
        }
    }

    switch (CurrentToken.code){
    case LEFTPARENTHESIS:
        ParseProcCallList( target ) ;
    case SEMICOLON: /*TODO Fix this in parsers*/
        if (target != NULL && target->type == STYPE_PROCEDURE){
            if(sigma == 0){
                _Emit( I_PUSHFP );
            }
            else{
                _Emit( I_LOADFP );
                
                printf("Sigma: %d\n", sigma);
                for(i=sigma; i-1<2; i++){
                    _Emit( I_LOADSP);
                }
            }
            _Emit( I_BSF );
            Emit( I_CALL, target->address );
            _Emit ( I_RSF );
        }
        else{
            /*TODO Better errors*/
            printf("Error: Not a procedure\n");
            KillCodeGeneration();
        }
        break;
    default:
    case ASSIGNMENT:
        ParseAssignment();
        if( target != NULL && target->type == STYPE_VARIABLE){
            Emit( I_STOREA, target->address);
        }
        else{
            /*TODO Better errors*/
            printf("Error: Undeclared Variable\n");
            KillCodeGeneration();
        }
        break;
    }
}

/* ParseProcCallList */
/* <ProcCallList> :== "(" <ActualParameter> { "," <ActualParameter> } ")" */
PRIVATE void ParseProcCallList(SYMBOL *target) {
    Accept( LEFTPARENTHESIS );
    ParseActualParameter();
    while (CurrentToken.code == COMMA){
        Accept( COMMA );
        ParseActualParameter();
    }
    Accept( RIGHTPARENTHESIS );
}

/* ParseAssignment */
/* <Assignment> :== ":==" <Expression> */
PRIVATE void ParseAssignment(){
    Accept ( ASSIGNMENT );
    ParseExpression();
}

/* ParseActualParameter */
/* <ActualParameter> :== <Variable> | <Expression> */
/*<Expression> -> <CompoundTerm> -> <SubTerm> -> <Variable> -> <Identifier> */
/*TODO look at this again? */
PRIVATE void ParseActualParameter(){
    ParseExpression();
    
    switch (CurrentToken.code)
    {
    case IDENTIFIER:
        ParseVariable();
        break;
    case SUBTRACT:
    /* more cases here if it is expression -> this can lead up to identifier again, how do we disambiguate? */
    default:
        break;
    }
}

/* ParseWhileStatement */
/* <WhileStatement> :== "WHILE" <BooleanExpression> "DO" <Block> */
PRIVATE void ParseWhileStatement() {
    int Label1, Label2, L2BackPatchLoc;
    
    Accept( WHILE );
    Label1 = CurrentCodeAddress();
    L2BackPatchLoc = ParseBooleanExpression();
    Accept( DO );
    ParseBlock();
    Emit(I_BR, Label1);
    Label2 = CurrentCodeAddress();
    BackPatch(L2BackPatchLoc,Label2);
}

/* ParseIfStatement */
/* <IfStatement> :== "IF" <BooleanExpression> "THEN" <Block> [ "ELSE" <Block> ] */
PRIVATE void ParseIfStatement() {
    int Label1, Label2, L1BackPatchLoc, L2BackPatchLoc;

    Accept( IF );
    L1BackPatchLoc = ParseBooleanExpression();
    Accept( THEN );
    ParseBlock();

    if (CurrentToken.code == ELSE ) {
        /*end of then block points to end of if statement*/
        L2BackPatchLoc = CurrentCodeAddress();
        Emit( I_BR , 9999);

        /*boolean is false, entering else block*/
        Label1 = CurrentCodeAddress();
        BackPatch(L1BackPatchLoc,Label1);

        /*actual else block*/
        Accept( ELSE );
        ParseBlock();

        /*End of else block, label2 address assigned to L2BackPatchLoc*/
        Label2 = CurrentCodeAddress();
        BackPatch(L2BackPatchLoc, Label2);
    }
    else{
        /*boolean statement is false -> points to the end of the if statement*/
        Label1 = CurrentCodeAddress();
        BackPatch(L1BackPatchLoc,Label1);
    }


}

/* ParseReadStatement */
/* <ReadStatement> :== "READ" "(" <Variable> { "," <Expression> } ")" */
PRIVATE void ParseReadStatement() {
    Accept ( READ );
    Accept ( LEFTPARENTHESIS );
    ParseVariable();
    while(CurrentToken.code == COMMA ){
        Accept( COMMA );
        ParseVariable();
    }
    Accept ( RIGHTPARENTHESIS );
}

/* ParseWriteStatement */
/* <WriteStatement> :== "WRITE" "(" <Expression> { "," <Expression> } */
PRIVATE void ParseWriteStatement() {
    Accept( WRITE );
    Accept( LEFTPARENTHESIS );
    ParseExpression();
    _Emit(I_WRITE);
    while (CurrentToken.code == COMMA ) {
        Accept( COMMA );
        ParseExpression();
        _Emit(I_WRITE);
    }
    Accept( RIGHTPARENTHESIS );
}

/* <expression> :== <CompoundTerm> {<AddOp> <CompoundTerm>} */
PRIVATE void ParseExpression() {
    int op;

    ParseCompoundTerm();
    while ((op = CurrentToken.code) == ADD || CurrentToken.code == SUBTRACT ){
        ParseAddOp();
        ParseCompoundTerm();
        if(op == ADD){
            _Emit( I_ADD );
        }
        else{
            _Emit( I_SUB );
        }
    }
}

/* ParseCompoundTerm */
/* <CompoundTerm> :== <Term> { <MultOp> <Term> } */
PRIVATE void ParseCompoundTerm() {
    int op;
    
    ParseTerm();
    while ((op = CurrentToken.code) == MULTIPLY || op == DIVIDE ) {
        ParseMultOp();
        ParseTerm();

        if (op == MULTIPLY ) _Emit( I_MULT ); else _Emit( I_DIV );
    }
}

/* ParseTerm */
/* <Term> :== [ "-" ] <SubTerm> */
PRIVATE void ParseTerm() {
    int negateflag = 0;
    if (CurrentToken.code == SUBTRACT) {
        negateflag = 1;
        Accept( SUBTRACT );
    }
    ParseSubTerm();
    if(negateflag)
        _Emit( I_NEG );
}


/* ParseSubTerm */
/* <SubTerm> :== <Variable> | <IntConst> | "(" <Expression> ")"  */
PRIVATE void ParseSubTerm() {
    int i, dS;
    SYMBOL *var;

    switch (CurrentToken.code) {
        default:
        case IDENTIFIER: /*Variable */
            var = LookupSymbol();
            if (var != NULL && var->type == STYPE_VARIABLE){
                Emit( I_LOADA, var->address);    
            }
            else if (var->type ==STYPE_LOCALVAR) {
                dS = scope - var->scope;
                if (dS == 0) {
                    Emit(I_LOADFP, var->address);
                } else {
                    _Emit(I_LOADFP);
                    for (i = 0; i < dS-1; i++) {
                        _Emit(I_LOADSP);
                    }
                Emit(I_LOADSP, var->address);   
                }
            }
            else{
                printf("Temporary error: name undeclared or not a variable in ParseSubTerm\n");
                /*error handling: name undeclared or not a variable
                to be handled in comp2.c*/
            }
            Accept( IDENTIFIER );
            break;
        case INTCONST:
        Emit( I_LOADI, CurrentToken.value);
            Accept(INTCONST);
            break;
        case LEFTPARENTHESIS:
            Accept( LEFTPARENTHESIS );
            ParseExpression();
            Accept( RIGHTPARENTHESIS );
            break;
    }
}

/* ParseBooleanExpression */
/* <BooleanExpression> :== <Expression> <RelOp> <Expression> */
PRIVATE int ParseBooleanExpression() {
    int BackPatchAddr, RelOpInstruction;

    ParseExpression();
    RelOpInstruction = ParseRelOp();
    ParseExpression();
    _Emit( I_SUB );
    BackPatchAddr = CurrentCodeAddress();
    Emit(RelOpInstruction, 9999);
    return BackPatchAddr;
}

/* ParseAddOp */
/* <AddOp> :== "+" | "-" */
PRIVATE void ParseAddOp() {
    if (CurrentToken.code == ADD ) {
        Accept(ADD);
    } else if (CurrentToken.code == SUBTRACT) {
        Accept(SUBTRACT);
    }
}

/* MultOp */
/* <MultOP> = <MULTIPLY> | <DIVIDE> */
PRIVATE void ParseMultOp(){
     if (CurrentToken.code == MULTIPLY ) {
        Accept(MULTIPLY);
    } else if (CurrentToken.code == DIVIDE) {
        Accept(DIVIDE);
    }
}

/* ParseRelOp */
/* <RelOp> :== "=" | "<=" | ">=" | "<" | ">" */
PRIVATE int ParseRelOp() {
int RelOpInstruction;

    switch(CurrentToken.code) {
        default:
        case EQUALITY:
            Accept( EQUALITY );
            break;
        case LESSEQUAL:
            RelOpInstruction = I_BG; Accept( LESSEQUAL );
            break;
        case GREATEREQUAL:
            RelOpInstruction = I_BL; Accept( GREATEREQUAL );
            break;
        case LESS:
            RelOpInstruction = I_BGZ; Accept( LESS );
            break;
        case GREATER:
            RelOpInstruction = I_BLZ; Accept( GREATER );
            break;
    }

    return RelOpInstruction;
}



/* ParseVariable */
/* <Variable> :== <Identifier> */
PRIVATE void ParseVariable(){
    
    Accept( IDENTIFIER );
}

/* ParseVarOrProcName */
/* <VarOrProcName> :== <Identifier> */
PRIVATE void ParseVarOrProcName() {
    Accept( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of parser.  Support routines follow.                                */
/*                                                                          */
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
    if (recovering){
        while (CurrentToken.code != ExpectedToken && CurrentToken.code != ENDOFINPUT){
            CurrentToken = GetToken();
        }
        recovering = 0;
    }
    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        recovering = 1;
    }
    else  CurrentToken = GetToken();
}



PRIVATE void Synchronise(SET *F, SET *FB) {
    SET S;

    S = Union(2, F, FB);
    if(!InSet(F, CurrentToken.code)) {
        SyntaxError2(*F, CurrentToken);
        while(!InSet(&S, CurrentToken.code)) {
            CurrentToken = GetToken();
        }
    }
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
        fprintf( stderr, "%s <inputfile> <listfile> <outputfile>\n", argv[0] );
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

PRIVATE SYMBOL *MakeSymbolTableEntry( int symtype, int *varaddress )
{
    SYMBOL *newSymbolPointer; /*new symbol pointer*/
    SYMBOL *oldSymbolPointer; /*old symbol pointer*/
    char *currentPointer;      /*current pointer*/
    int hashIndex;
    /*    static int varaddress = 0; */

    if( CurrentToken.code == IDENTIFIER )
{      /* check to see if there is an entry in the symbol table with the same name as IDENTIFIER */
       oldSymbolPointer = Probe( CurrentToken.s, &hashIndex );
       if ( NULL == oldSymbolPointer || oldSymbolPointer->scope < scope )
       {
           if ( oldSymbolPointer == NULL ){ 
               currentPointer = CurrentToken.s;
               }
            else {
                currentPointer = oldSymbolPointer->s;
            }
            newSymbolPointer = EnterSymbol(currentPointer, hashIndex);
            if ( newSymbolPointer == NULL ) {
                Error("Error: Fatal internal error in EnterSymbol\n", CurrentToken.pos);
                exit(EXIT_FAILURE);
            }
            else {
                if ( oldSymbolPointer == NULL ) {
                    PreserveString();
                }
                newSymbolPointer->scope = scope;
                newSymbolPointer->type = symtype;
                if ( symtype == STYPE_VARIABLE || symtype == STYPE_LOCALVAR ) {
                    newSymbolPointer->address = *varaddress;
                    (*varaddress)++;
                }
                else {
                    newSymbolPointer->address = -1;
                }
            }
        }
        else {
            Error("Error: Variable is already declared in this scope\n", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else {
        Error("Error: Current token is not of type identifier\n", CurrentToken.pos);
        KillCodeGeneration();
    }

    return newSymbolPointer;
    
}

PRIVATE SYMBOL *LookupSymbol(void){

    SYMBOL *symbolPointer;
    if(CurrentToken.code == IDENTIFIER){
        symbolPointer = Probe(CurrentToken.s, NULL);
        symbolPointer->s = CurrentToken.s;
        if(symbolPointer == NULL){
            Error("Identifier was not declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else{
        symbolPointer = NULL;
    } 
    return symbolPointer;
}