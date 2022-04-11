/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       comp1.c                                                            */
/*                                                                          */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*       Laoise Robinson          19253613                                  */
/*       Niall Hearne             18241026                                  */
/*       Daniel Puslednik         19261977                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parser2 - parser1 with Augmented S-Algol Error Recovery - parser   */
/*       is able to detect and report errors and then recover and resync    */
/*       with the input                                                     */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headers/global.h" /* "header/ can be removed later" */
#include "headers/scanner.h"
#include "headers/line.h"
#include "headers/symbol.h"
#include "headers/code.h"
#include "headers/strtab.h"
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile; /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;  /*  For nicely-formatted syntax errors.  */

PRIVATE int ERROR_FLAG; /* if any syntax errors are detected set to 1*/

PRIVATE FILE *CodeFile;     /* machine code output file */
PRIVATE int varaddress;     /* incremented each time a new symbol table entry made */
PRIVATE int writing;        /* set to one while parsing arguments*/
PRIVATE int reading;        /* set to one while parsing arguments*/
PRIVATE TOKEN CurrentToken; /*  Parser lookahead token.  Updated by  */
                            /*  routine Accept (below).  Must be     */
                            /*  initialised before parser starts.    */
PRIVATE int scope;
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int OpenFiles(int argc, char *argv[]);
PRIVATE void Accept(int code);
PRIVATE void Synchronise(SET *F, SET *FB);
PRIVATE void SetupSets(void);

PRIVATE void ParseProgram(void);
PRIVATE void ParseDeclarations(void);
PRIVATE void ParseProcDeclaration(void);
PRIVATE void ParseParameterList(void);
PRIVATE void ParseFormalParameter(void);
PRIVATE void ParseBlock(void);
PRIVATE void ParseStatement(void);
PRIVATE void ParseSimpleStatement(void);
PRIVATE void ParseRestOfStatement(void);
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
PRIVATE void ParseBooleanExpression(void);
PRIVATE void ParseAddOp(void);
PRIVATE void ParseMultOp(void);
PRIVATE void ParseRelOp(void);
PRIVATE void ParseVariable(void);
PRIVATE void ParseIntConst(void);
PRIVATE void ParseIdentifier(void);

PRIVATE SET SetBlockFBS;
PRIVATE SET SetBlockFS_aug;
PRIVATE SET SetDeclarFBS;
PRIVATE SET SetDeclarFS_aug1;
PRIVATE SET SetDeclarFS_aug2;
PRIVATE SET SetProgramFBS;
PRIVATE SET SetProgramFS_aug1;
PRIVATE SET SetProgramFS_aug2;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: parser entry point.  Sets up parser globals (opens input and      */
/*        output files, initialises current lookahead), calls SetupSets     */
/*        to initialize the sets used in augmented s-algol error recovery   */
/*        and then then calls "ParseProgram" to start the parse             */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main(int argc, char *argv[])
{
    ERROR_FLAG = 0;
    varaddress = 0;
    writing = 0;
    reading = 0;
    scope = 1;
    printf("parsing\n");
    if (OpenFiles(argc, argv))
    {
        InitCharProcessor(InputFile, ListFile);
        CurrentToken = GetToken();
        InitCodeGenerator(CodeFile); /*Initialize code generation*/
        SetupSets();
        ParseProgram();
        WriteCodeFile(); /*Write out assembly to file*/
        fclose(InputFile);
        fclose(ListFile);
        if (ERROR_FLAG)
        {
            printf("Syntax Error\n"); /*code file has syntax error*/
            return EXIT_FAILURE;
        }
        printf("Valid\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf("Syntax Error\n"); /*command line arguments are wrong*/
        return EXIT_FAILURE;
    }
}
/*
printf("Valid\n");
return EXIT_SUCCESS;
}
else
{
printf("Invalid\n");
return EXIT_FAILURE;
}
}*/

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
    /*Lookahead token it should be the Program's name, so we call the MakeSymbolTableEntry*/
    MakeSymbolTableEntry(STYPE_PROGRAM);
    Accept(IDENTIFIER);
    Accept(SEMICOLON);

    Synchronise(&SetProgramFS_aug1, &SetProgramFBS);
    if (CurrentToken.code == VAR)
    {
        ParseDeclarations();
    }
    Synchronise(&SetProgramFS_aug2, &SetProgramFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclaration();
        Synchronise(&SetProgramFS_aug2, &SetProgramFBS);
    }

    ParseBlock();
    Accept(ENDOFPROGRAM);
    /* _Emit(I_HALT);
}

/*--------------------------------------------------------------------------*/
    /*                                                                          */
    /*  ParseDeclarations implements:                                           */
    /*                                                                          */
    /*  〈Declarations〉 :== “VAR” 〈Variable〉 { “,” 〈Variable〉 } “;”          */
    /*                                                                          */
    /*--------------------------------------------------------------------------*/

    PRIVATE void ParseDeclarations(void)
    {
        int vcount = 0;
        Accept(VAR);
        MakeSymbolTableEntry(STYPE_VARIABLE);
        Accept(IDENTIFIER);

        while (CurrentToken.code == COMMA)
        {
            Accept(COMMA);
            MakeSymbolTableEntry(STYPE_VARIABLE);

            ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
            vcount++;
        }
        Accept(SEMICOLON);
        Emit(I_INC, vcount);
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
        Accept(PROCEDURE);
        MakeSymbolTableEntry(STYPE_PROCEDURE);
        Accept(IDENTIFIER);

        scope++; /* To avoid multiple declarations */

        if (CurrentToken.code == LEFTPARENTHESIS)
        {
            ParseParameterList();
        }
        Accept(SEMICOLON);
        Synchronise(&SetDeclarFS_aug1, &SetDeclarFBS);
        if (CurrentToken.code == VAR)
        {
            ParseDeclarations();
        }
        Synchronise(&SetDeclarFS_aug2, &SetDeclarFBS);
        while (CurrentToken.code == PROCEDURE)
        {
            ParseProcDeclaration();
            Synchronise(&SetDeclarFS_aug2, &SetDeclarFBS);
        }
        ParseBlock();
        Accept(SEMICOLON);

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
    /*  〈FormalParameter 〉 :== [ “REF” ] 〈Variable〉                           */
    /*                                                                          */
    /*--------------------------------------------------------------------------*/

    PRIVATE void ParseFormalParameter(void)
    {
        if (CurrentToken.code == REF)
        {
            MakeSymbolTableEntry(STYPE_REFPAR);
            Accept(REF);
        }
        MakeSymbolTableEntry(STYPE_VARIABLE);
        Accept(IDENTIFIER);
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
        Synchronise(&SetBlockFS_aug, &SetBlockFBS);
        while (CurrentToken.code != END)
        {
            ParseStatement();
            Accept(SEMICOLON);
            Synchronise(&SetBlockFS_aug, &SetBlockFBS);
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
        switch (CurrentToken.code)
        {
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
            SyntaxError(IDENTIFIER, CurrentToken);
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
        SYMBOL *var;
        var = LookupSymbol();
        Accept(IDENTIFIER);
        ParseRestOfStatement(var);
    }

    /*--------------------------------------------------------------------------*/
    /*                                                                          */
    /*  ParseRestOfStatement implements:                                        */
    /*                                                                          */
    /*       〈RestOfStatement〉 :== 〈ProcCallList〉 | 〈Assignment〉 | \eps     */
    /*                                                                          */
    /*--------------------------------------------------------------------------*/

    PRIVATE void ParseRestOfStatement(void)
    {
        switch (CurrentToken.code)
        {
        case LEFTPARENTHESIS:
            ParseProcCallList();
            break;
        case:
            if (var != NULL)
            {
                if (var->type == STYPE_PROCEDURE)
                {
                    Emit(I_CALL, var->address);
                }
                else
                {
                    printf("error in parse rest of statement semicolon case");
                    KillCodeGeneration();
                }
            }
            break;
        case ASSIGNMENT:
        default:
            ParseAssignment();
            if (var != NULL)
            {
                if (var->type == STYPE_VARIABLE)
                {
                    Emit(I_STOREA, var->address);
                }
                else
                {
                    printf("error in parse rest of statement default case");
                    KillCodeGeneration();
                }
                break;
            }
            break;
        /***Handle epsilon here?*/
        default:
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

    PRIVATE void ParseProcCallList(void)
    {
        Accept(LEFTPARENTHESIS);
        if (reading)
        { /*emit READ before each parameter*/
            _Emit(I_READ);
        }
        ParseActualParameter();
        if (writing)
        { /*emit WRITE after each parameter*/
            _Emit(I_WRITE);
        }

        while (CurrentToken.code == COMMA)
        {
            Accept(COMMA);
            if (reading)
            {
                _Emit(I_READ);
            }
            ParseActualParameter();
            if (writing)
            {
                _Emit(I_WRITE);
            }
        }
        Accept(RIGHTPARENTHESIS);
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
        if (CurrentToken.code == IDENTIFIER)
            Accept(IDENTIFIER);
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
    /*  ParseIfStatment implements:                                             */
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

        if (CurrentToken.code == ELSE)
        {
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

        while (CurrentToken.code == COMMA)
        {
            Accept(COMMA);
            Accept(IDENTIFIER);
        }
        Accept(RIGHTPARENTHESIS);
        reading = 0;
        /*reading = 1;
        Accept(READ);
        ParseProcCallList();
        reading = 0;
        Another way that this code function

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

            while (CurrentToken.code == COMMA)
            {
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
            while (CurrentToken.code == ADD || op == SUBTRACT)
            {
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
            while (CurrentToken.code == MULTIPLY || op == DIVIDE)
            {
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
            int negative_flag = 0;

            if (CurrentToken.code == SUBTRACT)
            {
                negative_flag = 1;
                Accept(SUBTRACT);
            }

            ParseSubTerm();
            if (negative_flag)
            {
                _Emit(I_NEG);
            }
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
            SYMBOL *var;
            switch (CurrentToken.code)
            {
            case IDENTIFIER:
            default:
                var = LookupSymbol();
                if (var != NULL && var->type == STYPE_VARIABLE)
                {
                    if (writing)
                    {
                        Emit(I_LOADA, var->address); /*Load the parameter value*/
                    }
                    else if (reading)
                    {
                        Emit(I_STOREA, var->address); /*Store user input into each parameter*/
                    }
                    else
                    {
                        Emit(I_LOADA, var->address);
                    }
                }
                else
                    printf("Name undeclared or not a variable..!!");
                ParseVariable(); /* ParseVariable() --> ParseIdentifier() --> Accept(IDENTIFIER); */
                break;
            }

        case INTCONST:
            Emit(I_LOADI, CurrentToken.value);
            Accept(INTCONST);
            break;
        case LEFTPARENTHESIS:
            Accept(LEFTPARENTHESIS);
            ParseExpression();
            Accept(RIGHTPARENTHESIS);
        default:
            SyntaxError(IDENTIFIER, CurrentToken);
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

    PRIVATE void ParseBooleanExpression(void)
    {
        int BackPatchAddr, RelOpInstruction;
        ParseExpression();
        RelOpInstruction = ParseRelOp();
        ParseExpression();
        _Emit(I_SUB);
        BackPatchAddr = CurrentCodeAddress();
        Emit(RelOpInstruction, 0);
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
        /* maybe a case to except a syntax error here niall hearne
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
            /* maybe a case to except a syntax error here niall hearne
    }

    /*--------------------------------------------------------------------------*/
            /*                                                                          */
            /*  ParseRelOp implements:                                                 */
            /*                                                                          */
            /*      〈RelOp〉 :== “=” | “<=” | “>=” | “<” | “>”                          */
            /*                                                                          */
            /*--------------------------------------------------------------------------*/

            PRIVATE void ParseRelOp(void)
            {
                int RelOpInstruction;
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
                    /*default:
                        SyntaxError(IDENTIFIER, CurrentToken);
                        break;*/
                }
                return RelopInstruction;
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
            /*       <Variable> :== <Digit> { <Digit> }                                 */
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
            /*                                                                          */
            /*  Accept:  Takes an expected token name as argument, and if the current   */
            /*           lookahead matches this, advances the lookahead and returns.    */
            /*                                                                          */
            /*           If the expected token fails to match the current lookahead,    */
            /*           this routine reports a syntax error. It then checks if the     */
            /*           parser is in a recovering state. If yes then the code checks   */
            /*           if the current token is neither the expected token or the end  */
            /*           of the input. if it is neither then the current token calls    */
            /*           the get token function and resets the recovering flag to 0.    */
            /*           This is all for the purpose of resyncing the parser with the   */
            /*           input.                                                         */
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

                if (recovering)
                {
                    while (CurrentToken.code != ExpectedToken &&
                           CurrentToken.code != ENDOFINPUT)
                        CurrentToken = GetToken();
                    recovering = 0;
                }

                if (CurrentToken.code != ExpectedToken)
                {
                    SyntaxError(ExpectedToken, CurrentToken);
                    KillCodeGeneration();
                    recovering = 1;
                    ERROR_FLAG = 1; /* for use in main to avoid printing valid*/
                }
                else
                    CurrentToken = GetToken();
            }

            /*  SetupSets: description here TODO
             */
            PRIVATE void SetupSets(void)
            {
                /* Set for ParseBlock */
                InitSet(&SetBlockFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);
                InitSet(&SetBlockFBS, 4, SEMICOLON, ELSE, ENDOFPROGRAM, ENDOFINPUT);

                /* Set for ParseProgram */
                InitSet(&SetProgramFS_aug1, 3, PROCEDURE, VAR, BEGIN);
                InitSet(&SetProgramFS_aug2, 2, PROCEDURE, BEGIN);
                InitSet(&SetProgramFBS, 3, ENDOFINPUT, ENDOFPROGRAM, END);

                /* Set for ParseProcDeclaration */
                InitSet(&SetDeclarFS_aug1, 3, VAR, PROCEDURE, BEGIN);
                InitSet(&SetDeclarFS_aug2, 2, PROCEDURE, BEGIN);
                InitSet(&SetDeclarFBS, 3, ENDOFINPUT, ENDOFPROGRAM, END);
            }

            /* Synchronise: description here TODO
             */
            PRIVATE void Synchronise(SET * F, SET * FB)
            {
                SET S;

                S = Union(2, F, FB);
                if (!InSet(F, CurrentToken.code))
                {
                    SyntaxError2(*F, CurrentToken);
                    while (!InSet(&S, CurrentToken.code))
                        CurrentToken = GetToken();
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

            PRIVATE int OpenFiles(int argc, char *argv[])
            {

                if (argc != 4) /*Updated to 4 due to codefile needed in comp1 */
                {
                    fprintf(stderr, "%s <inputfile> <listfile>\n", argv[0]);
                    return 0;
                }

                if (NULL == (InputFile = fopen(argv[1], "r")))
                {
                    fprintf(stderr, "cannot open \"%s\" for input\n", argv[1]);
                    return 0;
                }

                if (NULL == (ListFile = fopen(argv[2], "w")))
                {
                    fprintf(stderr, "cannot open \"%s\" for output\n", argv[2]);
                    fclose(InputFile);
                    return 0;
                }
                if (NULL == (CodeFile = fopen(argv[3], "w")))
                {
                    fprintf(stderr, "cannot open \"%s\" for output\n", argv[3]);
                    fclose(InputFile);
                    fclose(ListFile);
                    return 0;
                }

                return 1;
            }
            PRIVATE void MakeSymbolTableEntry(int symtype)
            {
                /*〈Variable Declarations here〉*/
                SYMBOL *newsptr; /*new symbol pointer*/
                SYMBOL *oldsptr; /*old symbol pointer*/
                char *cptr;      /*current pointer*/
                int hashindex;
                static int varaddress = 0;

                if (CurrentToken.code == IDENTIFIER)
                { /* check to see if there is an entry in the symbol table with the same name as IDENTIFIER */
                    if (NULL == (oldsptr = Probe(CurrentToken.s, &hashindex)) || oldsptr->scope < scope)
                    {
                        if (oldsptr == NULL)
                            cptr = CurrentToken.s;
                        else
                            cptr = oldsptr->s;

                        if (NULL == (newsptr = EnterSymbol(cptr, hashindex)))
                        {
                            /*〈Fatal internal error in EnterSymbol, compiler must exit: code for this goes here〉*/
                            printf("Fatal internal error in EnterSymbol..!!\n");
                            exit(EXIT_FAILURE);
                        }
                        else
                        {
                            if (oldsptr == NULL)
                            {
                                PreserveString();
                            }
                            newsptr->scope = scope;
                            newsptr->type = symtype;
                            if (symtype == STYPE_VARIABLE)
                            {
                                newsptr->address = varaddress;
                                varaddress++;
                            }
                            else
                                newsptr->address = -1;
                        }
                    }
                    else
                    { /*〈Error, variable already declared: code for this goes here〉*/
                        printf("Error, variable already declared...!!\n");
                        KillCodeGeneration();
                    }
                }
                else
                {
                    printf("current token not identifier");
                }
            }
