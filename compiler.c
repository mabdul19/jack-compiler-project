/************************************************************************
University of Liverpool
Compiler Design and Construction
The Compiler Module
*************************************************************************/
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#include "compiler.h"

#define MAX_JACK_FILES 16
//#define DUMP_SYM
int InitCompiler ()
{
	return 1;
}

char** getJackFilesInDirectory(const char* dir_name, int *count) {
    DIR* dir = opendir(dir_name);
    if (dir == NULL) {
        printf("Failed to open directory: %s\n", dir_name);
        return NULL;
    }

    char** jackFiles = malloc(sizeof(char*) * MAX_JACK_FILES);
    for (int i = 0; i < MAX_JACK_FILES; ++i)
        jackFiles[i] = NULL;

    size_t fileCount = 0;

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            const char* file_name = entry->d_name;
            size_t name_length = strlen(file_name);

            // Check if the file ends with ".jack"
            if (name_length > 5 && strcmp(file_name + name_length - 5, ".jack") == 0) {
                // Allocate memory for the file path
                char* file_path = malloc((name_length + strlen(dir_name) + 2) * sizeof(char));
                if (file_path == NULL) {
                    // Error handling for memory allocation failure
                    perror("Failed to allocate memory for file path");
                    // Free allocated memory before returning
                    for (int i = 0; i < fileCount; ++i)
                        free(jackFiles[i]);
                    free(jackFiles);
                    closedir(dir);
                    return NULL;
                }
                sprintf(file_path, "%s/%s", dir_name, file_name);
                file_path[name_length + strlen(dir_name) + 1] = '\0';

                jackFiles[fileCount++] = strdup(file_path);
                free(file_path);
            }
        }
    }

    jackFiles[fileCount++] = strdup("String.jack");
    jackFiles[fileCount++] = strdup("Keyboard.jack");
    jackFiles[fileCount++] = strdup("Math.jack");
    jackFiles[fileCount++] = strdup("Array.jack");
    jackFiles[fileCount++] = strdup("Memory.jack");
    jackFiles[fileCount++] = strdup("Output.jack");
    jackFiles[fileCount++] = strdup("Screen.jack");
    jackFiles[fileCount++] = strdup("Sys.jack");
    *count = (int)fileCount;
    closedir(dir);
    return jackFiles;
}

SymTable *sym;
char *crrClassName;

char *getClassName(char *fname) {
    char *buff = malloc(sizeof (char) * 16);
    memset(buff, '\0', 16);
    int i=0;
    char *s = strstr(fname, "/");
    if(!s)  s = fname;
    else    ++s;
    for(char *c = s; c != strstr(fname, ".jack"); ++c)
        buff[i++] = *c;
    return buff;
}

int isGeneralTok(char *c) {
    //numbers, true, false...
    return atoi(c) || !strcmp(c, "0") || !strcmp(c, "true") || !strcmp(c, "false") || !strcmp(c, "null") || !strcmp(c, "-");
}


ParserInfo validate() {
    SymTable *debug = sym;
    Class *class = sym->classes;
    while (class) {
        for(Var *i=class->vars; i; i=i->next) {
            if(strcmp(i->type, "int") == 0 ||
               strcmp(i->type, "char") == 0 ||
               strcmp(i->type, "boolean") == 0)
                continue;
            else {
                if(!containsClass(sym, i->type)) {
                    ParserInfo pi;
                    pi.er = undecIdentifier;
                    pi.tk = i->tk;
                    strcpy(pi.tk.lx, i->type);
                    return pi;
                }
            }
        }

        for(Var *i=class->vars; i; i=i->next) {
            for(Var *j=class->vars; j; j=j->next) {
                if(i->next != j->next && !strcmp(i->name, j->name)) {
                    ParserInfo pi;
                    pi.er = redecIdentifier;
                    pi.tk = i->tk;
                    return pi;
                }
            }
        }

        Method *method = class->methods;
        while(method) {
            ReferenceTokens *rtok = method->rTokens;
            while (rtok) {
                if(rtok->className) {
                    Class *c = 0;
                    c = getClass(sym, rtok->className);
                    if(!c) {
                        c = getVariableClass(sym, class, rtok->className);
                    }
                    if(!c) {
                        ParserInfo pi;
                        pi.er = undecIdentifier;
                        pi.tk = rtok->tk;
                        strcpy(pi.tk.lx, rtok->className);
                        return pi;
                    }
                    if(!classContainsMethod(c, rtok->tokenName) &&
                       !classContainsVar(c, rtok->tokenName) &&
                       !isGeneralTok(rtok->tokenName) &&
                       !methodContainsVar(method, rtok->tokenName))
                    {
                        ParserInfo pi;
                        pi.er = undecIdentifier;
                        pi.tk = rtok->tk;
                        return pi;
                    }
                } else {
                    if(!classContainsMethod(class, rtok->tokenName) &&
                       !classContainsVar(class, rtok->tokenName) &&
                       !methodContainsVar(method, rtok->tokenName) &&
                       !containsClass(sym, rtok->tokenName) &&
                       !isGeneralTok(rtok->tokenName))
                    {
                        if(strstr(rtok->tokenName, "Error:")) {   //error from lexer module
                            rtok = rtok->next;
                            continue;
                        }
                        ParserInfo pi;
                        pi.er = undecIdentifier;
                        pi.tk = rtok->tk;
                        return pi;
                    }
                }
                rtok = rtok->next;
            }


            for(Var *i=method->localVars; i; i=i->next) {
                for(Var *j=method->localVars; j; j=j->next) {
                    if(i->next != j->next && !strcmp(i->name, j->name)) {
                        ParserInfo pi;
                        pi.er = redecIdentifier;
                        pi.tk = i->tk;
                        return pi;
                    }
                }
            }
            method = method->next;
        }

        class = class->next;
    }

    ParserInfo pi;
    pi.er = none;
    return pi;
}

ParserInfo compile (char* dir_name)
{
	ParserInfo p;

	// write your code below
    sym = createSymbolTable();

    int total_file = 0;
    char ** files = getJackFilesInDirectory(dir_name, &total_file);

    for(int i=0; i<total_file; i++) {
        crrClassName = getClassName(files[i]);
        char vmFileName[128];
        sprintf(vmFileName, "%s/%s.vm", dir_name, crrClassName);
        FILE *out = fopen(vmFileName, "w");
        writer_init(out);
        InitParser(files[i]);
        Parse();
    }

#ifdef DUMP_SYM
    char buff[128];
    sprintf(buff, "sym_%s.txt", dir_name);
    FILE *out = fopen(buff, "w");
    printSymbolTable(sym, out);
#endif

    return validate();
}
int StopCompiler()
{
    // Deallocate the SymTable
    if (sym != NULL) {
        // Deallocate globalVars
        Var* globalVar = sym->globalVars;
        while (globalVar != NULL) {
            Var* temp = globalVar;
            globalVar = globalVar->next;
            free(temp->name);
            free(temp->type);

            free(temp);
        }

        // Deallocate classes and their members
        Class* class = sym->classes;
        while (class != NULL) {
            // Deallocate vars
            Var* var = class->vars;
            while (var != NULL) {
                Var* temp = var;
                var = var->next;
                free(temp->name);
                free(temp->type);

                free(temp);
            }

            // Deallocate methods and their members
            Method* method = class->methods;
            while (method != NULL) {
                // Deallocate localVars
                Var* localVar = method->localVars;
                while (localVar != NULL) {
                    Var* temp = localVar;
                    localVar = localVar->next;
                    free(temp->name);
                    free(temp->type);

                    free(temp);
                }

                // Deallocate args
                Var* arg = method->args;
                while (arg != NULL) {
                    Var* temp = arg;
                    arg = arg->next;
                    free(temp->name);
                    free(temp->type);

                    free(temp);
                }

                // Deallocate rTokens
                ReferenceTokens* rToken = method->rTokens;
                while (rToken != NULL) {
                    ReferenceTokens* temp = rToken;
                    rToken = rToken->next;
                    free(temp->className);
                    free(temp->tokenName);

                    free(temp);
                }

                Method* temp = method;
                method = method->next;
                free(temp->name);
                free(temp->type);

                free(temp);
            }

            Class* temp = class;
            class = class->next;
            free(temp->name);

            free(temp);
        }

        free(sym);
    }

    //deallocate crrClassName
    free(crrClassName);

    return 1;
}


#ifndef TEST_COMPILER
int main ()
{
	InitCompiler ();
	ParserInfo p = compile ("Pong");
	PrintError (p);
	StopCompiler ();
	return 1;
}
#endif
