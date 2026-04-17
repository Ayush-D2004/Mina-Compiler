#include "symbol_table.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Scope *g_root_scope_ref = NULL;

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

Scope *symtab_new_scope(Scope *parent) {
    Scope *s = (Scope *)calloc(1, sizeof(Scope));
    if (!s) {
        perror("calloc");
        exit(1);
    }
    s->parent = parent;
    return s;
}

static void free_symbols(Symbol *sym) {
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym->params);
        free(sym);
        sym = next;
    }
}

void symtab_free_scope(Scope *scope) {
    if (!scope) return;
    free_symbols(scope->symbols);
    free(scope);
}

Symbol *symtab_lookup_current(Scope *scope, const char *name) {
    if (!scope) return NULL;
    for (Symbol *s = scope->symbols; s; s = s->next) {
        if (strcmp(s->name, name) == 0) return s;
    }
    return NULL;
}

Symbol *symtab_lookup(Scope *scope, const char *name) {
    for (Scope *cur = scope; cur; cur = cur->parent) {
        Symbol *s = symtab_lookup_current(cur, name);
        if (s) return s;
    }
    return NULL;
}

static Symbol *insert_common(Scope *scope, const char *name, SymbolKind kind, TypeKind type) {
    if (!scope) return NULL;
    if (symtab_lookup_current(scope, name)) return NULL;

    Symbol *s = (Symbol *)calloc(1, sizeof(Symbol));
    if (!s) {
        perror("calloc");
        exit(1);
    }
    s->name = xstrdup(name);
    s->kind = kind;
    s->type = type;
    s->next = scope->symbols;
    scope->symbols = s;
    return s;
}

Symbol *symtab_insert_var(Scope *scope, const char *name, TypeKind type) {
    return insert_common(scope, name, SYM_VAR, type);
}

Symbol *symtab_insert_func(Scope *scope, const char *name, TypeKind ret_type, TypeKind *params, int param_count, ASTNode *func_node) {
    Symbol *s = insert_common(scope, name, SYM_FUNC, ret_type);
    if (!s) return NULL;
    s->params = params;
    s->param_count = param_count;
    s->func_node = func_node;
    return s;
}

void symtab_print(Scope *scope) {
    if (!scope) return;
    for (Symbol *s = scope->symbols; s; s = s->next) {
        printf("%-15s %-10s %-10s\n",
               s->name,
               type_name(s->type),
               s->kind == SYM_FUNC ? "FUNC" : "VAR");
    }
}

void print_symbol_table(void) {
    printf("\n==== SYMBOL TABLE ====\n");
    printf("%-15s %-10s %-10s\n", "Name", "Type", "Kind");
    symtab_print(g_root_scope_ref);
}
