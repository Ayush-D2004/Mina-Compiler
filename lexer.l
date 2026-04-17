%{
#include "parser.tab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *xstrdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) {
        perror("malloc");
        exit(1);
    }
    memcpy(p, s, n);
    return p;
}
%}

%option noyywrap nodefault yylineno

DIGIT      [0-9]
ID         [A-Za-z_][A-Za-z0-9_]*

%%

[ \t\r]+                  ;
\n                        ;
\#.*                      ;
"//".*                    ;

"proc"                    { return PROC; }
"let"                     { return LET; }
"set"                     { return SET; }
"when"                    { return WHEN; }
"otherwise"               { return OTHERWISE; }
"repeat"                  { return REPEAT; }
"give"                    { return GIVE; }
"show"                    { return SHOW; }
"ask"                     { return ASK; }

"num"                     { return NUM; }
"bool"                    { return BOOL; }
"void"                    { return VOID; }

"true"                    { return TRUE; }
"false"                   { return FALSE; }

"<-"                      { return ARROW; }
"&&"                      { return AND; }
"||"                      { return OR; }
"=="                      { return EQ; }
"!="                      { return NEQ; }
"<="                      { return LE; }
">="                      { return GE; }

{DIGIT}+"."{DIGIT}+       { yylval.fval = atof(yytext); return FLOAT_LIT; }
{DIGIT}+                  { yylval.ival = atol(yytext); return INT_LIT; }

\"([^\\\"]|\\.)*\"    {
    yylval.sval = xstrdup(yytext);
    return STRING_LIT;
}

{ID}                      { yylval.sval = xstrdup(yytext); return IDENT; }

"("                       { return '('; }
")"                       { return ')'; }
"{"                       { return '{'; }
"}"                       { return '}'; }
":"                       { return ':'; }
","                       { return ','; }
";"                       { return ';'; }

"+"                       { return '+'; }
"-"                       { return '-'; }
"*"                       { return '*'; }
"/"                       { return '/'; }
"%"                       { return '%'; }

"!"                       { return '!'; }
"<"                       { return '<'; }
">"                       { return '>'; }

. {
    fprintf(stderr, "Lexer error at line %d: unexpected character '%s'\n",
            yylineno, yytext);
}

%%