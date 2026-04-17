#include "runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

typedef struct Value {
    TypeKind type;
    double num;
    int boolean;
    int is_void;
    char *str;
} Value;

typedef struct VarBinding {
    char *name;
    TypeKind type;
    double num;
    int boolean;
    char *str;
    struct VarBinding *next;
} VarBinding;

typedef struct Frame {
    VarBinding *vars;
    struct Frame *parent;
} Frame;

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} StrBuf;

typedef struct {
    int returned;
    Value value;
} ExecResult;

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

static void sb_init(StrBuf *sb) {
    sb->buf = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static Value make_num(double v) { Value x = { TYPE_NUM, v, 0, 0, NULL }; return x; }
static Value make_bool(int v) { Value x = { TYPE_BOOL, 0.0, v ? 1 : 0, 0, NULL }; return x; }
static Value make_void(void) { Value x = { TYPE_VOID, 0.0, 0, 1, NULL }; return x; }
static Value make_string(char *s) { Value x = { TYPE_STRING, 0.0, 0, 0, s }; return x; }

static char *value_to_string(Value v) {
    if (v.is_void || v.type == TYPE_VOID) return xstrdup("");
    if (v.type == TYPE_BOOL) return xstrdup(v.boolean ? "true" : "false");
    if (v.type == TYPE_STRING) return xstrdup(v.str ? v.str : "");
    long lv = (long)v.num;
    if ((double)lv == v.num) return xasprintf("%ld", lv);
    return xasprintf("%g", v.num);
}

static Frame *frame_new(Frame *parent) {
    Frame *f = (Frame *)calloc(1, sizeof(Frame));
    if (!f) {
        perror("calloc");
        exit(1);
    }
    f->parent = parent;
    return f;
}

static VarBinding *frame_lookup_local(Frame *f, const char *name) {
    for (VarBinding *v = f ? f->vars : NULL; v; v = v->next) {
        if (strcmp(v->name, name) == 0) return v;
    }
    return NULL;
}

static VarBinding *frame_lookup(Frame *f, const char *name) {
    for (Frame *cur = f; cur; cur = cur->parent) {
        VarBinding *v = frame_lookup_local(cur, name);
        if (v) return v;
    }
    return NULL;
}

static void frame_declare(Frame *f, const char *name, TypeKind type, Value v) {
    if (frame_lookup_local(f, name)) {
        fprintf(stderr, "runtime error: duplicate local variable %s\n", name);
        exit(1);
    }
    VarBinding *b = (VarBinding *)calloc(1, sizeof(VarBinding));
    if (!b) {
        perror("calloc");
        exit(1);
    }
    b->name = xstrdup(name);
    b->type = type;
    b->num = v.num;
    b->boolean = v.boolean;
    b->str = v.str;
    b->next = f->vars;
    f->vars = b;
}

static void frame_set(Frame *f, const char *name, Value v) {
    VarBinding *b = frame_lookup(f, name);
    if (!b) {
        fprintf(stderr, "runtime error: unknown variable %s\n", name);
        exit(1);
    }
    b->type = v.type;
    b->num = v.num;
    b->boolean = v.boolean;
    b->str = v.str;
}

static Value frame_get(Frame *f, const char *name) {
    VarBinding *b = frame_lookup(f, name);
    if (!b) {
        fprintf(stderr, "runtime error: unknown variable %s\n", name);
        exit(1);
    }
    if (b->type == TYPE_BOOL) return make_bool(b->boolean);
    if (b->type == TYPE_STRING) return make_string(b->str);
    return make_num(b->num);
}

static ASTNode *find_func(ASTNode *root, const char *name) {
    if (!root || root->kind != NODE_PROGRAM) return NULL;
    for (ASTList *cur = root->list; cur; cur = cur->next) {
        ASTNode *f = cur->node;
        if (f->kind == NODE_FUNC && strcmp(f->name, name) == 0) return f;
    }
    return NULL;
}

static Value eval_expr(ASTNode *n, ASTNode *root, Frame *frame, StrBuf *out);
static ExecResult exec_stmt(ASTNode *n, ASTNode *root, Frame *frame, StrBuf *out);

static Value runtime_read(void) {
    double x;
    if (scanf("%lf", &x) != 1) {
        fprintf(stderr, "runtime error: failed to read number\n");
        exit(1);
    }
    return make_num(x);
}

static void runtime_show(ASTList *args, ASTNode *root, Frame *frame, StrBuf *out) {
    for (ASTList *p = args; p; p = p->next) {
        Value v = eval_expr(p->node, root, frame, out);
        char *s = value_to_string(v);
        printf("%s ", s);
        free(s);
    }
    printf("\n");
    fflush(stdout);
}

static Value eval_call(ASTNode *n, ASTNode *root, Frame *frame, StrBuf *out) {
    if (strcmp(n->name, "ask") == 0) {
        printf("> ");
        fflush(stdout);
        return runtime_read();
    }

    ASTNode *func = find_func(root, n->name);
    if (!func) {
        fprintf(stderr, "runtime error: unknown function %s\n", n->name);
        exit(1);
    }

    Frame *call_frame = frame_new(frame);
    ASTList *p = func->list;
    ASTList *a = n->list;
    while (p && a) {
        Value arg = eval_expr(a->node, root, frame, out);
        frame_declare(call_frame, p->node->name, p->node->type, arg);
        p = p->next;
        a = a->next;
    }

    ExecResult r = exec_stmt(func->a, root, call_frame, out);
    if (r.returned) return r.value;
    return make_void();
}

static Value eval_expr(ASTNode *n, ASTNode *root, Frame *frame, StrBuf *out) {
    if (!n) return make_void();
    switch (n->kind) {
        case NODE_INT: return make_num((double)n->ival);
        case NODE_FLOAT: return make_num(n->fval);
        case NODE_BOOL: return make_bool(n->bval);
        case NODE_STRING: return make_string(n->str_val);
        case NODE_VAR: return frame_get(frame, n->name);
        case NODE_CALL: return eval_call(n, root, frame, out);
        case NODE_UNOP: {
            Value v = eval_expr(n->a, root, frame, out);
            if (!strcmp(n->op, "-")) return make_num(-v.num);
            if (!strcmp(n->op, "!")) return make_bool(!v.boolean);
            return make_void();
        }
        case NODE_BINOP: {
            Value l = eval_expr(n->a, root, frame, out);
            Value r = eval_expr(n->b, root, frame, out);
            if (!strcmp(n->op, "+")) return make_num(l.num + r.num);
            if (!strcmp(n->op, "-")) return make_num(l.num - r.num);
            if (!strcmp(n->op, "*")) return make_num(l.num * r.num);
            if (!strcmp(n->op, "/")) return make_num(l.num / r.num);
            if (!strcmp(n->op, "%")) return make_num((double)((long)l.num % (long)r.num));
            if (!strcmp(n->op, "<")) return make_bool(l.num < r.num);
            if (!strcmp(n->op, ">")) return make_bool(l.num > r.num);
            if (!strcmp(n->op, "<=")) return make_bool(l.num <= r.num);
            if (!strcmp(n->op, ">=")) return make_bool(l.num >= r.num);
            if (!strcmp(n->op, "==")) {
                if (l.type == TYPE_BOOL || r.type == TYPE_BOOL) return make_bool(l.boolean == r.boolean);
                return make_bool(fabs(l.num - r.num) < 1e-9);
            }
            if (!strcmp(n->op, "!=")) {
                if (l.type == TYPE_BOOL || r.type == TYPE_BOOL) return make_bool(l.boolean != r.boolean);
                return make_bool(fabs(l.num - r.num) >= 1e-9);
            }
            if (!strcmp(n->op, "&&")) return make_bool(l.boolean && r.boolean);
            if (!strcmp(n->op, "||")) return make_bool(l.boolean || r.boolean);
            return make_void();
        }
        default:
            return make_void();
    }
}

static ExecResult exec_stmt(ASTNode *n, ASTNode *root, Frame *frame, StrBuf *out) {
    ExecResult r = {0, make_void()};
    if (!n) return r;

    switch (n->kind) {
        case NODE_BLOCK: {
            Frame *inner = frame_new(frame);
            for (ASTList *cur = n->list; cur; cur = cur->next) {
                ExecResult x = exec_stmt(cur->node, root, inner, out);
                if (x.returned) return x;
            }
            return r;
        }
        case NODE_VAR_DECL: {
            Value v = make_void();
            if (n->a) v = eval_expr(n->a, root, frame, out);
            frame_declare(frame, n->name, n->type, v);
            return r;
        }
        case NODE_SET: {
            Value v = eval_expr(n->a, root, frame, out);
            frame_set(frame, n->name, v);
            return r;
        }
        case NODE_SHOW: {
            runtime_show(n->list, root, frame, out);
            return r;
        }
        case NODE_IF: {
            Value cond = eval_expr(n->a, root, frame, out);
            if (cond.boolean) return exec_stmt(n->b, root, frame, out);
            if (n->c) return exec_stmt(n->c, root, frame, out);
            return r;
        }
        case NODE_WHILE: {
            while (1) {
                Value cond = eval_expr(n->a, root, frame, out);
                if (!cond.boolean) break;
                ExecResult x = exec_stmt(n->b, root, frame, out);
                if (x.returned) return x;
            }
            return r;
        }
        case NODE_RETURN: {
            r.returned = 1;
            r.value = n->a ? eval_expr(n->a, root, frame, out) : make_void();
            return r;
        }
        case NODE_EXPR_STMT:
            (void)eval_expr(n->a, root, frame, out);
            return r;
        default:
            return r;
    }
}

char *runtime_execute(ASTNode *root) {
    StrBuf out;
    sb_init(&out);

    ASTNode *mainf = find_func(root, "main");
    if (!mainf) {
        fprintf(stderr, "runtime error: no main() function\n");
        exit(1);
    }

    Frame *global = frame_new(NULL);
    ExecResult r = exec_stmt(mainf->a, root, global, &out);
    (void)r;

    if (!out.buf) return xstrdup("");
    return out.buf;
}
