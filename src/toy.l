L			[a-zA-Z_]
F			[0-9]+(\.[0-9]*)?
U           [\+\.\-\*\&\^\%\$\#\@\!\/\,\:\;\']

%{

#include <stdio.h>
#include <string.h>
#include <vector>
#include "../src/ast.hpp"
#include "toy.tab.hpp"

extern "C" int yylex();

int check_type();

#define LOG

#ifdef LOG
#define log(...) printf(__VA_ARGS__); printf("\n")
#else
#define log(...)
#endif

%}

%%

"//"[^\n]*              { }

{F}			    {yylval.r = atof(yytext);return(REAL);}

"<="            {return(LE);}
">="            {return(GE);}
"=="            {return(EQ);}
"!="            {return(NEQ);}
"<"             {return('<');}
">"             {return('>');}
"="             {return('=');}
"!"             {return('!');}
"not"           {return('!');}
"&&"            {return(LAND);}
"||"            {return(LOR);}
"and"           {return(LAND);}
"or"            {return(LOR);}
"+"             {return('+');}
"-"             {return('-');}
"*"             {return('*');}
"/"             {return('/');}
"("             {return('(');}
")"             {return(')');}
{U}+            {printf("USER OP!\n");}

"true"          {yylval.i = 1; return(BOOL);}
"false"          {yylval.i = 0; return(BOOL);}

"if"            {return(IF);}
"then"          {return(THEN);}
"else"          {return(ELSE);}

"fn"            {return(FN);}

{L}*            {strcpy(yylval.id,yytext);return ID;}

[ \t\v\n\f]		{ }
.			{ /* Add code to complain about unmatched characters */ }

%%

extern "C" int yywrap(void)
{
	return 1;
}