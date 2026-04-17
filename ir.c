#include "ir.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (!p) {
        perror("malloc");
        exit(1);
    }
    memcpy(p, s, n);
    return p;
}

static char *xasprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (n < 0) {
        va_end(ap2);
        return NULL;
    }
    char *buf = (char *)malloc((size_t)n + 1);
    if (!buf) {
        perror("malloc");
        exit(1);
    }
    vsnprintf(buf, (size_t)n + 1, fmt, ap2);
    va_end(ap2);
    return buf;
}

static IRList *ir_new(void) {
    IRList *ir = (IRList *)calloc(1, sizeof(IRList));
    if (!ir) {
        perror("calloc");
        exit(1);
    }
    return ir;
}

static void ir_push(IRList *ir, IRKind kind, const char *a, const char *b, const char *c, const char *d) {
    if (ir->count == ir->cap) {
        ir->cap = ir->cap ? ir->cap * 2 : 64;
        ir->items = (IRInstr *)realloc(ir->items, (size_t)ir->cap * sizeof(IRInstr));
        if (!ir->items) {
            perror("realloc");
            exit(1);
        }
    }
    IRInstr *ins = &ir->items[ir->count++];
    ins->kind = kind;
    ins->a = xstrdup(a);
    ins->b = xstrdup(b);
    ins->c = xstrdup(c);
    ins->d = xstrdup(d);
}

static int temp_id = 0;
static int label_id = 0;

static char *new_temp(void) {
    return xasprintf("t%d", ++temp_id);
}

static char *new_label(void) {
    return xasprintf("L%d", ++label_id);
}

static char *literal_of(ASTNode *n) {
    if (!n) return xstrdup("0");
    switch (n->kind) {
        case NODE_INT:
            return xasprintf("#%ld", n->ival);
        case NODE_FLOAT:
            return xasprintf("#%g", n->fval);
        case NODE_BOOL:
            return xstrdup(n->bval ? "#true" : "#false");
        default:
            return NULL;
    }
}

static char *gen_expr(ASTNode *n, IRList *ir);

static void gen_stmt(ASTNode *n, IRList *ir) {
    if (!n) return;
    switch (n->kind) {
        case NODE_BLOCK:
            for (ASTList *cur = n->list; cur; cur = cur->next) gen_stmt(cur->node, ir);
            break;
        case NODE_VAR_DECL:
            if (n->a) {
                char *src = gen_expr(n->a, ir);
                ir_push(ir, IR_ASSIGN, n->name, src, NULL, NULL);
                free(src);
            }
            break;
        case NODE_SET: {
            char *src = gen_expr(n->a, ir);
            ir_push(ir, IR_ASSIGN, n->name, src, NULL, NULL);
            free(src);
            break;
        }
        case NODE_SHOW: {
            char *src = gen_expr(n->a, ir);
            ir_push(ir, IR_PRINT, src, NULL, NULL, NULL);
            free(src);
            break;
        }
        case NODE_IF: {
            char *cond = gen_expr(n->a, ir);
            char *l_else = new_label();
            char *l_end = new_label();
            ir_push(ir, IR_IFZ, cond, l_else, NULL, NULL);
            free(cond);
            gen_stmt(n->b, ir);
            ir_push(ir, IR_GOTO, l_end, NULL, NULL, NULL);
            ir_push(ir, IR_LABEL, l_else, NULL, NULL, NULL);
            if (n->c) gen_stmt(n->c, ir);
            ir_push(ir, IR_LABEL, l_end, NULL, NULL, NULL);
            free(l_else);
            free(l_end);
            break;
        }
        case NODE_WHILE: {
            char *l_start = new_label();
            char *l_end = new_label();
            ir_push(ir, IR_LABEL, l_start, NULL, NULL, NULL);
            char *cond = gen_expr(n->a, ir);
            ir_push(ir, IR_IFZ, cond, l_end, NULL, NULL);
            free(cond);
            gen_stmt(n->b, ir);
            ir_push(ir, IR_GOTO, l_start, NULL, NULL, NULL);
            ir_push(ir, IR_LABEL, l_end, NULL, NULL, NULL);
            free(l_start);
            free(l_end);
            break;
        }
        case NODE_RETURN: {
            if (n->a) {
                char *src = gen_expr(n->a, ir);
                ir_push(ir, IR_RETURN, src, NULL, NULL, NULL);
                free(src);
            } else {
                ir_push(ir, IR_RETURN, NULL, NULL, NULL, NULL);
            }
            break;
        }
        case NODE_EXPR_STMT:
            (void)gen_expr(n->a, ir);
            break;
        default:
            break;
    }
}

static char *gen_call(ASTNode *n, IRList *ir) {
    if (strcmp(n->name, "ask") == 0) {
        char *t = new_temp();
        ir_push(ir, IR_READ, t, NULL, NULL, NULL);
        return t;
    }

    int argc = 0;
    for (ASTList *cur = n->list; cur; cur = cur->next) {
        char *arg = gen_expr(cur->node, ir);
        ir_push(ir, IR_PARAM, arg, NULL, NULL, NULL);
        free(arg);
        argc++;
    }

    char *t = new_temp();
    char *argc_s = xasprintf("%d", argc);
    ir_push(ir, IR_CALL, t, n->name, argc_s, NULL);
    free(argc_s);
    return t;
}

static char *gen_expr(ASTNode *n, IRList *ir) {
    if (!n) return xstrdup("#0");
    switch (n->kind) {
        case NODE_INT: case NODE_FLOAT: case NODE_BOOL:
            return literal_of(n);
        case NODE_VAR:
            return xstrdup(n->name);
        case NODE_CALL:
            return gen_call(n, ir);
        case NODE_UNOP: {
            char *x = gen_expr(n->a, ir);
            char *t = new_temp();
            ir_push(ir, IR_UNOP, t, n->op, x, NULL);
            free(x);
            return t;
        }
        case NODE_BINOP: {
            char *l = gen_expr(n->a, ir);
            char *r = gen_expr(n->b, ir);
            char *t = new_temp();
            ir_push(ir, IR_BINOP, t, n->op, l, r);
            free(l); free(r);
            return t;
        }
        default:
            return xstrdup("#0");
    }
}

static void gen_func(ASTNode *func, IRList *ir) {
    ir_push(ir, IR_FUNC, func->name, NULL, NULL, NULL);
    gen_stmt(func->a, ir);
    ir_push(ir, IR_ENDFUNC, func->name, NULL, NULL, NULL);
}

IRList *ir_generate(ASTNode *root) {
    IRList *ir = ir_new();
    if (!root || root->kind != NODE_PROGRAM) return ir;
    for (ASTList *cur = root->list; cur; cur = cur->next) gen_func(cur->node, ir);
    return ir;
}

void ir_print(IRList *ir) {
    for (int i = 0; i < ir->count; ++i) {
        IRInstr *in = &ir->items[i];
        switch (in->kind) {
            case IR_FUNC:    printf("FUNC %s\n", in->a); break;
            case IR_ENDFUNC: printf("ENDFUNC %s\n", in->a); break;
            case IR_LABEL:   printf("%s:\n", in->a); break;
            case IR_GOTO:    printf("  goto %s\n", in->a); break;
            case IR_IFZ:     printf("  ifz %s goto %s\n", in->a, in->b); break;
            case IR_ASSIGN:  printf("  %s = %s\n", in->a, in->b); break;
            case IR_BINOP:   printf("  %s = %s %s %s\n", in->a, in->c, in->b, in->d); break;
            case IR_UNOP:    printf("  %s = %s %s\n", in->a, in->b, in->c); break;
            case IR_PRINT:   printf("  print %s\n", in->a); break;
            case IR_READ:    printf("  %s = read()\n", in->a); break;
            case IR_PARAM:   printf("  param %s\n", in->a); break;
            case IR_CALL:    printf("  %s = call %s, %s\n", in->a, in->b, in->c); break;
            case IR_RETURN:  printf("  return %s\n", in->a ? in->a : ""); break;
        }
    }
}

void ir_free(IRList *ir) {
    if (!ir) return;
    for (int i = 0; i < ir->count; ++i) {
        free(ir->items[i].a);
        free(ir->items[i].b);
        free(ir->items[i].c);
        free(ir->items[i].d);
    }
    free(ir->items);
    free(ir);
}
