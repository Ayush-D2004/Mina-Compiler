#ifndef AST_H
#define AST_H

#include <stdio.h>

typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_NUM,
    TYPE_BOOL,
    TYPE_VOID,
    TYPE_STRING
} TypeKind;

typedef enum {
    NODE_PROGRAM = 0,
    NODE_FUNC,
    NODE_PARAM,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_SET,
    NODE_SHOW,
    NODE_IF,
    NODE_WHILE,
    NODE_RETURN,
    NODE_EXPR_STMT,
    NODE_INT,
    NODE_FLOAT,
    NODE_BOOL,
    NODE_VAR,
    NODE_BINOP,
    NODE_UNOP,
    NODE_CALL,
    NODE_STRING
} NodeKind;

typedef struct ASTNode ASTNode;
typedef struct ASTList ASTList;

struct ASTList {
    ASTNode *node;
    ASTList *next;
};

struct ASTNode {
    NodeKind kind;
    int line;
    TypeKind type;      /* declared type for decl/param/func return, inferred type for exprs */
    char *name;         /* identifier / function name / variable name */
    char *op;           /* operator string */
    long ival;
    double fval;
    int bval;
    char *str_val;
    ASTNode *a;
    ASTNode *b;
    ASTNode *c;
    ASTList *list;
};

ASTList *ast_list_append(ASTList *list, ASTNode *node);
ASTNode *ast_new_program(ASTList *items, int line);
ASTNode *ast_new_func(TypeKind ret_type, char *name, ASTList *params, ASTNode *body, int line);
ASTNode *ast_new_param(TypeKind type, char *name, int line);
ASTNode *ast_new_block(ASTList *stmts, int line);
ASTNode *ast_new_vardecl(TypeKind type, char *name, ASTNode *init, int line);
ASTNode *ast_new_set(char *name, ASTNode *expr, int line);
ASTNode *ast_new_show(ASTList *args, int line);
ASTNode *ast_new_if(ASTNode *cond, ASTNode *thenb, ASTNode *elseb, int line);
ASTNode *ast_new_while(ASTNode *cond, ASTNode *body, int line);
ASTNode *ast_new_return(ASTNode *expr, int line);
ASTNode *ast_new_exprstmt(ASTNode *expr, int line);
ASTNode *ast_new_int(long v, int line);
ASTNode *ast_new_float(double v, int line);
ASTNode *ast_new_bool(int v, int line);
ASTNode *ast_new_var(char *name, int line);
ASTNode *ast_new_binop(char *op, ASTNode *lhs, ASTNode *rhs, int line);
ASTNode *ast_new_unop(char *op, ASTNode *expr, int line);
ASTNode *ast_new_call(char *name, ASTList *args, int line);
ASTNode *ast_new_string(char *s, int line);

void ast_print(ASTNode *node, int indent);
void ast_free(ASTNode *node);

const char *type_name(TypeKind t);

#endif
