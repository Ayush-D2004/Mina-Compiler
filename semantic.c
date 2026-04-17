#include "semantic.h"
#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Scope *scope;
    TypeKind current_return;
    int in_function;
} SemContext;

static void sem_error(int line, const char *msg) {
    fprintf(stderr, "Semantic error at line %d: %s\n", line, msg);
    exit(1);
}

static Symbol *ensure_symbol(Scope *scope, const char *name, int line) {
    Symbol *s = symtab_lookup(scope, name);
    if (!s) {
        char buf[256];
        snprintf(buf, sizeof(buf), "undeclared identifier '%s'", name);
        sem_error(line, buf);
    }
    return s;
}

static TypeKind sem_expr(ASTNode *n, SemContext *ctx);

static void sem_stmt(ASTNode *n, SemContext *ctx);

static TypeKind sem_expr(ASTNode *n, SemContext *ctx) {
    if (!n) return TYPE_VOID;
    switch (n->kind) {
        case NODE_INT:
        case NODE_FLOAT:
            n->type = TYPE_NUM;
            return TYPE_NUM;
        case NODE_BOOL:
            n->type = TYPE_BOOL;
            return TYPE_BOOL;
        case NODE_STRING:
            n->type = TYPE_STRING;
            return TYPE_STRING;
        case NODE_VAR: {
            Symbol *s = ensure_symbol(ctx->scope, n->name, n->line);
            if (s->kind != SYM_VAR) sem_error(n->line, "expected a variable, found a function");
            n->type = s->type;
            return s->type;
        }
        case NODE_CALL: {
            if (strcmp(n->name, "ask") == 0) {
                if (n->list) sem_error(n->line, "ask() takes no arguments");
                n->type = TYPE_NUM;
                return TYPE_NUM;
            }
            Symbol *s = ensure_symbol(ctx->scope, n->name, n->line);
            if (s->kind != SYM_FUNC) sem_error(n->line, "expected a function call");
            int argc = 0;
            for (ASTList *a = n->list; a; a = a->next) {
                TypeKind t = sem_expr(a->node, ctx);
                if (argc >= s->param_count) sem_error(n->line, "too many call arguments");
                if (t != s->params[argc]) sem_error(n->line, "call argument type mismatch");
                argc++;
            }
            if (argc != s->param_count) sem_error(n->line, "wrong number of call arguments");
            n->type = s->type;
            return s->type;
        }
        case NODE_UNOP: {
            TypeKind t = sem_expr(n->a, ctx);
            if (strcmp(n->op, "-") == 0) {
                if (t != TYPE_NUM) sem_error(n->line, "unary '-' expects a numeric operand");
                n->type = TYPE_NUM;
                return TYPE_NUM;
            }
            if (strcmp(n->op, "!") == 0) {
                if (t != TYPE_BOOL) sem_error(n->line, "logical '!' expects a boolean operand");
                n->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            sem_error(n->line, "unknown unary operator");
            return TYPE_UNKNOWN;
        }
        case NODE_BINOP: {
            TypeKind lt = sem_expr(n->a, ctx);
            TypeKind rt = sem_expr(n->b, ctx);
            const char *op = n->op;

            if (!strcmp(op, "+") || !strcmp(op, "-") || !strcmp(op, "*") || !strcmp(op, "/") || !strcmp(op, "%")) {
                if (lt != TYPE_NUM || rt != TYPE_NUM) sem_error(n->line, "arithmetic operator expects numeric operands");
                n->type = TYPE_NUM;
                return TYPE_NUM;
            }
            if (!strcmp(op, "<") || !strcmp(op, ">") || !strcmp(op, "<=") || !strcmp(op, ">=")) {
                if (lt != TYPE_NUM || rt != TYPE_NUM) sem_error(n->line, "comparison operator expects numeric operands");
                n->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            if (!strcmp(op, "==") || !strcmp(op, "!=")) {
                if (lt != rt) sem_error(n->line, "equality operator expects operands of the same type");
                n->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            if (!strcmp(op, "&&") || !strcmp(op, "||")) {
                if (lt != TYPE_BOOL || rt != TYPE_BOOL) sem_error(n->line, "logical operator expects boolean operands");
                n->type = TYPE_BOOL;
                return TYPE_BOOL;
            }
            sem_error(n->line, "unknown binary operator");
            return TYPE_UNKNOWN;
        }
        default:
            sem_error(n->line, "invalid expression node");
    }
    return TYPE_UNKNOWN;
}

static void sem_block(ASTNode *block, SemContext *ctx) {
    Scope *saved = ctx->scope;
    ctx->scope = symtab_new_scope(saved);
    for (ASTList *cur = block->list; cur; cur = cur->next) sem_stmt(cur->node, ctx);
    symtab_free_scope(ctx->scope);
    ctx->scope = saved;
}

static void sem_stmt(ASTNode *n, SemContext *ctx) {
    if (!n) return;
    switch (n->kind) {
        case NODE_BLOCK:
            sem_block(n, ctx);
            break;
        case NODE_VAR_DECL: {
            if (symtab_lookup_current(ctx->scope, n->name)) sem_error(n->line, "duplicate declaration in the same scope");
            if (!symtab_insert_var(ctx->scope, n->name, n->type)) sem_error(n->line, "failed to insert variable");
            if (n->a) {
                TypeKind t = sem_expr(n->a, ctx);
                if (t != n->type) sem_error(n->line, "initializer type mismatch");
            }
            break;
        }
        case NODE_SET: {
            Symbol *s = ensure_symbol(ctx->scope, n->name, n->line);
            if (s->kind != SYM_VAR) sem_error(n->line, "cannot assign to a function");
            TypeKind t = sem_expr(n->a, ctx);
            if (t != s->type) sem_error(n->line, "assignment type mismatch");
            break;
        }
        case NODE_SHOW: {
            for (ASTList *p = n->list; p; p = p->next) {
                TypeKind t = sem_expr(p->node, ctx);
                if (t == TYPE_VOID) sem_error(n->line, "show() cannot print void");
            }
            break;
        }
        case NODE_IF: {
            TypeKind t = sem_expr(n->a, ctx);
            if (t != TYPE_BOOL) sem_error(n->line, "when condition must be boolean");
            sem_stmt(n->b, ctx);
            if (n->c) sem_stmt(n->c, ctx);
            break;
        }
        case NODE_WHILE: {
            TypeKind t = sem_expr(n->a, ctx);
            if (t != TYPE_BOOL) sem_error(n->line, "repeat condition must be boolean");
            sem_stmt(n->b, ctx);
            break;
        }
        case NODE_RETURN: {
            if (ctx->current_return == TYPE_VOID) {
                if (n->a) sem_error(n->line, "void function cannot return a value");
            } else {
                if (!n->a) sem_error(n->line, "non-void function must return a value");
                TypeKind t = sem_expr(n->a, ctx);
                if (t != ctx->current_return) sem_error(n->line, "return type mismatch");
            }
            break;
        }
        case NODE_EXPR_STMT:
            sem_expr(n->a, ctx);
            break;
        default:
            sem_error(n->line, "unexpected statement node");
    }
}

static int count_params(ASTList *params) {
    int n = 0;
    for (ASTList *cur = params; cur; cur = cur->next) n++;
    return n;
}

static TypeKind *collect_param_types(ASTList *params, int count) {
    TypeKind *arr = (TypeKind *)calloc((size_t)count, sizeof(TypeKind));
    if (!arr) {
        perror("calloc");
        exit(1);
    }
    int i = 0;
    for (ASTList *cur = params; cur; cur = cur->next) {
        arr[i++] = cur->node->type;
    }
    return arr;
}

static void sem_func(ASTNode *func, SemContext *global) {
    SemContext ctx = *global;
    ctx.scope = symtab_new_scope(global->scope);
    ctx.current_return = func->type;
    ctx.in_function = 1;

    int i = 0;
    for (ASTList *cur = func->list; cur; cur = cur->next, ++i) {
        ASTNode *p = cur->node;
        if (p->kind != NODE_PARAM) sem_error(p->line, "internal error: expected param node");
        if (symtab_lookup_current(ctx.scope, p->name)) sem_error(p->line, "duplicate parameter name");
        if (!symtab_insert_var(ctx.scope, p->name, p->type)) sem_error(p->line, "failed to insert parameter");
    }

    sem_stmt(func->a, &ctx);
    symtab_free_scope(ctx.scope);
}

void semantic_analyze(ASTNode *root) {
    if (!root || root->kind != NODE_PROGRAM) sem_error(root ? root->line : 0, "program root is invalid");

    Scope *global = symtab_new_scope(NULL);

    /* built-in input function */
    TypeKind *ask_params = NULL;
    if (!symtab_insert_func(global, "ask", TYPE_NUM, ask_params, 0, NULL))
        sem_error(root->line, "failed to register builtin ask()");

    for (ASTList *cur = root->list; cur; cur = cur->next) {
        ASTNode *f = cur->node;
        if (f->kind != NODE_FUNC) sem_error(f->line, "top-level items must be function definitions");
        int pc = count_params(f->list);
        TypeKind *pt = collect_param_types(f->list, pc);
        if (!symtab_insert_func(global, f->name, f->type, pt, pc, f)) {
            free(pt);
            sem_error(f->line, "duplicate function name");
        }
    }

    Symbol *mainf = symtab_lookup(global, "main");
    if (!mainf || mainf->kind != SYM_FUNC) sem_error(root->line, "program must define proc main()");
    if (mainf->param_count != 0) sem_error(root->line, "main() must not take parameters");

    for (ASTList *cur = root->list; cur; cur = cur->next) sem_func(cur->node, &(SemContext){ .scope = global, .current_return = TYPE_VOID, .in_function = 0 });

    g_root_scope_ref = global;
}
