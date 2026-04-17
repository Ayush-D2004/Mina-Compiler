#include "ast.h"

#include <stdlib.h>
#include <string.h>

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

const char *type_name(TypeKind t) {
    switch (t) {
        case TYPE_NUM: return "num";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        case TYPE_STRING: return "string";
        default: return "unknown";
    }
}

ASTList *ast_list_append(ASTList *list, ASTNode *node) {
    ASTList *n = (ASTList *)calloc(1, sizeof(ASTList));
    if (!n) {
        perror("calloc");
        exit(1);
    }
    n->node = node;
    if (!list) return n;
    ASTList *cur = list;
    while (cur->next) cur = cur->next;
    cur->next = n;
    return list;
}

static ASTNode *new_node(NodeKind kind, int line) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    if (!n) {
        perror("calloc");
        exit(1);
    }
    n->kind = kind;
    n->line = line;
    n->type = TYPE_UNKNOWN;
    return n;
}

ASTNode *ast_new_program(ASTList *items, int line) {
    ASTNode *n = new_node(NODE_PROGRAM, line);
    n->list = items;
    return n;
}

ASTNode *ast_new_func(TypeKind ret_type, char *name, ASTList *params, ASTNode *body, int line) {
    ASTNode *n = new_node(NODE_FUNC, line);
    n->type = ret_type;
    n->name = xstrdup(name);
    n->list = params;
    n->a = body;
    return n;
}

ASTNode *ast_new_param(TypeKind type, char *name, int line) {
    ASTNode *n = new_node(NODE_PARAM, line);
    n->type = type;
    n->name = xstrdup(name);
    return n;
}

ASTNode *ast_new_block(ASTList *stmts, int line) {
    ASTNode *n = new_node(NODE_BLOCK, line);
    n->list = stmts;
    return n;
}

ASTNode *ast_new_vardecl(TypeKind type, char *name, ASTNode *init, int line) {
    ASTNode *n = new_node(NODE_VAR_DECL, line);
    n->type = type;
    n->name = xstrdup(name);
    n->a = init;
    return n;
}

ASTNode *ast_new_set(char *name, ASTNode *expr, int line) {
    ASTNode *n = new_node(NODE_SET, line);
    n->name = xstrdup(name);
    n->a = expr;
    return n;
}

ASTNode *ast_new_show(ASTList *args, int line) {
    ASTNode *n = new_node(NODE_SHOW, line);
    n->list = args;
    return n;
}

ASTNode *ast_new_string(char *s, int line) {
    ASTNode *n = new_node(NODE_STRING, line);
    if (s && s[0] == '"') {
        size_t len = strlen(s);
        if (len >= 2 && s[len - 1] == '"') {
            char *buf = xstrdup(s + 1);
            buf[len - 2] = '\0';
            n->str_val = buf;
            return n;
        }
    }
    n->str_val = xstrdup(s);
    return n;
}

ASTNode *ast_new_if(ASTNode *cond, ASTNode *thenb, ASTNode *elseb, int line) {
    ASTNode *n = new_node(NODE_IF, line);
    n->a = cond;
    n->b = thenb;
    n->c = elseb;
    return n;
}

ASTNode *ast_new_while(ASTNode *cond, ASTNode *body, int line) {
    ASTNode *n = new_node(NODE_WHILE, line);
    n->a = cond;
    n->b = body;
    return n;
}

ASTNode *ast_new_return(ASTNode *expr, int line) {
    ASTNode *n = new_node(NODE_RETURN, line);
    n->a = expr;
    return n;
}

ASTNode *ast_new_exprstmt(ASTNode *expr, int line) {
    ASTNode *n = new_node(NODE_EXPR_STMT, line);
    n->a = expr;
    return n;
}

ASTNode *ast_new_int(long v, int line) {
    ASTNode *n = new_node(NODE_INT, line);
    n->type = TYPE_NUM;
    n->ival = v;
    return n;
}

ASTNode *ast_new_float(double v, int line) {
    ASTNode *n = new_node(NODE_FLOAT, line);
    n->type = TYPE_NUM;
    n->fval = v;
    return n;
}

ASTNode *ast_new_bool(int v, int line) {
    ASTNode *n = new_node(NODE_BOOL, line);
    n->type = TYPE_BOOL;
    n->bval = v ? 1 : 0;
    return n;
}

ASTNode *ast_new_var(char *name, int line) {
    ASTNode *n = new_node(NODE_VAR, line);
    n->name = xstrdup(name);
    return n;
}

ASTNode *ast_new_binop(char *op, ASTNode *lhs, ASTNode *rhs, int line) {
    ASTNode *n = new_node(NODE_BINOP, line);
    n->op = xstrdup(op);
    n->a = lhs;
    n->b = rhs;
    return n;
}

ASTNode *ast_new_unop(char *op, ASTNode *expr, int line) {
    ASTNode *n = new_node(NODE_UNOP, line);
    n->op = xstrdup(op);
    n->a = expr;
    return n;
}

ASTNode *ast_new_call(char *name, ASTList *args, int line) {
    ASTNode *n = new_node(NODE_CALL, line);
    n->name = xstrdup(name);
    n->list = args;
    return n;
}

static void indent(int n) {
    for (int i = 0; i < n; ++i) fputs("  ", stdout);
}

static void print_list(ASTList *list, int indent_level);
static void print_node(ASTNode *n, int indent_level);

static void print_list(ASTList *list, int indent_level) {
    for (ASTList *cur = list; cur; cur = cur->next) {
        print_node(cur->node, indent_level);
    }
}

static void print_node(ASTNode *n, int indent_level) {
    if (!n) {
        indent(indent_level);
        puts("(null)");
        return;
    }

    switch (n->kind) {
        case NODE_PROGRAM:
            indent(indent_level); puts("PROGRAM");
            print_list(n->list, indent_level + 1);
            break;
        case NODE_FUNC:
            indent(indent_level);
            printf("FUNC %s : %s\n", n->name, type_name(n->type));
            if (n->list) {
                for (ASTList *p = n->list; p; p = p->next) {
                    print_node(p->node, indent_level + 1);
                }
            }
            if (n->a) print_node(n->a, indent_level + 1);
            break;
        case NODE_PARAM:
            indent(indent_level);
            printf("PARAM %s : %s\n", n->name, type_name(n->type));
            break;
        case NODE_BLOCK:
            indent(indent_level); puts("BLOCK");
            print_list(n->list, indent_level + 1);
            break;
        case NODE_VAR_DECL:
            indent(indent_level);
            printf("LET %s : %s\n", n->name, type_name(n->type));
            if (n->a) print_node(n->a, indent_level + 1);
            break;
        case NODE_SET:
            indent(indent_level);
            printf("SET %s\n", n->name);
            print_node(n->a, indent_level + 1);
            break;
        case NODE_SHOW:
            indent(indent_level); puts("SHOW");
            print_list(n->list, indent_level + 1);
            break;
        case NODE_IF:
            indent(indent_level); puts("WHEN");
            print_node(n->a, indent_level + 1);
            print_node(n->b, indent_level + 1);
            if (n->c) {
                indent(indent_level); puts("OTHERWISE");
                print_node(n->c, indent_level + 1);
            }
            break;
        case NODE_WHILE:
            indent(indent_level); puts("REPEAT");
            print_node(n->a, indent_level + 1);
            print_node(n->b, indent_level + 1);
            break;
        case NODE_RETURN:
            indent(indent_level); puts("GIVE");
            if (n->a) print_node(n->a, indent_level + 1);
            else { indent(indent_level + 1); puts("(void)"); }
            break;
        case NODE_EXPR_STMT:
            indent(indent_level); puts("EXPR");
            print_node(n->a, indent_level + 1);
            break;
        case NODE_INT:
            indent(indent_level); printf("NUM %ld\n", n->ival); break;
        case NODE_FLOAT:
            indent(indent_level); printf("NUM %g\n", n->fval); break;
        case NODE_BOOL:
            indent(indent_level); printf("BOOL %s\n", n->bval ? "true" : "false"); break;
        case NODE_STRING:
            indent(indent_level); printf("STRING %s\n", n->str_val); break;
        case NODE_VAR:
            indent(indent_level); printf("VAR %s\n", n->name); break;
        case NODE_BINOP:
            indent(indent_level); printf("BINOP %s\n", n->op);
            print_node(n->a, indent_level + 1);
            print_node(n->b, indent_level + 1);
            break;
        case NODE_UNOP:
            indent(indent_level); printf("UNOP %s\n", n->op);
            print_node(n->a, indent_level + 1);
            break;
        case NODE_CALL:
            indent(indent_level); printf("CALL %s\n", n->name);
            print_list(n->list, indent_level + 1);
            break;
        default:
            indent(indent_level); puts("<unknown>");
            break;
    }
}

void ast_print(ASTNode *node, int indent_level) {
    print_node(node, indent_level);
}

static void free_list(ASTList *list) {
    while (list) {
        ASTList *next = list->next;
        ast_free(list->node);
        free(list);
        list = next;
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;
    free(node->name);
    free(node->op);
    free(node->str_val);
    switch (node->kind) {
        case NODE_PROGRAM:
            free_list(node->list);
            break;
        case NODE_FUNC:
            free_list(node->list);
            ast_free(node->a);
            break;
        case NODE_BLOCK:
            free_list(node->list);
            break;
        case NODE_VAR_DECL:
        case NODE_SET:
        case NODE_WHILE:
        case NODE_RETURN:
        case NODE_EXPR_STMT:
        case NODE_IF:
        case NODE_BINOP:
        case NODE_UNOP:
        case NODE_CALL:
            ast_free(node->a);
            ast_free(node->b);
            ast_free(node->c);
            free_list(node->list);
            break;
        default:
            break;
    }
    free(node);
}
