#include <stdio.h>
#include <stdlib.h>

#include "ast.h"
#include "semantic.h"
#include "optimize.h"
#include "ir.h"
#include "codegen.h"
#include "symbol_table.h"
#include "runtime.h"

extern int yyparse(void);
extern FILE *yyin;
extern ASTNode *g_root;

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin) {
        perror("fopen");
        return 1;
    }

    if (yyparse() != 0 || !g_root) {
        fprintf(stderr, "parsing failed\n");
        fclose(yyin);
        return 1;
    }
    fclose(yyin);

    puts("==== AST ====");
    ast_print(g_root, 0);

    semantic_analyze(g_root);
    print_symbol_table();

    ASTNode *optimized = optimize_ast(g_root);
    puts("==== OPTIMIZED AST ====");
    ast_print(optimized, 0);

    IRList *ir = ir_generate(optimized);
    puts("==== 3 ADDRESS CODE ====");
    ir_print(ir);

    puts("==== TARGET CODE ====");
    codegen_print(ir);

    puts("==== PROGRAM OUTPUT ====");
    char *out = runtime_execute(optimized);
    fputs(out, stdout);
    free(out);

    ir_free(ir);
    symtab_free_scope(g_root_scope_ref);
    ast_free(g_root);
    return 0;
}
