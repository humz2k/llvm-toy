%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string.h>
#include "../src/ast.hpp"

extern "C" int yylex();

extern "C" void yyerror(char const *s);

%}

%token <r> REAL
%token <i> BOOL
%token <id> ID

%token LE GE EQ NEQ LAND LOR
%token IF ELSE THEN

%type <expr> expression
%type <expr_list> exp_list
%type <bexpr> boolean
%type <input_list> id_list
%type <func_t> prototype
%type <unit_t> definition
%type <unit_t> top_level

%token FN

%left '='

%nonassoc ELSE

%left EQ NEQ GE LE '<' '>'

%left LAND LOR
%right "neg"
%left '+' '-'
%left '*' '/'

%right '!'
%right ')'

%left '('

%start pgm

%union {
    char id[31];
    double r;
    int i;
    ExprAST* expr;
    ExprList* expr_list;
    BoolAST* bexpr;
    IdList* input_list;
    FunctionAST* func_t;
    UnitAST* unit_t;
}

%%

expression
    : REAL {$$ = make_object(NumberExprAST,$1);}
    | ID {$$ = make_object(NamedExprAST,$1);}
    | expression '+' expression {$$ = make_object(BinopExprAST,$1,'+',$3);}
    | expression '-' expression {$$ = make_object(BinopExprAST,$1,'-',$3);}
    | expression '/' expression {$$ = make_object(BinopExprAST,$1,'/',$3);}
    | expression '*' expression {$$ = make_object(BinopExprAST,$1,'*',$3);}
    | '(' expression ')' {$$ = $2;}
    | IF boolean THEN expression ELSE expression {$$ = make_object(IfExprAST,$2,$4,$6);}
    | '(' ID exp_list ')' {$$ = make_object(CallExprAST,$2,$3);}

boolean
    : BOOL {$$ = make_object(ConstBoolAST,$1);}
    | expression LE expression {$$ = make_object(BinopBoolAST,$1,op_LE,$3);}
    | expression GE expression {$$ = make_object(BinopBoolAST,$1,op_GE,$3);}
    | expression EQ expression {$$ = make_object(BinopBoolAST,$1,op_EQ,$3);}
    | expression NEQ expression {$$ = make_object(BinopBoolAST,$1,op_NEQ,$3);}
    | expression '<' expression {$$ = make_object(BinopBoolAST,$1,op_LT,$3);}
    | expression '>' expression {$$ = make_object(BinopBoolAST,$1,op_GT,$3);}
    | '!' boolean {$$ = make_object(NotBoolAST,$2);}
    | boolean LAND boolean {$$ = make_object(LOPBoolAST,$1,'&',$3);}
    | boolean LOR boolean {$$ = make_object(LOPBoolAST,$1,'|',$3);}
    | '(' boolean ')' {$$ = $2;}

exp_list
    : expression {$$ = make_object(ExprList); $$->push_back($1);}
    | exp_list expression {$1->push_back($2); $$ = $1;}

id_list
    : ID {$$ = make_object(IdList); $$->push_back($1);}
    | id_list ID {$1->push_back($2); $$ = $1;}

prototype
    : FN '(' ID id_list ')' {$$ = make_object(FunctionAST,$3,$4);}
    | FN ID {$$ = make_object(FunctionAST,$2);}
    | FN '(' ID ')' {$$ = make_object(FunctionAST,$3);}

definition
    : prototype expression {$1->add_expr($2); $$ = $1;}

top_level
    : expression {$$ = make_object(TopLevelExprAST,$1);}

unit
    : definition {$1->execute();}
    | top_level {$1->execute();}

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