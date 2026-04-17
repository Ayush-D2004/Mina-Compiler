%code requires {
    #include "ast.h"
}

%{
#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

int yylex(void);
void yyerror(const char *s);
extern int yylineno;

ASTNode *g_root = NULL;

static ASTList *append_node(ASTList *list, ASTNode *node) {
    return ast_list_append(list, node);
}
%}

%define parse.error verbose
%locations

%union {
    long ival;
    double fval;
    char *sval;
    TypeKind type;
    ASTNode *node;
    ASTList *list;
}

%token <sval> IDENT STRING_LIT
%token <ival> INT_LIT
%token <fval> FLOAT_LIT
%token TRUE FALSE
%token PROC LET SET WHEN OTHERWISE REPEAT GIVE SHOW ASK
%token NUM BOOL VOID
%token AND OR EQ NEQ LE GE
%token ARROW

%type <node> program funcdef block stmt vardecl setstmt showstmt ifstmt whilestmt retstmt expr primary param
%type <list> funcs params_opt params stmts args_opt args
%type <type> type

%start program

%left OR
%left AND
%left EQ NEQ
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS '!'
%nonassoc ELSE

%%

program
    : funcs                 { g_root = ast_new_program($1, yylineno); }
    ;

funcs
    : funcdef               { $$ = append_node(NULL, $1); }
    | funcs funcdef         { $$ = append_node($1, $2); }
    ;

funcdef
    : PROC IDENT '(' params_opt ')' ':' type block
        { $$ = ast_new_func($7, $2, $4, $8, yylineno); }
    ;

params_opt
    : /* empty */           { $$ = NULL; }
    | params                { $$ = $1; }
    ;

params
    : param                 { $$ = append_node(NULL, $1); }
    | params ',' param      { $$ = append_node($1, $3); }
    ;

param
    : IDENT ':' type        { $$ = ast_new_param($3, $1, yylineno); }
    ;

block
    : '{' stmts '}'         { $$ = ast_new_block($2, yylineno); }
    ;

stmts
    : /* empty */           { $$ = NULL; }
    | stmts stmt            { $$ = append_node($1, $2); }
    ;

stmt
    : vardecl ';'           { $$ = $1; }
    | setstmt ';'           { $$ = $1; }
    | showstmt ';'          { $$ = $1; }
    | retstmt ';'           { $$ = $1; }
    | ifstmt                { $$ = $1; }
    | whilestmt             { $$ = $1; }
    | expr ';'              { $$ = ast_new_exprstmt($1, yylineno); }
    | block                 { $$ = $1; }
    ;

vardecl
    : LET IDENT ':' type
        { $$ = ast_new_vardecl($4, $2, NULL, yylineno); }
    | LET IDENT ':' type ARROW expr
        { $$ = ast_new_vardecl($4, $2, $6, yylineno); }
    ;

setstmt
    : SET IDENT ARROW expr  { $$ = ast_new_set($2, $4, yylineno); }
    ;

showstmt
    : SHOW '(' args ')'     { $$ = ast_new_show($3, yylineno); }
    ;

ifstmt
    : WHEN expr block OTHERWISE block
        { $$ = ast_new_if($2, $3, $5, yylineno); }
    | WHEN expr block
        { $$ = ast_new_if($2, $3, NULL, yylineno); }
    ;

whilestmt
    : REPEAT expr block     { $$ = ast_new_while($2, $3, yylineno); }
    ;

retstmt
    : GIVE                  { $$ = ast_new_return(NULL, yylineno); }
    | GIVE expr             { $$ = ast_new_return($2, yylineno); }
    ;

args_opt
    : /* empty */           { $$ = NULL; }
    | args                  { $$ = $1; }
    ;

args
    : expr                  { $$ = append_node(NULL, $1); }
    | args ',' expr         { $$ = append_node($1, $3); }
    ;

expr
    : expr OR expr          { $$ = ast_new_binop("||", $1, $3, yylineno); }
    | expr AND expr         { $$ = ast_new_binop("&&", $1, $3, yylineno); }
    | expr EQ expr          { $$ = ast_new_binop("==", $1, $3, yylineno); }
    | expr NEQ expr         { $$ = ast_new_binop("!=", $1, $3, yylineno); }
    | expr '<' expr         { $$ = ast_new_binop("<", $1, $3, yylineno); }
    | expr '>' expr         { $$ = ast_new_binop(">", $1, $3, yylineno); }
    | expr LE expr          { $$ = ast_new_binop("<=", $1, $3, yylineno); }
    | expr GE expr          { $$ = ast_new_binop(">=", $1, $3, yylineno); }
    | expr '+' expr         { $$ = ast_new_binop("+", $1, $3, yylineno); }
    | expr '-' expr         { $$ = ast_new_binop("-", $1, $3, yylineno); }
    | expr '*' expr         { $$ = ast_new_binop("*", $1, $3, yylineno); }
    | expr '/' expr         { $$ = ast_new_binop("/", $1, $3, yylineno); }
    | expr '%' expr         { $$ = ast_new_binop("%", $1, $3, yylineno); }
    | '-' expr %prec UMINUS { $$ = ast_new_unop("-", $2, yylineno); }
    | '!' expr              { $$ = ast_new_unop("!", $2, yylineno); }
    | primary               { $$ = $1; }
    ;

primary
    : INT_LIT               { $$ = ast_new_int($1, yylineno); }
    | FLOAT_LIT             { $$ = ast_new_float($1, yylineno); }
    | STRING_LIT            { $$ = ast_new_string($1, yylineno); }
    | TRUE                  { $$ = ast_new_bool(1, yylineno); }
    | FALSE                 { $$ = ast_new_bool(0, yylineno); }
    | IDENT                 { $$ = ast_new_var($1, yylineno); }
    | IDENT '(' args_opt ')' { $$ = ast_new_call($1, $3, yylineno); }
    | ASK '(' ')'           { $$ = ast_new_call("ask", NULL, yylineno); }
    | '(' expr ')'          { $$ = $2; }
    ;

type
    : NUM                   { $$ = TYPE_NUM; }
    | BOOL                  { $$ = TYPE_BOOL; }
    | VOID                  { $$ = TYPE_VOID; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error at line %d: %s\n", yylineno, s);
}
