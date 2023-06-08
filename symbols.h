#ifndef _symtable_h
#define _symtable_h

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

typedef enum {
    Local, Static, Arg, Field
} Kind;

typedef enum {
    rMethod, rConstructor, rFunction
} rKind;

typedef struct Var {
    char* name;
    char* type;
    Kind kind;
    int idx;
    Token tk;
    struct Var* next;
} Var;

typedef struct ReferenceTokens {
    char *className;
    char *tokenName;
    Token tk;
    struct ReferenceTokens* next;
} ReferenceTokens;

//TODO: add additional arg-field for this @ idx:0
typedef struct Method {
    Var* localVars;
    char* name;
    char* type;
    rKind kind;
    int idx;
    Var* args;
    ReferenceTokens* rTokens;
    struct Method* next;
} Method;

typedef struct Class {
    Method* methods;
    Var* vars;
    char* name;
    struct Class* next;
} Class;

typedef struct {
    Class* classes;
    Var* globalVars;
} SymTable;

SymTable* createSymbolTable();
void addClass(SymTable* symbolTable, Class *class);
void addMethod(Class* class, Method method, rKind k, int idx);
void addVar(Class* class, Var** varList, char* name, char* type, Kind kind, int idx, Token tk);
void addGlobalVar(SymTable* symbolTable, char* name, char* type, Kind kind, int idx, Token tk);
void addRtok(Method *method, ReferenceTokens rtok);
void addLocalVar(Method* method, char* name, char* type, Kind kind, int idx, Token tk);
void addVarToClass(Class* class, char* name, char* type, Kind kind, int idx, Token tk);
void printSymbolTable(SymTable* symbolTable, FILE *fd);
int classContainsVar(Class* class, char* varName);
int classContainsMethod(Class* class, char* methodName);
int methodContainsVar(Method* method, char* varName);
int containsVar(SymTable* symbolTable, char* varName);
int containsClass(SymTable* symbolTable, char* className);
Class* getVariableClass(SymTable* sym, Class *class, char* variableName);
Class* getClass(SymTable* symbolTable, char *className);
Kind kind(char *k);
rKind rkind(char *k);
char *kindstr(Kind k);
char *rkindstr(rKind k);

#endif
