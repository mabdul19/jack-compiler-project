#include "symbols.h"

SymTable* createSymbolTable() {
    SymTable* symbolTable = malloc(sizeof(SymTable));
    symbolTable->classes = NULL;
    symbolTable->globalVars = NULL;
    return symbolTable;
}

void addVar(Class* class, Var** varList, char* name, char* type, Kind kind, int idx, Token tk) {
    Var* newVar = malloc(sizeof(Var));
    newVar->name = strdup(name);
    newVar->type = strdup(type);
    newVar->next = *varList;
    newVar->kind = kind;
    newVar->idx = idx;
    newVar->tk = tk;
    *varList = newVar;
}

void addClass(SymTable* symbolTable, Class* class) {
    Class* newClass = malloc(sizeof(Class));
    newClass->methods = class->methods;
    newClass->vars = class->vars;
    newClass->name = strdup(class->name);
    newClass->next = NULL;
    class->vars = NULL;
    class->methods = NULL;
    class->next = NULL;

    if (symbolTable->classes == NULL) {
        symbolTable->classes = newClass;
    } else {
        Class* lastClass = symbolTable->classes;
        while (lastClass->next != NULL) {
            lastClass = lastClass->next;
        }
        lastClass->next = newClass;
    }
}

void addMethod(Class* class, Method method, rKind k, int idx) {
    Method* newMethod = malloc(sizeof(Method));
    newMethod->localVars = method.localVars;
    newMethod->rTokens = method.rTokens;
    newMethod->args = method.args;
    newMethod->name = strdup(method.name);
    newMethod->type = strdup(method.type);
    newMethod->kind = k;
    newMethod->idx = idx;
    newMethod->next = NULL;
    method.args = NULL;
    method.localVars = NULL;
    method.rTokens = NULL;
    newMethod->next = NULL;

    if (class->methods == NULL) {
        class->methods = newMethod;
    } else {
        Method* lastMethod = class->methods;
        while (lastMethod->next != NULL) {
            lastMethod = lastMethod->next;
        }
        lastMethod->next = newMethod;
    }
}

void addGlobalVar(SymTable* symbolTable, char* name, char* type, Kind kind,  int idx, Token tk) {
    addVar(NULL, &(symbolTable->globalVars), name, type, kind, idx, tk);
}

void addLocalVar(Method* method, char* name, char* type, Kind kind, int idx, Token tk) {
    addVar(NULL, &(method->localVars), name, type, kind, idx, tk);
}

void addVarToClass(Class* class, char* name, char* type, Kind kind, int idx, Token tk) {
    addVar(class, &(class->vars), name, type, kind, idx, tk);
}

void printSymbolTable(SymTable* symbolTable, FILE* fd) {
    fprintf(fd, "Global Variables:\n");
    Var* globalVar = symbolTable->globalVars;
    while (globalVar != NULL) {
        fprintf(fd, "Name: %s, Type: %s\n", globalVar->name, globalVar->type);
        globalVar = globalVar->next;
    }

    fprintf(fd, "\nClasses:\n");
    Class* currentClass = symbolTable->classes;
    while (currentClass != NULL) {
        fprintf(fd, "Class: %s\n", currentClass->name);

        fprintf(fd, "*Variables:\n");
        Var* classVar = currentClass->vars;
        while (classVar != NULL) {
            fprintf(fd, "\tName: %s, Type: %s, Kind: %s\n", classVar->name, classVar->type, kindstr(classVar->kind));
            classVar = classVar->next;
        }

        fprintf(fd, "*Methods:\n");
        Method* classMethod = currentClass->methods;
        while (classMethod != NULL) {
            fprintf(fd, "\tName: %s, Type: %s, Kind: %s, Idx: %d\n", classMethod->name, classMethod->type, rkindstr(classMethod->kind), classMethod->idx);

            fprintf(fd, "\t*Local Variables:\n");
            Var* localVar = classMethod->localVars;
            if(!localVar)
                fprintf(fd, "\t\tNone.\n");
            while (localVar != NULL) {
                fprintf(fd, "\t\tName: %s, Type: %s, Kind: %s\n", localVar->name, localVar->type, kindstr(localVar->kind));
                localVar = localVar->next;
            }

            fprintf(fd, "\t*Arguments:\n");
            Var* arg = classMethod->args;
            if(!arg)
                fprintf(fd, "\t\tNone.\n");
            while (arg != NULL) {
                fprintf(fd, "\t\tName: %s, Type: %s, Kind: %s\n", arg->name, arg->type, kindstr(arg->kind));
                arg = arg->next;
            }

            fprintf(fd, "\t*Reference Tokens:\n");
            ReferenceTokens* rtok = classMethod->rTokens;
            if(!rtok)
                fprintf(fd, "\t\tNone.\n");
            while (rtok != NULL) {
                fprintf(fd, "\t\tClass: %s, Name: %s\n", rtok->className?rtok->className:"None", rtok->tokenName);
                rtok = rtok->next;
            }
            classMethod = classMethod->next;
        }

        currentClass = currentClass->next;
    }
}

int classContainsVar(Class* class, char* varName) {
    if(!class)
        return 0;
    Var* classVar = class->vars;
    while (classVar != NULL) {
        if (strcmp(classVar->name, varName) == 0) {
            return 1; // Found a variable with the same name
        }
        classVar = classVar->next;
    }
    return 0; // Variable not found
}

int classContainsMethod(Class* class, char* methodName) {
    if(!class)
        return 0;
    Method* classMethod = class->methods;
    while (classMethod != NULL) {
        if (strcmp(classMethod->name, methodName) == 0) {
            return 1; // Found a method with the same name
        }
        classMethod = classMethod->next;
    }
    return 0; // Method not found
}

Class* getVariableClass(SymTable *sym, Class* class, char* variableName) {
    Var* var = class->vars;
    while (var != NULL) {
        if (strcmp(var->name, variableName) == 0) {
            return getClass(sym, var->type);
        }
        var = var->next;
    }

    Method *m = class->methods;
    while(m) {
        Var* var = m->localVars;
        while (var != NULL) {
            if (strcmp(var->name, variableName) == 0) {
                return getClass(sym, var->type);
            }
            var = var->next;
        }
        m = m->next;
    }
    return NULL;  // Variable not found or data type mismatch
}

Class* getClass(SymTable* symbolTable, char *className) {
    Class *c = symbolTable->classes;
    while(c) {
        if(!strcmp(c->name, className))
            return c;
        c = c->next;
    }
    return NULL;
}


int methodContainsVar(Method* method, char* varName) {
    Var* localVar = method->localVars;
    while (localVar != NULL) {
        if (strcmp(localVar->name, varName) == 0) {
            return 1; // Found a local variable with the same name
        }
        localVar = localVar->next;
    }

    Var* arg = method->args;
    while (arg != NULL) {
        if (strcmp(arg->name, varName) == 0 || strcmp(arg->type, varName) == 0) {
            return 1; // Found an argument with the same name
        }
        arg = arg->next;
    }

    return 0; // Variable not found
}

int containsVar(SymTable* symbolTable, char* varName) {
    Class* currentClass = symbolTable->classes;
    while (currentClass != NULL) {
        if (classContainsVar(currentClass, varName)) {
            return 1; // Found a matching variable in the class
        }
        currentClass = currentClass->next;
    }

    Var* globalVar = symbolTable->globalVars;
    while (globalVar != NULL) {
        if (strcmp(globalVar->name, varName) == 0) {
            return 1; // Found a matching global variable
        }
        globalVar = globalVar->next;
    }

    return 0; // Variable not found
}

int containsClass(SymTable* symbolTable, char* className) {
    Class* currentClass = symbolTable->classes;
    while (currentClass != NULL) {
        if (strcmp(currentClass->name, className) == 0) {
            return 1; // Found a matching class
        }
        currentClass = currentClass->next;
    }
    return 0; // Class not found
}

void addRtok(Method *method, ReferenceTokens rtok)
{
    ReferenceTokens *newRtok = malloc(sizeof (ReferenceTokens));
    if(rtok.className)
        newRtok->className = strdup(rtok.className);
    else
        newRtok->className = NULL;
    newRtok->tokenName = strdup(rtok.tokenName);
    newRtok->tk = rtok.tk;
    newRtok->next = method->rTokens;
    method->rTokens = newRtok;

    rtok.className = 0;
}

Kind kind(char *k) {
    if(strcmp("field", k) == 0)
        return Field;
    if(strcmp("static", k) == 0)
        return Static;
    if(strcmp("arg", k) == 0)
        return Arg;
    if(strcmp("var", k) == 0)
        return Local;

    return Arg;
}

char *kindstr(Kind k)
{
    if(k == Field)
        return "Class Feild";
    if(k == Static)
        return "Static";
    if(k == Arg)
        return "Argument";
    if(k == Local)
        return "Local Variable";

    return "";
}

rKind rkind(char *k)
{
    if(strcmp("constructor", k) == 0)
        return rConstructor;
    if(strcmp("method", k) == 0)
        return rMethod;
    if(strcmp("function", k) == 0)
        return rFunction;

    return rMethod;
}

char *rkindstr(rKind k)
{
    if(k == rConstructor)
        return "constructor";
    if(k == rMethod)
        return "method";
    if(k == rFunction)
        return "function";

    return "method";
}
