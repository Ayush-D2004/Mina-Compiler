#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "ast.h"

typedef enum {
    SYM_VAR = 0,
    SYM_FUNC
} SymbolKind;

typedef struct Symbol Symbol;
typedef struct Scope Scope;

struct Symbol {
    char *name;
    SymbolKind kind;
    TypeKind type;          /* variable type or function return type */
    TypeKind *params;
    int param_count;
    ASTNode *func_node;     /* for functions */
    int initialized;
    Symbol *next;
};

struct Scope {
    Symbol *symbols;
    Scope *parent;
};

Scope *symtab_new_scope(Scope *parent);
void symtab_free_scope(Scope *scope);

Symbol *symtab_insert_var(Scope *scope, const char *name, TypeKind type);
Symbol *symtab_insert_func(Scope *scope, const char *name, TypeKind ret_type, TypeKind *params, int param_count, ASTNode *func_node);
Symbol *symtab_lookup(Scope *scope, const char *name);
Symbol *symtab_lookup_current(Scope *scope, const char *name);

void symtab_print(Scope *scope);
void print_symbol_table(void);
extern Scope *g_root_scope_ref;

#endif
