#ifndef IR_H
#define IR_H

#include "ast.h"

typedef enum {
    IR_FUNC = 0,
    IR_ENDFUNC,
    IR_LABEL,
    IR_GOTO,
    IR_IFZ,
    IR_ASSIGN,
    IR_BINOP,
    IR_UNOP,
    IR_PRINT,
    IR_READ,
    IR_PARAM,
    IR_CALL,
    IR_RETURN
} IRKind;

typedef struct {
    IRKind kind;
    char *a;
    char *b;
    char *c;
    char *d;
} IRInstr;

typedef struct {
    IRInstr *items;
    int count;
    int cap;
} IRList;

IRList *ir_generate(ASTNode *root);
void ir_print(IRList *ir);
void ir_free(IRList *ir);

#endif
