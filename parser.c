#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "symbols.h"

//#define DEBUG
// you can declare prototypes of parser functions below

// internal flags
int pmethod;

Class   *crrClass;
extern SymTable *sym;
extern char *crrClassName;

ParserInfo pi;
Token start;

Var variable;
Method method;

int vIdx[4];
int rIdx[3];
char crrkind[10];
char crrRkind[10];


int WhileCounter = 0;
int IfCounter = 0;

int InitParser (char* file_name)
{
    InitLexer(file_name);
    return 1;
}




/*
    the writer module which writes to .vm file
*/

FILE *out;
void writer_init(FILE* o) {
    out = o;
}

void writer_close() {
    fclose(out);
}

void writer_add() {
    fprintf(out, "add\n");
}

void writer_substract() {
    fprintf(out, "sub\n");
}

void writer_negate() {
    fprintf(out, "neg\n");
}

void writer_equal() {
    fprintf(out, "eq\n");
}

void writer_greaterThan() {
    fprintf(out, "gt\n");
}

void writer_lessThan() {
    fprintf(out, "lt\n");
}

void writer_and() {
    fprintf(out, "and\n");
}

void writer_or() {
    fprintf(out, "or\n");
}

void writer_not() {
    fprintf(out, "not\n");
}

void writer_push(const char* str, short i) {
    fprintf(out, "push %s %d\n", str, i);
}

void writer_pop(const char* str, short i) {
    fprintf(out, "pop %s %d\n", str, i);
}

void writer_label(const char* str) {
    fprintf(out, "label %s\n", str);
}

void writer_goTo(const char* str) {
    fprintf(out, "goto %s\n", str);
}

void writer_ifGoTo(const char* str) {
    fprintf(out, "if-goto %s\n", str);
}

void writer_function(const char* str, short i) {
    fprintf(out, "function %s %d\n", str, i);
}

void writer_returnFromFunction() {
    fprintf(out, "return\n");
}

void writer_callFunction(const char* str, short i) {
    fprintf(out, "call %s %d\n", str, i);
}

/*
 * end of writer module.
*/


enum LookAhead {
    Nothing,
    Statement,
    IfStat,
    WhileStat,
    LetStat,
    DoStat,
    ReturnStat,
    Expression,
    SmallBracketE,
    CurlyBracketE,
    SmallBracketS,
    CurlyBracketS,
    BigBracketS,
    BigBracketE,
    ClassVarDecl,
    SubRoutineDecl,
    KeywordConstant,
    VarDecl,
    Type,
    Term,
    BinOperator,
    UniOperator,
    aClass
};
typedef enum LookAhead LookAhead;

char* classNames[] = {
    "Square",
    "List",
    "PongGame",
    "Bat",
    "Ball",
    "Array",
    "Fraction",
    "SquareGame"
};

int isClassName(char *name) {
    for(int i=0; i<8; ++i) {
        if(strcmp(classNames[i], name) == 0)
            return 1;
    }
    return 0;
}

LookAhead identifyToken(Token t) {
    if(strcmp(t.lx, "while") == 0)
        return WhileStat;
    else if(strcmp(t.lx, "if") == 0)
        return  IfStat;
    else if(strcmp(t.lx, "let") == 0)
        return  LetStat;
    else if(strcmp(t.lx, "class") == 0)
        return aClass;
    else if(t.lx[0] == '}')
        return  CurlyBracketE;
    else if(t.lx[0] == '{')
        return  CurlyBracketS;
    else if(t.lx[0] == ')')
        return  SmallBracketE;
    else if(t.lx[0] == '(')
        return  SmallBracketS;
    else if(t.lx[0] == ']')
        return BigBracketE;
    else if(t.lx[0] == '[')
        return BigBracketS;
    else if(strcmp(t.lx, "do") == 0)
        return  DoStat;
    else if(strcmp(t.lx, "return") == 0)
        return  ReturnStat;
    else if(strcmp(t.lx, "static") == 0 || strcmp(t.lx, "field") == 0) {
        memset(crrkind, '\0', strlen(crrkind));
        strcpy(crrkind, t.lx);
        return ClassVarDecl;
    } else if(strcmp(t.lx, "int") == 0 || strcmp(t.lx, "char") == 0 || strcmp(t.lx, "boolean") == 0 || strcmp(t.lx, "String") == 0 || isClassName(t.lx))
        return Type;
    else if(strcmp(t.lx, "constructor") == 0 || strcmp(t.lx, "function") == 0 || strcmp(t.lx, "method") == 0 || strcmp(t.lx, "void") == 0 ) {
        memset(crrRkind, '\0', strlen(crrRkind));
        strcpy(crrRkind, t.lx);
        return SubRoutineDecl;
    } else if(strcmp(t.lx, "var") == 0) {
        memset(crrkind, '\0', strlen(crrkind));
        strcpy(crrkind, t.lx);
        return VarDecl;
    }
    else if(t.lx[0] == '~' || t.lx[0] == '-')
        return UniOperator;
    else if((t.lx[0] == '+') || (t.lx[0] == '-') || (t.lx[0] == '*') || (t.lx[0] == '/') ||
            (t.lx[0] == '&') || (t.lx[0] == '|') || (t.lx[0] == '<') || (t.lx[0] == '>') ||
            (t.lx[0] == '='))
        return BinOperator;
    else if(strcmp(t.lx, "true") == 0 || strcmp(t.lx, "false") == 0 || strcmp(t.lx, "null") == 0 || strcmp(t.lx, "this") == 0 )
        return KeywordConstant;
    return Nothing;
}


int parse(LookAhead crr, LookAhead pev);
int parseSubroutineCall();
int rtokAhead = 0;

int parseExpression(LookAhead end) {
    int preceeder = 0, cid = 0, oldcid = 0;     //c=count, id=identifier | cid -> identifiers count so far
    ReferenceTokens rtok;
    Token prev;
    rtok.className = 0;

    while(1) {
        oldcid = cid;
        cid = 0;
        int preceederVal = 1;
        start = GetNextToken();
        if(strstr(start.lx, "Error:"))
            return 1;
        if(start.tp == STRING) {
            char *str = start.lx;
            short n = strlen(str);
            writer_push("constant", n);
            writer_callFunction("String.new", 1);

            for (short index = 0; index < n; ++index) {
                writer_push("constant", str[index]);
                writer_callFunction("String.appendChar", 2);
            }
        }
        LookAhead w = identifyToken(start);
        if(start.tp == STRING)
            rtokAhead = 0;
        if(rtokAhead) {
            if(PeekNextToken().lx[0] == '.') {
                rtok.className = strdup(start.lx);
                rtok.tk = start;
                rtok.tk.tp = ID;
                GetNextToken();         //skip '.'
                start = GetNextToken();
                rtok.tokenName = strdup(start.lx);
                addRtok(&method, rtok);
            }
            rtok.tokenName = strdup(start.lx);
            rtok.tk = start;
            rtok.tk.tp = ID;
            addRtok(&method, rtok);

            rtokAhead = 0;
        }
        if(w == end && w != Nothing) {
            return 1;
        }
        else if(w == SmallBracketS) {
            int res = parseExpression(SmallBracketE);
            if(end == Nothing)
                return res;
        } else if(w == BigBracketS)
            return parseExpression(BigBracketE);

        if(w == SmallBracketE) {
            if(end == SmallBracketE) {
                return 1;
            } else {
                pi.er = semicolonExpected;
                return 0;
            }
        }
        else if (w == BigBracketE) {
            if(end == BigBracketE) {
                return 1;
            } else {
                pi.er = syntaxError;
                return 0;
            }
        }

        if(start.lx[0] == ';' && end != Nothing) {
            if(end == CurlyBracketE)
                pi.er = closeBraceExpected;
            else if(end == SmallBracketE)
                pi.er = closeParenExpected;
            else if(end == BigBracketE)
                pi.er = closeBracketExpected;
            else
                return 1;
            return 0;
        }
        if(start.lx[0] == ';')
            return 1;
        if(start.lx[0] == '=' && end == Nothing)
            rtokAhead = 1;
        if(w == BinOperator && start.tp != STRING) {
            if(!preceeder) {
                pi.er = syntaxError;
                return 0;
            }
            preceederVal = 0;
        } else if(w == UniOperator) {
            preceederVal = 0;
        } else if(start.lx[0] == '.' && !preceeder) {
            pi.er = idExpected;
            return 0;
        } else if(start.lx[0] == '.') {
            rtok.className = strdup(prev.lx);
            rtokAhead = 1;
        }

        cid += start.tp == ID;
        if(oldcid && cid) {
            pi.er = closeParenExpected;
            return 0;
        }

        preceeder = preceederVal;
        prev = start;
    }
    return 1;
}

int parseReturn() {
    start = PeekNextToken();
    if(start.lx[0] == ';') {
        writer_push("constant", 0);
        writer_returnFromFunction();
        return 1;
    }
    else if(start.lx[0] == '}') {
        start = GetNextToken();
        pi.er = semicolonExpected;
        return 0;
    }

    return parseExpression(Nothing);
}

int parseLet() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);

    //varName
    if(w != Nothing) {
        pi.er = syntaxError;
        return 0;
    }
    if(start.tp != ID) {
        pi.er = syntaxError;
        return 0;
    }

    ReferenceTokens rtok;
    rtok.tokenName = strdup(start.lx);
    rtok.className = NULL;
    rtok.tk = start;
    rtok.tk.tp = ID;
    addRtok(&method, rtok);


    start = GetNextToken();

    if(start.lx[0] == '[') {
        if(!parseExpression(BigBracketE))
            return 0;
        writer_add();
        while(identifyToken(PeekNextToken()) == BigBracketE) {
            /*start = */
            GetNextToken();
#ifdef DEBUG
            printf("skipped %s@%d\n", start.lx, start.ln);
#endif
        }
        start = GetNextToken();
        writer_pop("temp", 0);
        writer_pop("pointer", 1);
        writer_push("temp", 0);
        writer_pop("that", 0);
    }


    if(start.lx[0] != '=') {
        pi.er = equalExpected;
        return 0;
    }

    start = PeekNextToken();
    w = identifyToken(start);

    if(start.lx[0] == ';') {
        int line = start.ln;
        start = GetNextToken();
        start.ln = line;
        start.lx[0] = ';';
        start.lx[1] = '\0';
        start.tp = SYMBOL;
        pi.er = syntaxError;
        return 0;
    }

    if(w == SmallBracketS) {
        start = GetNextToken();
        w = SmallBracketE;
        //        printf("\r\r");
    } else if(w == BigBracketS) {
        //        printf("\r\r");
        start = GetNextToken();
        w = BigBracketE;
    }
    rtokAhead = 1;
    if(!parseExpression(w)) {
#ifdef DEBUG
        printf("exp not ok @ %s %d\n", PeekNextToken().lx, PeekNextToken().ln);
#endif
        return 0;
    }
    return 1;
}

int parseIf() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);
    if(w != SmallBracketS) {
        pi.er = openParenExpected;
        return 0;
    }
    if(!parseExpression(SmallBracketE))
        return 0;

    start = GetNextToken();
    w = identifyToken(start);
    if(w != CurlyBracketS) {
        pi.er = openBraceExpected;
        return 0;
    }

    int tic = IfCounter++;
    char lbl1[25], lbl2[25], lbl3[25];
    sprintf(lbl1, "IF_TRUE%d", tic);
    sprintf(lbl2, "IF_FALSE%d", tic);
    sprintf(lbl3, "IF_END%d", tic);
    writer_ifGoTo(lbl1);
    writer_goTo(lbl2);
    writer_label(lbl1);

    if(!parse(IfStat, Nothing))
        return 0;

    //else part
    start = PeekNextToken();
    if(strcmp(start.lx, "else") != 0) {
        writer_label(lbl2);
        return 1;
    }
    start = GetNextToken(); //consume the else token
    start = GetNextToken();
    w = identifyToken(start);
    if(w != CurlyBracketS) {
        pi.er = openBraceExpected;
        return 0;
    }

    writer_goTo(lbl3);
    writer_label(lbl2);
    int res = parse(IfStat, Nothing);
    writer_label(lbl3);
    writer_label(lbl2);
    return res;
}

int parseWhile() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);
    if(w != SmallBracketS) {
        pi.er = openParenExpected;
        return 0;
    }
    char lbl1[25], lbl2[25];
    int twc = WhileCounter++;
    sprintf(lbl1, "WHILE_EXP%d", twc);
    sprintf(lbl2, "WHILE_END%d", twc);
    writer_label(lbl1);
    parseExpression(SmallBracketE);
    writer_not();

    start = GetNextToken();
    w = identifyToken(start);
    if(w != CurlyBracketS) {
        pi.er = openBraceExpected;
        return 0;
    }

    writer_ifGoTo(lbl2);
    if(!parse(IfStat, Nothing))
        return 0;

    writer_goTo(lbl1);
    writer_label(lbl2);

    return 1;
}

int rtokreg = 0;
int external = 0;
int parseSubroutineCall() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);

    if(w != Nothing || start.tp != ID && w != Type) {
        pi.er = idExpected;
        return 0;
    }

    Token prev = start;
    start = GetNextToken();
    w = identifyToken(start);
    ReferenceTokens rtok;
    if(start.lx[0] == '.') {
        rtok.className = strdup(prev.lx);
        rtok.tokenName = strdup(PeekNextToken().lx);
        rtok.tk = PeekNextToken();
        addRtok(&method, rtok);
        rtokreg = 1;
        char str[128];
        sprintf(str, "%s.%s", rtok.className, rtok.tokenName);
        writer_callFunction(str, 1);
        external = 1;
        return parseSubroutineCall();
    } else if(!rtokreg) {
        rtok.className = NULL;
        rtok.tokenName = strdup(prev.lx);
        rtok.tk = prev;
        addRtok(&method, rtok);
    } else if(rtokreg) {
        rtokreg = 0;
    }

    if(w != SmallBracketS) {
        pi.er = openParenExpected;
        return 0;
    }

    if(external) {
        external = 0;
    } else {
        writer_push("pointer", 0);
        char str[128];
        sprintf(str, "%s.%s", crrClassName, rtok.tokenName);
        writer_callFunction(str, 1);
    }

    int res = parseExpression(SmallBracketE);
    writer_pop("temp", 0);
    return res;
}

int parseClass() {
    start = GetNextToken();
    if(start.tp == ERR) {
        pi.er = lexerErr;
        return 0;
    }

    if(strcmp(start.lx, "class") != 0) {
        pi.er = classExpected;
        pi.tk = start;
        return 0;
    }
    start = GetNextToken();
    if(start.tp != ID) {
        pi.er = idExpected;
        pi.tk = start;
        return 0;
    }
    start = GetNextToken();
    if(strcmp(start.lx, "{") != 0) {
        pi.er = openBraceExpected;
        pi.tk = start;
        return 0;
    }

    if(!parse(aClass, Nothing))
        return 0;

    if(strcmp(start.lx, "}") != 0) {
        pi.er = closeBraceExpected;
        pi.tk = start;
        return 0;
    }

    return 1;
}

int parseSubRoutine() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);
    if(w != SubRoutineDecl && w != Type)    //for subroutine name
        goto ERR;
    method.type = strdup(start.lx);

    start = GetNextToken();
    w = identifyToken(start);
    if(w != Nothing)    //for subroutine name
        goto ERR;
    if(start.tp != ID)
        goto ERR1;
    method.name = strdup(start.lx);

    rKind k = rkind(crrRkind);
    writer_function(method.name, rIdx[k]);
    int subroutineType = k;
    if (subroutineType == rMethod) {
       writer_push("argument", 0);
       writer_pop("pointer", 0);
    }
    else {
       if (subroutineType == rConstructor) {
           writer_push("constant", rIdx[k]);
           writer_callFunction("Memory.alloc", 1);
           writer_pop("pointer", 0);
       }
    }

#ifdef DEBUG
    printf("parsing method: %s->%s\n", crrClassName, method.name);
#endif

    start = GetNextToken();
    w = identifyToken(start);
    if(start.lx[0] == '.')
        return parseSubRoutine();   //same recursive grammer

    //parameter list

    // (
    if(w != SmallBracketS) {
        pi.er = openParenExpected;
        return 0;
    }

    // EMPTY )
    start = GetNextToken();
    w = identifyToken(start);
    int empty = (w == SmallBracketE);

    //type + name , ...
    if(!empty) {
        while(1) {
            w = identifyToken(start);   //type name
            if(w == CurlyBracketS) {
                pi.er = closeParenExpected;
                return 0;
            }
            if(w != Type)
                goto ERR;
            variable.type = strdup(start.lx);

            start = GetNextToken();     //var name
            w = identifyToken(start);
            if(w != Nothing)
                goto ERR;
            if(start.tp != ID)
                goto ERR1;
            variable.name = start.lx;

            addVar(NULL, &(method.args), variable.name, variable.type, kind(""), vIdx[kind("")]++, start);

            start = GetNextToken();
            if(strcmp(start.lx, ","))
                break;

            start = GetNextToken();     //next
        }

        // )
        w = identifyToken(start);
        if(w != SmallBracketE) {
            pi.er = closeParenExpected;
            return 0;
        }
    }

    start = GetNextToken();
    if(strcmp(start.lx, "{") != 0) {
        pi.er = openBraceExpected;
        pi.tk = start;
        return 0;
    }

    //Sub Routine Body
    pmethod = 1;
    if(!parse(SubRoutineDecl, Nothing)) {
#ifdef DEBUG
        printf("finished parsing %s .. not ok\n", method.name);
        printf("@ token: %s on %d\n", PeekNextToken().lx, PeekNextToken().ln);
#endif
        pmethod = 0;
        goto ERR;
    } else {
#ifdef DEBUG
        printf("finished parsing %s .. ok\n", method.name);
#endif
        pmethod = 0;
        return 1;
    }

    if(strcmp(start.lx, "}") != 0) {
        pi.er = closeBraceExpected;
        pi.tk = start;
        return 0;
    }

ERR:
    //    pi.er = subroutineDeclarErr;
    return 0;
ERR1:
    pi.er = idExpected;
    return 0;
}

int parseVarDecl() {
    start = GetNextToken();
    LookAhead w = identifyToken(start);
    //    if(w != Type) {
    //        pi.er = classVarErr;
    //        return 0;
    //    }
    variable.type = strdup(start.lx);
    while(1) {
        start = GetNextToken();
        w = identifyToken(start);
        if(w != Nothing) {
            pi.er = classVarErr;
            return 0;
        }
        if(start.tp != ID) {
            pi.er = idExpected;
            return 0;
        }

        variable.name = strdup(start.lx);
        if(pmethod) {
            Kind k = kind(crrkind);
            addLocalVar(&method, variable.name, variable.type, k, vIdx[k]++, start);
        } else {
            Kind k = kind(crrkind);
            addVarToClass(crrClass, variable.name, variable.type, k, vIdx[k]++, start);
        }

        start = GetNextToken();
        if(strcmp(start.lx, ","))
            break;
    }

    if(strcmp(start.lx, ";") != 0) {
        pi.er = semicolonExpected;
        return 0;
    }

    return 1;
}

int parse(LookAhead crr, LookAhead pev)
{
    start = GetNextToken();
    if(crr == aClass && (strcmp(start.lx, "}") == 0)) {  //base cond for class
        return 1;
    }

    if(crr == SubRoutineDecl && (strcmp(start.lx, "}") == 0)) {  //base cond for subroutine
        rKind k = rkind(crrRkind);
        addMethod(crrClass, method, k, rIdx[k]++);
        method.args = NULL;
        method.localVars = NULL;
        method.rTokens = NULL;
        pmethod = 0;
        return 1;
    }

    if(crr == IfStat && (strcmp(start.lx, "}") == 0)) {  //base cond for if condition
        return 1;
    }

    LookAhead w = identifyToken(start);
    if(w == ClassVarDecl && crr == aClass) {
        if(!parseVarDecl()) {
            return 0;
        }
    } if(w == SubRoutineDecl && crr == aClass) {
        if(!parseSubRoutine()) {
            return 0;
        }
    }
    if(w == VarDecl && crr == SubRoutineDecl) {
        if(!parseVarDecl()) {
            if(pi.er != semicolonExpected)
                pi.er = subroutineDeclarErr;
            return 0;
        }
    }

    //statements
    if(crr != aClass) {
        int ret = 1;
        switch (w) {
        case IfStat:
            ret = parseIf();
            break;
        case LetStat:
            ret = parseLet();
            break;
        case WhileStat:
            ret = parseWhile();
            break;
        case DoStat:
            ret = parseSubroutineCall();
            break;
        case ReturnStat:
            ret = parseReturn();
            break;
        default:
            break;
        }
        if(!ret)
            return 0;
    }

    return parse(crr, Nothing);
}

void freeVars() {
    // Freeing before setting to NULL for method.localVars
    while (method.localVars != NULL) {
        Var* temp = method.localVars;
        method.localVars = method.localVars->next;
        free(temp->name);
        free(temp->type);
        // Free any other members of the Var struct if necessary
        free(temp);
    }
    method.localVars = NULL;

    // Freeing before setting to NULL for method.next
    Method* tempMethod = method.next;
    method.next = NULL;
    while (tempMethod != NULL) {
        Method* nextMethod = tempMethod->next;
        // Free any members of the Method struct if necessary
        free(tempMethod);
        tempMethod = nextMethod;
    }

    // Freeing before setting to NULL for method.rTokens
    while (method.rTokens != NULL) {
        ReferenceTokens* temp = method.rTokens;
        method.rTokens = method.rTokens->next;
        free(temp->className);
        free(temp->tokenName);
        // Free any other members of the ReferenceTokens struct if necessary
        free(temp);
    }
    method.rTokens = NULL;

    // Freeing before setting to NULL for method.args
    while (method.args != NULL) {
        Var* temp = method.args;
        method.args = method.args->next;
        free(temp->name);
        free(temp->type);
        // Free any other members of the Var struct if necessary
        free(temp);
    }
    method.args = NULL;

    // Freeing before setting to NULL for crrClass->methods
    while (crrClass->methods != NULL) {
        Method* temp = crrClass->methods;
        crrClass->methods = crrClass->methods->next;
        // Free any members of the Method struct if necessary
        free(temp);
    }
    crrClass->methods = NULL;

    // Freeing before setting to NULL for crrClass->next
    crrClass->next = NULL;

    // Freeing before setting to NULL for crrClass->vars
    while (crrClass->vars != NULL) {
        Var* temp = crrClass->vars;
        crrClass->vars = crrClass->vars->next;
        free(temp->name);
        free(temp->type);
        // Free any other members of the Var struct if necessary
        free(temp);
    }
    crrClass->vars = NULL;
}

ParserInfo Parse ()
{
    for(int i=0; i<4; ++i)
        vIdx[i] = 0;
    for(int i=0; i<3; ++i)
        rIdx[i] = 0;

    WhileCounter = 0;
    IfCounter = 0;

    crrClass = (Class*)malloc(sizeof (Class*));
    crrClass->methods = NULL;
    crrClass->name = strdup(crrClassName);
    crrClass->vars = NULL;
    parseClass();
    addClass(sym, crrClass);
    //    if(strcmp(crrClass->name, "Keyboard") == 0)
    //        addClass(sym, crrClass);
    pi.tk = start;

    method.localVars = NULL;
    method.next = NULL;
    method.rTokens = NULL;
    method.args = NULL;

    crrClass->methods = NULL;
    crrClass->next = NULL;
    crrClass->vars = NULL;

    return pi;
}

int StopParser ()
{
    method.localVars = NULL;
    method.next = NULL;
    method.rTokens = NULL;
    method.args = NULL;

    crrClass->methods = NULL;
    crrClass->next = NULL;
    crrClass->vars = NULL;
    return 1;
}

#ifndef TEST_PARSER
//int main ()
//{

//    return 1;
//}
#endif
