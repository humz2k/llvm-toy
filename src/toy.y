%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string.h>

extern "C" int yylex();

extern "C" void yyerror(char const *s);

%}

%token <r> REAL
%token <i> BOOL
%token <id> ID

%token LE GE EQ NEQ LAND LOR
%token IF ELSE THEN

%token FN

%left '='

%left EQ NEQ GE LE '<' '>'

%left LAND LOR

%left '+' '-'
%left '*' '/'

%right '!'
%right ')'

%left '('

%nonassoc "then"
%nonassoc ELSE

%start pgm

%union {
    char id[31];
    double r;
    int i;
}

%%

expression
    : REAL
    | ID
    | expression '+' expression
    | expression '-' expression
    | expression '/' expression
    | expression '+' expression 
    | '-' expression
    | '(' expression ')'
    | IF boolean THEN expression ELSE expression
    //| ID '(' real_list ')'
    //| ID '(' ')'

boolean
    : BOOL
    | expression LE expression
    | expression GE expression
    | expression EQ expression
    | expression NEQ expression
    | expression '<' expression
    | expression '>' expression
    | '!' boolean
    | boolean LAND boolean
    | boolean LOR boolean
    | '(' boolean ')'

real_list
    : expression
    | real_list expression

id_list
    : ID
    | id_list ID

prototype
    : FN ID '(' id_list ')'
    | FN ID '(' ')'

definition
    : prototype expression

top_level
    : expression

unit
    : definition
    | top_level

unit_list
    : unit
    | unit_list unit

pgm
    : unit_list

%%

extern "C" void yyerror(char const *s)
{
    printf("ERROR: %s\n",s);
    exit(1);
}