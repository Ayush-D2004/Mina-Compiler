#include "codegen.h"

#include <stdio.h>
#include <string.h>

static int is_immediate(const char *s) {
    return s && s[0] == '#';
}

static void emit_load(const char *op) {
    if (!op) return;
    if (is_immediate(op)) printf("    PUSH %s\n", op + 1);
    else printf("    LOAD %s\n", op);
}

static void emit_store(const char *dst) {
    printf("    STORE %s\n", dst);
}

void codegen_print(IRList *ir) {
    for (int i = 0; i < ir->count; ++i) {
        IRInstr *in = &ir->items[i];
        switch (in->kind) {
            case IR_FUNC:
                printf("FUNC %s\n", in->a);
                break;
            case IR_ENDFUNC:
                printf("END %s\n", in->a);
                break;
            case IR_LABEL:
                printf("%s:\n", in->a);
                break;
            case IR_GOTO:
                printf("    JMP %s\n", in->a);
                break;
            case IR_IFZ:
                emit_load(in->a);
                printf("    JZ %s\n", in->b);
                break;
            case IR_ASSIGN:
                emit_load(in->b);
                emit_store(in->a);
                break;
            case IR_BINOP:
                emit_load(in->c);
                emit_load(in->d);
                if (!strcmp(in->b, "+")) printf("    ADD\n");
                else if (!strcmp(in->b, "-")) printf("    SUB\n");
                else if (!strcmp(in->b, "*")) printf("    MUL\n");
                else if (!strcmp(in->b, "/")) printf("    DIV\n");
                else if (!strcmp(in->b, "%")) printf("    MOD\n");
                else if (!strcmp(in->b, "<")) printf("    LT\n");
                else if (!strcmp(in->b, ">")) printf("    GT\n");
                else if (!strcmp(in->b, "<=")) printf("    LE\n");
                else if (!strcmp(in->b, ">=")) printf("    GE\n");
                else if (!strcmp(in->b, "==")) printf("    EQ\n");
                else if (!strcmp(in->b, "!=")) printf("    NE\n");
                else if (!strcmp(in->b, "&&")) printf("    AND\n");
                else if (!strcmp(in->b, "||")) printf("    OR\n");
                emit_store(in->a);
                break;
            case IR_UNOP:
                emit_load(in->c);
                if (!strcmp(in->b, "-")) printf("    NEG\n");
                else if (!strcmp(in->b, "!")) printf("    NOT\n");
                emit_store(in->a);
                break;
            case IR_PRINT:
                emit_load(in->a);
                printf("    PRINT\n");
                break;
            case IR_READ:
                printf("    READ %s\n", in->a);
                break;
            case IR_PARAM:
                emit_load(in->a);
                printf("    PUSHARG\n");
                break;
            case IR_CALL:
                printf("    CALL %s, %s\n", in->b, in->c);
                if (in->a && in->a[0]) emit_store(in->a);
                break;
            case IR_RETURN:
                if (in->a && in->a[0]) emit_load(in->a);
                printf("    RET\n");
                break;
        }
    }
}
