/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp2                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*       Ronan Randles            19242441                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/global.h" /* "header/ can be removed later" */
#include "headers/scanner.h"
#include "headers/line.h"
#include "headers/symbol.h"
#include "headers/code.h"
#include "headers/sets.h"
#include "headers/strtab.h"


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;
PRIVATE int writing = 0;
PRIVATE int reading = 0;
PRIVATE int ErrorFlag = 0;

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */
static int scope = 1;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles(int argc, char *argv[]);
PRIVATE void Accept(int code);
PRIVATE void Synchronise(SET *F, SET *FB);
PRIVATE void SetupSets(void);
PRIVATE SYMBOL *MakeSymbolTableEntry(int symtype);
PRIVATE SYMBOL *LookupSymbol();

PRIVATE SET ProgramFS_aug1;
PRIVATE SET ProgramFS_aug2;
PRIVATE SET ProgramFBS_aug;
PRIVATE SET ProcDeclarationFS_aug1;
PRIVATE SET ProcDeclarationFS_aug2;
PRIVATE SET ProcDeclarationFBS_aug;
PRIVATE SET BlockFS_aug;
PRIVATE SET BlockFBS_aug;

PRIVATE void ParseProgram(void);
PRIVATE int ParseDeclarations(void);
PRIVATE void ParseProcDeclaration(void);
PRIVATE void ParseParameterList(SYMBOL *target); /*Passing target allows setting
                                                   of pcount*/
PRIVATE void ParseFormalParameter(SYMBOL *target);/*Passing target allows
                                                    setting of ptypes*/
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestOfStatement(SYMBOL *target);
PRIVATE void ParseProcCallList(SYMBOL *target);/*Passing target allows checking
                                                 pcount and ptypes of called
                                                 function*/
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
PRIVATE int GetPtype(int ptype, int index); /*Helper function to check procedure
                                              ptypes*/
PRIVATE int CheckSuffix(int argc, char *argv[]); /*Helper function to check file
                                                   suffixes*/


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: parser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main (int argc, char *argv[])
{
    if (OpenFiles(argc, argv))  {

        InitCharProcessor(InputFile, ListFile);/*Initialise input processing*/
        InitCodeGenerator(CodeFile); /*Initialise code generation*/
        CurrentToken = GetToken();
        SetupSets();
        ParseProgram();
        WriteCodeFile(); /*Write to assembly file*/
        fclose(InputFile);
        fclose(ListFile);
        if (!ErrorFlag) {
            printf("Valid\n");
            return EXIT_SUCCESS; /*print valid and exit success*/
        } else
            return EXIT_FAILURE; /*exit failure, no "invalid" print because
                                   error prints will be present*/

    }
    else {
        return EXIT_FAILURE; /*impropper command line inputs, Openfiles
                               -> CheckSuffix will give feedback to user*/
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
/*       <Program〉 :== “PROGRAM” 〈Identifier 〉 “;”                        */
/*                [ 〈Declarations〉 ] { 〈ProcDeclaration〉 } 〈Block 〉 “.” */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProgram(void)
{
    Accept(PROGRAM);
    MakeSymbolTableEntry(STYPE_PROGRAM);
    Accept(IDENTIFIER);
    Accept(SEMICOLON);

    Synchronise(&ProgramFS_aug1, &ProgramFBS_aug);
    if (CurrentToken.code == VAR)
        ParseDeclarations();
    Synchronise(&ProgramFS_aug2, &ProgramFBS_aug);
    while (CurrentToken.code == PROCEDURE)
        ParseProcDeclaration();
    Synchronise(&ProgramFS_aug2, &ProgramFBS_aug);

    ParseBlock();
    Accept(ENDOFPROGRAM);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseDeclarations implements:                                           */
/*                                                                          */
/*  〈Declarations〉 :== “VAR” 〈Variable〉 { “,” 〈Variable〉 } “;”          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int ParseDeclarations(void)
{
    int vcount = 0;
    Accept(VAR);
    /* Do while used to reduce duplicate code*/
    do {
        if (CurrentToken.code == COMMA)
            Accept(COMMA);
        MakeSymbolTableEntry(STYPE_VARIABLE);
        Accept(IDENTIFIER);
        vcount++;
    } while (CurrentToken.code == COMMA);

    Accept(SEMICOLON);
    Emit(I_INC, vcount);
    return vcount;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcDeclaration implements:                                        */
/*                                                                          */
/*  <ProcDeclaration〉 :== “PROCEDURE” 〈Identifier                          */
/*    [ 〈ParameterList〉 ] “;” [ 〈Declarations〉 ] { 〈ProcDeclaration〉 }  */
/*    〈Block 〉 “;”                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcDeclaration(void)
{
    int vcount = 0;
    int backpatch_addr;
    SYMBOL *procedure;
    Accept(PROCEDURE);
    procedure = MakeSymbolTableEntry(STYPE_PROCEDURE);
    Accept(IDENTIFIER);
    backpatch_addr = CurrentCodeAddress();
    Emit(I_BR,0);
    procedure->address = CurrentCodeAddress();
    scope++;
    if (CurrentToken.code == LEFTPARENTHESIS)
        ParseParameterList(procedure);

    Accept(SEMICOLON);
    Synchronise(&ProcDeclarationFS_aug1, &ProcDeclarationFBS_aug);
    if (CurrentToken.code == VAR)
        vcount = ParseDeclarations();

    Synchronise(&ProcDeclarationFS_aug2, &ProcDeclarationFBS_aug);
    while (CurrentToken.code == PROCEDURE) {
        ParseProcDeclaration();
        Synchronise(&ProcDeclarationFS_aug2, &ProcDeclarationFBS_aug);
    }

    ParseBlock();
    Accept(SEMICOLON);
    Emit(I_DEC, vcount);

    _Emit(I_RET);
    BackPatch(backpatch_addr, CurrentCodeAddress());
    /*DumpSymbols(0);*/ /*Uncomment to dump symbols */
    RemoveSymbols(scope);
    scope--;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseParameterList implements:                                          */
/*                                                                          */
/*  〈ParameterList〉 :== “(” 〈FormalParameter                              */
/*                        { “,” 〈FormalParameter 〉 } “)”                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseParameterList(SYMBOL *target)
{
    Accept(LEFTPARENTHESIS);
    if (CurrentToken.code != 0)
    target->pcount = 1;
    target->ptypes = 0;
    ParseFormalParameter(target);

    while (CurrentToken.code == COMMA) {
        Accept(COMMA);
        target->pcount++;
        ParseFormalParameter(target);
    }
    Accept(RIGHTPARENTHESIS);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseFormalParameter implements:                                        */
/*                                                                          */
/*  〈FormalParameter 〉 :== [ “REF” ] 〈Variable〉                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseFormalParameter(SYMBOL *target)
{
    int shiftVal;

    if (CurrentToken.code == REF) {
        Accept(REF); /* Consume "REF" term */
        MakeSymbolTableEntry(STYPE_REFPAR); /* Set following symbol to type
                                               REFPAR */
        shiftVal = (STYPE_REFPAR << (3 * (target->pcount - 1)));
        Accept(IDENTIFIER);
    }else {
        MakeSymbolTableEntry(STYPE_VARIABLE);
        shiftVal = (STYPE_VARIABLE << (3 * (target->pcount - 1)));
        Accept(IDENTIFIER);
    }
    /* Bitshift to store multiple pytpes in one 32 bit int */
    /*Ensure that each value takes up space of 0x7 (3 binary digits)
      for future extraction. This is suitable here since there are only 7 stypes
      => n binary digits needed to differentiate between (2^n - 1) stypes
      => 32 bit ptype field will support up to 10 parmas stored in ptypes int*/
    target->ptypes = target->ptypes | shiftVal;

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBlock implements:                                                  */
/*                                                                          */
/*  〈Block 〉 :== “BEGIN” { 〈Statement〉 “;” } “END”                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseBlock(void)
{
    Accept(BEGIN);
    Synchronise(&BlockFS_aug, &BlockFBS_aug);
    while (CurrentToken.code == IDENTIFIER || CurrentToken.code == WHILE ||
            CurrentToken.code == IF || CurrentToken.code == READ ||
            CurrentToken.code == WRITE) {
        ParseStatement();
        Accept(SEMICOLON);
        Synchronise(&BlockFS_aug, &BlockFBS_aug);
    }

    Accept(END);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                              */
/*                                                                          */
/*       <Statement〉 :== 〈SimpleStatement〉 | 〈WhileStatement〉 |          */
/*                    〈IfStatement〉 |〈ReadStatement〉 | 〈WriteStatement〉 */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseStatement(void)
{
    switch (CurrentToken.code) {
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
    default:
        break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSimpleStatement implements:                                        */
/*                                                                          */
/*       〈SimpleStatement〉 :== 〈VarOrProcName〉 〈RestOfStatement〉        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement(void)
{
    SYMBOL *target;
    target = LookupSymbol();
    Accept(IDENTIFIER);
    if (target)
        ParseRestOfStatement(target);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                        */
/*                                                                          */
/*       〈RestOfStatement〉 :== 〈ProcCallList〉 | 〈Assignment〉 | \eps     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseRestOfStatement(SYMBOL *target)
{
    switch(CurrentToken.code) {
        case LEFTPARENTHESIS:
            ParseProcCallList(target);
        case SEMICOLON:
            if (target != NULL && target->type == STYPE_PROCEDURE) {
                _Emit(I_PUSHFP);
                _Emit(I_BSF);
                Emit(I_CALL, target->address);
                _Emit(I_RSF);
            }
            else {
                printf("Not a procedure\n");
                KillCodeGeneration();
                ErrorFlag = 1;
            }
            break;
        case ASSIGNMENT:
        default:
            ParseAssignment();
            if (target!= NULL &&
            (target->type == STYPE_VARIABLE || target->type == STYPE_REFPAR))

                Emit(I_STOREA, target->address);
            else {
                printf("Undeclared variable: %s\n", target->s);
                KillCodeGeneration();
                ErrorFlag = 1;
            }
            break;

    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcCallList implements:                                           */
/*                                                                          */
/*       〈ProcCallList〉 :== “(” 〈ActualParameter                          */
/*                            { “,” 〈ActualParameter 〉 } “)”               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseProcCallList(SYMBOL *target)
{
    int readParamCount = 0;
    Accept(LEFTPARENTHESIS);
    /* Do while used to reduce duplicate code*/
    do {
        if (CurrentToken.code == COMMA)
            Accept(COMMA);

        if(reading)
            _Emit(I_READ);

        if (GetPtype(target->ptypes,readParamCount) == 7){
            if (CurrentToken.code != IDENTIFIER) {
                printf("Expected identifier, killing code generation...\n");
                KillCodeGeneration();
                ErrorFlag = 1;
            }
            ParseActualParameter();
        }
        else if (GetPtype(target->ptypes,readParamCount) == 2)
            ParseExpression();

        readParamCount++;
        if(writing)
            _Emit(I_WRITE);

    }while(CurrentToken.code == COMMA);
    Accept(RIGHTPARENTHESIS);
}
/**
 * @brief Get the parameter type stored at given index of ptype field
 *
 * @param ptype ptype stored in procedure symbol
 *
 * @param index index of parameter type to be retrieved, starts at 0
 * @return Type retrieved
 */
PRIVATE int GetPtype(int ptype, int index)
{
    /*Shift desired set of 3 digits to 3 LSB's, & with 7 (binary 111) to
      take only 3 LSB's*/
    return ((ptype >> (3 * index)) & 7);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                           */
/*                                                                          */
/*       〈Assignment〉 :== “:=” 〈Expression〉                              */
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
/*       〈ActualParameter〉 :== 〈Variable〉 | 〈Expression〉                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter(void)
{
    if (CurrentToken.code == IDENTIFIER) {
        Accept(IDENTIFIER);
    }
    else
        ParseExpression();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatment implements:                                          */
/*                                                                          */
/*       <WhileStatement〉 :== “WHILE” 〈BooleanExpression〉 “DO” 〈Block 〉  */
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
    BackPatch(L2BackPatchLoc, Label2);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                                             */
/*                                                                          */
/*       〈IfStatement〉 :== “IF” 〈BooleanExpression〉 “THEN”               */
/*                            〈Block 〉 [ “ELSE” 〈Block 〉 ]               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement(void)
{
    Accept(IF);
    ParseBooleanExpression();
    Accept(THEN);
    ParseBlock();

    if (CurrentToken.code == ELSE) {
        Accept(ELSE);
        ParseBlock();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatment implements:                                           */
/*                                                                          */
/*       〈ReadStatement〉 :== “READ” “(” 〈Variable〉                       */
/*                            { “,” 〈Variable〉 } “)”                       */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement(void)
{
    reading = 1;
    Accept(READ);
    Accept(LEFTPARENTHESIS);
    Accept(IDENTIFIER);

    while (CurrentToken.code == COMMA) {
        Accept(COMMA);
        Accept(IDENTIFIER);
    }
    Accept(RIGHTPARENTHESIS);
    reading = 0;
}

/*--------------------------------------------------------------------------*/
/*                                                                           */
/*  ParseWriteStatment implements:                                           */
/*                                                                           */
/*       〈WriteStatement〉 :== “WRITE” “(” 〈Expression〉                    */
/*                            { “,” 〈Expression〉 } “)”                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement(void)
{
    writing = 1;
    Accept(WRITE);
    Accept(LEFTPARENTHESIS);
    ParseExpression();

    while (CurrentToken.code == COMMA) {
        Accept(COMMA);
        ParseExpression();
    }
    Accept(RIGHTPARENTHESIS);

}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseExpression implements:                                             */
/*                                                                          */
/*       <Expression>  :== 〈CompoundTerm〉 { 〈AddOp〉 〈CompoundTerm〉 }    */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseExpression(void)
{
    int op;
    ParseCompoundTerm();

    while ((op = CurrentToken.code) == ADD || op == SUBTRACT) {
        ParseAddOp();
        ParseCompoundTerm();

        if (op == ADD)
            _Emit(I_ADD);
        else
            _Emit(I_SUB);
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                                           */
/*                                                                          */
/*      〈CompoundTerm〉 :== 〈Term〉 { 〈MultOp〉 〈Term〉 }                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm(void)
{
    int op;
    ParseTerm();

    while ((op = CurrentToken.code) == MULTIPLY || op == DIVIDE) {
        ParseMultOp();
        ParseTerm();

        if (op == MULTIPLY)
            _Emit(I_MULT);
        else
            _Emit(I_DIV);

    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                                                   */
/*                                                                          */
/*      〈Term〉 :== [ “-” ] 〈SubTerm〉                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm(void)
{
    int negateFlag = 0;
    if (CurrentToken.code == SUBTRACT) {
        negateFlag = 1;
        Accept(SUBTRACT);
    }
    ParseSubTerm();

    if (negateFlag)
        _Emit(I_NEG);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                                                */
/*                                                                          */
/*      〈SubTerm〉 :== 〈Variable〉 | 〈IntConst〉 | “(” 〈Expression〉 “)”   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm(void)
{
    int i, dS;
    SYMBOL *var;
    switch (CurrentToken.code)
    {
        case INTCONST:
            Emit(I_LOADI, CurrentToken.value);
            Accept(INTCONST);
            break;
        case LEFTPARENTHESIS:
            Accept(LEFTPARENTHESIS);
            ParseExpression();
            Accept(RIGHTPARENTHESIS);
            break;
        case IDENTIFIER:
        default:
            var = LookupSymbol();


            if (var != NULL){
                if (var->type == STYPE_VARIABLE) {
                    if (writing) {
                        Emit(I_LOADA,var->address);
                    }
                    else if (reading) {
                        Emit(I_STOREA,var->address);
                    }
                    else {
                        Emit(I_LOADA,var->address);
                    }
                } else if (var->type == STYPE_LOCALVAR) {
                    dS = scope - var->scope;
                    if (dS == 0)
                        Emit(I_LOADFP, var->address);
                    else {
                        _Emit(I_LOADFP);
                        for (i = 0; i < dS - 1; i++)
                            _Emit(I_LOADSP);
                        Emit(I_LOADSP, var->address);
                    }
                } else if (var->type == STYPE_REFPAR) {
                    if (writing) {
                        Emit(I_LOADA,var->address);
                    }
                    else if (reading) {
                        Emit(I_STOREA,var->address);
                    }
                    else {
                        Emit(I_LOADA,var->address);
                    }
                    _Emit(I_PUSHFP);
                }
            }
            else {
                printf("name undeclared or not a variable\n");
            }
            Accept(IDENTIFIER);
            break;
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*      〈BooleanExpression〉 :== 〈Expression〉 〈RelOp〉 〈Expression〉      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int ParseBooleanExpression(void)
{
    int BackPatchAddr, RelOpInstruction;
    ParseExpression();
    RelOpInstruction = ParseRelOp();
    ParseExpression();
    _Emit(I_SUB);
    BackPatchAddr = CurrentCodeAddress();
    Emit(RelOpInstruction, 9999);
    return BackPatchAddr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                                  */
/*                                                                          */
/*      〈AddOp〉 :== “+” | “-”                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseAddOp(void)
{
    if (CurrentToken.code == ADD)
        Accept(ADD);
    else if (CurrentToken.code == SUBTRACT)
        Accept(SUBTRACT);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                                 */
/*                                                                          */
/*      〈MultOp〉 :== “*” | “/”                                             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseMultOp(void)
{
    if (CurrentToken.code == MULTIPLY)
        Accept(MULTIPLY);
    else if (CurrentToken.code == DIVIDE)
        Accept(DIVIDE);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                                 */
/*                                                                          */
/*      〈RelOp〉 :== “=” | “<=” | “>=” | “<” | “>”                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int ParseRelOp(void)
{
    int RelOpInstruction = 0;
    switch (CurrentToken.code)
    {
    case EQUALITY:
        RelOpInstruction = I_BZ;
        Accept(EQUALITY);
        break;
    case LESSEQUAL:
        RelOpInstruction = I_BG;
        Accept(LESSEQUAL);
        break;
    case GREATEREQUAL:
        RelOpInstruction = I_BL;
        Accept(GREATEREQUAL);
        break;
    case LESS:
        RelOpInstruction = I_BGZ;
        Accept(LESS);
        break;
    case GREATER:
        RelOpInstruction = I_BLZ;
        Accept(GREATER);
        break;
    default:
        break;
    }
    return RelOpInstruction;
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

PRIVATE void Accept(int ExpectedToken)
{
    static int recovering = 0;
    if (recovering) {
        while (CurrentToken.code != ExpectedToken
            && CurrentToken.code != ENDOFINPUT){
            CurrentToken = GetToken();
        }
        recovering = 0;
    }
    if (CurrentToken.code != ExpectedToken)  {
        SyntaxError(ExpectedToken, CurrentToken);
        KillCodeGeneration();
        recovering = 1;
        ErrorFlag = 1;
    }
    else  CurrentToken = GetToken();
}

PRIVATE void SetupSets(void)
{

    /* Set for synching ParseProgram */
    InitSet(&ProgramFS_aug1, 3, VAR, PROCEDURE, BEGIN);
    InitSet(&ProgramFS_aug2, 2, PROCEDURE, BEGIN);
    InitSet(&ProgramFBS_aug, 3, ENDOFPROGRAM, ENDOFINPUT, END);

    /* Set for synching ParseProcDeclaration */
    InitSet(&ProcDeclarationFS_aug1, 3, VAR, PROCEDURE, BEGIN);
    InitSet(&ProcDeclarationFS_aug2, 2, PROCEDURE, BEGIN);
    InitSet(&ProcDeclarationFBS_aug, 3, ENDOFPROGRAM, ENDOFINPUT, END);

    /* Set for synching ParseBlock */
    InitSet(&BlockFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);
    InitSet(&BlockFBS_aug, 4, ENDOFINPUT, ENDOFPROGRAM, SEMICOLON, ELSE);
}

PRIVATE void Synchronise(SET *F, SET *FB)
{
    SET S;
    S = Union(2, F, FB);
    if (!InSet(F, CurrentToken.code)) {
        SyntaxError2(*F, CurrentToken);
        while (!InSet(&S, CurrentToken.code))
            CurrentToken = GetToken();
    }
}


PRIVATE SYMBOL *MakeSymbolTableEntry(int symtype)
{
    SYMBOL *oldsptr;
    SYMBOL *newsptr;
    char *cptr;
    int hashindex;
    static int varaddress = 0;
    if (CurrentToken.code == IDENTIFIER) {
        if (NULL == (oldsptr = Probe(CurrentToken.s, &hashindex))
                || oldsptr->scope < scope) {
            if (oldsptr == NULL)
                cptr = CurrentToken.s;
            else
                cptr = oldsptr->s;

            if (NULL == (newsptr = EnterSymbol(cptr, hashindex))) {
                printf("Fatal internal error in EnterSymbol, Exiting...\n");
                exit(EXIT_FAILURE);
            }
            else {
                if (oldsptr == NULL)
                    PreserveString();

                newsptr->scope = scope;
                newsptr->type = symtype;

                if (symtype == STYPE_VARIABLE || symtype == STYPE_LOCALVAR
                    || symtype == STYPE_REFPAR) {
                    newsptr->address = varaddress;
                    varaddress++;
                }
                else newsptr->address = -1;
            }
        }
        else {
            printf("Error: Variable %s already declared,"
                    "stopping code generation...\n", CurrentToken.s);
            KillCodeGeneration();
            ErrorFlag = 1;
        }

    }
    else
        printf("Current token is not an identifier: %i %s\n",
                CurrentToken.code, CurrentToken.s);

    return newsptr;
}

PRIVATE SYMBOL *LookupSymbol(void)
{
    SYMBOL *sptr;
    if (CurrentToken.code == IDENTIFIER) {
        sptr = Probe(CurrentToken.s, NULL);
        if (sptr == NULL) {
            Error("Identifier not declared", CurrentToken.pos);
            KillCodeGeneration();
            ErrorFlag = 1;
        }
    }
    else sptr = NULL;
    return sptr;
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

PRIVATE int  OpenFiles(int argc, char *argv[])
{
    if (argc != 4)  {
        fprintf(stderr, "%s <inputfile> <listfile> <codefile>\n", argv[0]);
        return 0;
    }

    if(!CheckSuffix(argc, argv))
        return 0;

    if (NULL == (InputFile = fopen(argv[1], "r")))  {
        fprintf(stderr, "cannot open \"%s\" for input\n", argv[1]);
        return 0;
    }

    if (NULL == (ListFile = fopen(argv[2], "w")))  {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
        fclose(InputFile);
        return 0;
    }

    if (NULL == (CodeFile = fopen(argv[3], "w")))  {
        fprintf(stderr, "cannot open \"%s\" for output\n", argv[3]);
        fclose(InputFile);
        fclose(ListFile);
        return 0;
    }

    return 1;
}
/**
 * @brief Check that input files have expected suffix's
 *
 * @param argc number of command line arguments
 * @param argv command line arguments
 * @return 1 if arguments have valid suffix's
 * @return 0 if arguments have invalid suffix's
 */
PRIVATE int CheckSuffix(int argc, char *argv[])
{
    char *suffix = strrchr(argv[1],'.');
    if (!(suffix && ((!strcmp(suffix, ".prog")||(!strcmp(suffix, ".errs")))))) {
        printf("Input file should end in .prog or .errs\n");
        return 0;
    }
    suffix = strrchr(argv[2],'.');
    if (!(suffix && (!strcmp(suffix, ".list")))) {
        printf("List file should end in .list\n");
        return 0;
    }
    suffix = strrchr(argv[3],'.');
    if (!(suffix && (!strcmp(suffix, ".asm")))) {
        printf("Assembly output file should end in .asm\n");
        return 0;
    }
    return 1;

}