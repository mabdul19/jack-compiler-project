/************************************************************************
University of Bradford

*************************************************************************/

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"


// YOU CAN ADD YOUR OWN FUNCTIONS, DECLARATIONS AND VARIABLES HERE

FILE* f;
char* Gfile_name;

const char* Keywords[21] = {"class", "constructor", "function", "method",
                            "field", "static", "var", "int", "char", "do",
                            "boolean", "void", "true", "false", "null",
                            "this", "let", "if", "else", "while", "return"}; //an array of Keywords

const char* Symbols[22] = {"//", "/*", "*/", "(", ")", "{", "}", "[", "]", ".", ",", ";", "+",
                           "-", "*", "/", "&", "|", "<", ">", "=", "~"};

int crrLine = 1;
int isKeyword(char* a)
{
    int i;
    for (i = 0; i < 21; i++)
    {
        if (strcmp(a, Keywords[i]) == 0)
        {
            return 1; //keyword found
        }
    }
    return 0; //not a keyword
}

int isSymbol(char* b)
{
    int i;
    for (i = 0; i < 22; i++)
    {
        if (strcmp(Symbols[i], b) == 0)
        {
            return 1; //symbol found
        }
    }
    return 0; //not a symbol
}

int is_symbol(char w) {
    int i;
    for (i = 0; i < 22; i++)
    {
        if (Symbols[i][0] == w)
        {
            return 1; //symbol found
        }
    }
    return 0; //not a symbol
}


typedef struct word_list_node {
    char* word;
    int line_num;
    struct word_list_node* next;
} WordListNode;
WordListNode* wordsList;
WordListNode* parse_file_to_words();
void split_words(WordListNode **head_ref);
void filter_comments(WordListNode** head_ptr);
Token ptoken;
int ptokenset = 0;
int comments = 0;

// IMPLEMENT THE FOLLOWING functions
//***********************************

// Initialise the lexer to read from source file
// file_name is the name of the source file
// This requires opening the file and making any necessary initialisations of the lexer
// If an error occurs, the function should return 0
// if everything goes well the function should return 1
int InitLexer (char* file_name)
{
    f = fopen(file_name, "r");
    if (f == NULL)
    {
        printf("Error:can't open file\n");
        return 0;
    }
    Gfile_name = malloc(strlen(file_name) + 1);
    strcpy(Gfile_name, file_name);
    Gfile_name[strlen(file_name)] = '\0';

    comments = 0;
    wordsList = parse_file_to_words();
    split_words(&wordsList);
    filter_comments(&wordsList);
    return 1; //success
}


WordListNode* parse_file_to_words() {
    FILE* fp = fopen(Gfile_name, "r");
    if (!fp) {
        printf("Error opening file %s\n", Gfile_name);
        return NULL;
    }

    char line[512];
    int line_num = 1;
    WordListNode* head = NULL;
    WordListNode** current_ptr = &head;
    while (fgets(line, sizeof(line), fp)) {
        char* token = strtok(line, " \t\n");
        while (token != NULL) {
            *current_ptr = (WordListNode*) malloc(sizeof(WordListNode));
            (*current_ptr)->word = strdup(token);
            (*current_ptr)->line_num = line_num;
            (*current_ptr)->next = NULL;
            current_ptr = &((*current_ptr)->next);
            token = strtok(NULL, " \t\n");
        }
        line_num++;
    }
    (*current_ptr) = NULL;
    fclose(fp);
    return head;
}

WordListNode* create_word_list_node(char* symbol, int line_num) {
    WordListNode* new_node = malloc(sizeof(WordListNode));
    unsigned int len = strlen(symbol) ;
    new_node->word = (char*)malloc(len + 1);
    new_node->word[len] = '\0';
    strcpy(new_node->word, symbol);
    new_node->line_num = line_num;
    new_node->next = 0;
    return new_node;
}

void split_words(WordListNode **head_ref) {
    WordListNode *curr = *head_ref;
    WordListNode *prev = NULL;
    while (curr != NULL) {
        char *word = curr->word;
        int newline_pos = strcspn(word, "\r\n");
        word[newline_pos] = '\0';
        if(strlen(word) == 0) {
            if(prev) {
                prev->next = curr->next;
            } else {
                *head_ref = curr->next;
            }
            curr = curr->next;
            continue;
        }
        unsigned int split_pos = -1;

        for (int i = 0; i < 22; i++) {
            char *symbol = (char*)Symbols[i];
            char *symbol_pos = strstr(word, symbol);

            if (symbol_pos != NULL) {
                split_pos = symbol_pos - word;
                if(!split_pos)
                    split_pos += strlen(symbol);
                break;
            }
        }

        int skip=1;
        if (split_pos >= 0 && split_pos <= strlen(word)) {
            char *left_word = (char*)malloc(split_pos + 1);
            strncpy(left_word, word, split_pos);
            left_word[split_pos] = '\0';

            char *right_word = strdup(word + split_pos);

            if(strlen(right_word) && strlen(left_word)) {
                curr->word = left_word;
                WordListNode *new_node = create_word_list_node(right_word, curr->line_num);

                new_node->next = curr->next;
                curr->next = new_node;
                skip = 0;
            }
        }
        if(skip) {
            prev = curr;
            curr = curr->next;
        }
    }
}

void filter_comments(WordListNode** head_ptr) {
    WordListNode *crr = *head_ptr;
    WordListNode *previous = NULL;
    while(crr) {
        if(crr && strcmp(crr->word, "//") == 0) {
            int ln = crr->line_num;
            while(crr && crr->line_num == ln) {
                comments = 1;
                crr = crr->next;
            }
            if(previous) {
                previous->next = crr;
            } else {
                *head_ptr = crr;
            }
        }
        else if(crr && strcmp(crr->word, "/*") == 0) {
            while(crr && strcmp(crr->word, "*/") != 0) {
                comments = 1;
                crr = crr->next;
            }
            if(crr)
                crr = crr->next;
            else {
                crr = malloc(sizeof(WordListNode));
                crr->line_num = -1;
                crr->word = strdup("cEOF");
                crr->next = 0;
            }

            if(previous) {
                previous->next = crr;
            } else {
                *head_ptr = crr;
            }
        } else {
            previous = crr;
            crr = crr->next;
        }
    }
}


void remove_whitespace(char* str) {
    // Remove whitespace from beginning of string
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // Remove whitespace from end of string
    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}


Token GetNextToken ()
{
    if(ptokenset) {
        ptokenset = 0;
        return ptoken;
    }
    Token t;
    strcpy(t.fl, Gfile_name);

    if(wordsList && wordsList->line_num == -1/*< 0 && strcmp(wordsList->word, "cEOF")*/) {
        t.tp = ERR;
        t.ln = 10;
        strcpy(t.lx, "Error: unexpected eof in comment");
        return t;
    }

    if (wordsList == NULL)
    {
        t.tp = EOFile;
        strcpy(t.lx, "End of file");
        return t;
    }

    WordListNode* curr = wordsList;

    t.ln = curr->line_num;

    // Check for new line character
    if (strcmp(curr->word, "\n") == 0)
    {
        crrLine++;
        wordsList = curr->next;
        free(curr);
        return GetNextToken();
    }

    // Check for white space
    while (isspace(curr->word[0]))
    {
        wordsList = curr->next;
        free(curr);
        curr = wordsList;
    }

    // Check for end of file
    if (curr == NULL)
    {
        t.tp = EOFile;
        strcpy(t.lx, "End of file");
        return t;
    }

    // Check for comment
    if (strcmp(curr->word, "/*") == 0)
    {
        wordsList = curr->next;
        free(curr);
        curr = wordsList;

        while (curr != NULL && (strcmp(curr->word, "*/") != 0))
        {
            if (strcmp(curr->word, "\n") == 0)
            {
                crrLine++;
            }
            wordsList = curr->next;
            free(curr);
            curr = wordsList;
        }

        if (curr == NULL)
        {
            t.tp = ERR;
            t.ln = crrLine;
            strcpy(t.lx, "Error: unexpected eof in comment");
            return t;
        }

        wordsList = curr->next;
        free(curr);
        return GetNextToken();
    }

    // Check for string constant
    if (curr->word[0] == '\"')
    {
        char str[128];
        strcpy(str, curr->word+1);
        wordsList = curr->next;
        free(curr);
        curr = wordsList;
        int ln = curr->line_num;

        while (curr != NULL && curr->word[0] != '\"')
        {
            if(curr->line_num != ln) {
                t.tp = ERR;
                strcpy(t.lx, "Error: new line in string constant");
                return t;
            }
            strcat(str, " ");
            strcat(str, curr->word);
            wordsList = curr->next;
            free(curr);
            curr = wordsList;
        }

        if (curr == NULL)
        {
            t.tp = ERR;
            strcpy(t.lx, "Error: unexpected eof in string constant");
            return t;
        }

        strcat(str, " ");
        curr->word[strlen(curr->word)-1] = '\0';
        strcat(str, curr->word);
        strcpy(t.lx, str);
        t.tp = STRING;
        wordsList = curr->next;
        free(curr);
        if(strlen(t.lx) == 3 && t.lx[0] == ' ' && t.lx[2] == ' ')
            t.lx[0] = t.lx[1], t.lx[1] = '\0';
        return t;
    }

    // Check for keyword or identifier
    if (isalpha(curr->word[0]) || curr->word[0] == '_')
    {
        if (isKeyword(curr->word))
        {
            t.tp = RESWORD;
            strcpy(t.lx, curr->word);
        }
        else
        {
            t.tp = ID;
            memset(t.lx, '\0', strlen(t.lx));
            strcpy(t.lx, curr->word);
        }
        wordsList = curr->next;
        free(curr);
        return t;
    }

    // Check for integer constant
    if (isdigit(curr->word[0]))
    {
        t.tp = INT;
        strcpy(t.lx, curr->word);
        wordsList = curr->next;
        free(curr);
        return t;
    }

    // Check for symbol
    if (isSymbol(curr->word))
    {
        t.tp = SYMBOL;
        strcpy(t.lx, curr->word);
        wordsList = curr->next;
    }
    else if(strcmp(curr->word, "=")) {
        t.tp = ERR;
        t.ln = crrLine+1;
        strcpy(t.lx, "Error: illegal symbol in source file");
        return t;
    }
    else
    {
        // Check if it's a keyword
        if (isKeyword(curr->word))
        {
            t.tp = RESWORD;
            strcpy(t.lx, curr->word);
        }
        else
        {
            // Check if it's a number
//            if (isNumber(curr->word))
//            {
//                t.tp = INT;
//                strcpy(t.lx, curr->word);
//            }
//            else
            {
                // It must be an identifier
                t.tp = ID;
                strcpy(t.lx, curr->word);
            }
        }

        // Update the head of the list
        wordsList = curr->next;
    }

    // Free the memory allocated for the node
    free(curr);

    return t;
}

// peek (look) at the next token in the source file without removing it from the stream
Token PeekNextToken () {
    if(ptokenset) {
        return ptoken;
    }
    Token t = GetNextToken();
    ptoken = t;
    ptokenset = 1;
    return t;
}

// clean out at end, e.g. close files, free memory, ... etc
int StopLexer ()
{
    fclose(f);
    crrLine = 1;
    return 0;
}

// do not remove the next line
#ifndef TEST
//int main ()
//{
//    // implement your main function here
//    // NOTE: the autograder will not use your main function
//    int r = InitLexer(Gfile_name);
//    if (r == 0)
//    {
//        return 0;
//    }
//    Token t;
//    do
//    {
//        Token t = GetNextToken();
//        if (t.tp == ERR)
//        {
//            printf("Error at line %d in file %s: %s\n", t.ln, t.fl, t.lx);
//            break;
//        }
//        printf("%s\n", t.lx);
//    } while (t.tp != EOFile);

//}
// do not remove the next line
#endif

