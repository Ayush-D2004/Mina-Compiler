#include "optimize.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static ASTNode *opt_node(ASTNode *n);

static int is_true(ASTNode *n) {
    return n && n->kind == NODE_BOOL && n->bval != 0;
}

static int is_false(ASTNode *n) {
    return n && n->kind == NODE_BOOL && n->bval == 0;
}

static int as_num(ASTNode *n, double *out) {
    if (!n) return 0;
    if (n->kind == NODE_INT) { *out = (double)n->ival; return 1; }
    if (n->kind == NODE_FLOAT) { *out = n->fval; return 1; }
    return 0;
}

static ASTNode *mk_num(double v, int line) {
    long lv = (long)v;
    if ((double)lv == v) return ast_new_int(lv, line);
    return ast_new_float(v, line);
}

static ASTList *opt_list(ASTList *list) {
    ASTList *head = NULL;
    ASTList *tail = NULL;
    for (ASTList *cur = list; cur; cur = cur->next) {
        ASTNode *o = opt_node(cur->node);
        ASTList *nn = (ASTList *)calloc(1, sizeof(ASTList));
        nn->node = o;
        if (!head) head = tail = nn;
        else { tail->next = nn; tail = nn; }
    }
    return head;
}

static ASTNode *opt_node(ASTNode *n) {
    if (!n) return NULL;

    switch (n->kind) {
        case NODE_PROGRAM:
            n->list = opt_list(n->list);
            return n;
        case NODE_FUNC:
            n->list = opt_list(n->list);
            n->a = opt_node(n->a);
            return n;
        case NODE_BLOCK:
            n->list = opt_list(n->list);
            return n;
        case NODE_VAR_DECL:
        case NODE_SET:
        case NODE_SHOW:
        case NODE_RETURN:
        case NODE_EXPR_STMT:
            n->a = opt_node(n->a);
            return n;
        case NODE_IF:
            n->a = opt_node(n->a);
            n->b = opt_node(n->b);
            n->c = opt_node(n->c);
            if (is_true(n->a)) return n->b ? n->b : ast_new_block(NULL, n->line);
            if (is_false(n->a)) return n->c ? n->c : ast_new_block(NULL, n->line);
            return n;
        case NODE_WHILE:
            n->a = opt_node(n->a);
            n->b = opt_node(n->b);
            if (is_false(n->a)) return ast_new_block(NULL, n->line);
            return n;
        case NODE_UNOP: {
            n->a = opt_node(n->a);
            if (n->a && (n->a->kind == NODE_INT || n->a->kind == NODE_FLOAT || n->a->kind == NODE_BOOL)) {
                if (!strcmp(n->op, "-")) {
                    double v;
                    if (as_num(n->a, &v)) return mk_num(-v, n->line);
                } else if (!strcmp(n->op, "!")) {
                    if (n->a->kind == NODE_BOOL) return ast_new_bool(!n->a->bval, n->line);
                }
            }
            return n;
        }
        case NODE_BINOP: {
            n->a = opt_node(n->a);
            n->b = opt_node(n->b);
            double lv, rv;
            if (as_num(n->a, &lv) && as_num(n->b, &rv)) {
                if (!strcmp(n->op, "+")) return mk_num(lv + rv, n->line);
                if (!strcmp(n->op, "-")) return mk_num(lv - rv, n->line);
                if (!strcmp(n->op, "*")) return mk_num(lv * rv, n->line);
                if (!strcmp(n->op, "/")) return mk_num(lv / rv, n->line);
                if (!strcmp(n->op, "%")) return ast_new_int((long)lv % (long)rv, n->line);
                if (!strcmp(n->op, "<")) return ast_new_bool(lv < rv, n->line);
                if (!strcmp(n->op, ">")) return ast_new_bool(lv > rv, n->line);
                if (!strcmp(n->op, "<=")) return ast_new_bool(lv <= rv, n->line);
                if (!strcmp(n->op, ">=")) return ast_new_bool(lv >= rv, n->line);
                if (!strcmp(n->op, "==")) return ast_new_bool(lv == rv, n->line);
                if (!strcmp(n->op, "!=")) return ast_new_bool(lv != rv, n->line);
            }
            if (n->a && n->a->kind == NODE_BOOL && n->b && n->b->kind == NODE_BOOL) {
                if (!strcmp(n->op, "&&")) return ast_new_bool(n->a->bval && n->b->bval, n->line);
                if (!strcmp(n->op, "||")) return ast_new_bool(n->a->bval || n->b->bval, n->line);
                if (!strcmp(n->op, "==")) return ast_new_bool(n->a->bval == n->b->bval, n->line);
                if (!strcmp(n->op, "!=")) return ast_new_bool(n->a->bval != n->b->bval, n->line);
            }
            return n;
        }
        case NODE_CALL:
            for (ASTList *cur = n->list; cur; cur = cur->next) cur->node = opt_node(cur->node);
            return n;
        default:
            return n;
    }
}

ASTNode *optimize_ast(ASTNode *root) {
    return opt_node(root);
}
