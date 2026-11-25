#include <stdio.h>
#include <assert.h>
#include <string.h>

typedef enum {
    ExprKind_Call,
    ExprKind_Fn,
    ExprKind_Var,
    ExprKind_ForeignVal,
    ExprKind_ForeignFn,
} ExprKind;
typedef struct Expr Expr;
struct Expr {
    ExprKind kind;
    char *debug;
    union {
        struct { Expr *arg, *fn; } call;
        /* fn.var filled at runtime */
        struct { Expr *body, *var; char symbol; } fn;
        struct { Expr *from_fn; } var;
        struct { int payload; } foreign_val;
        struct { int kind; } foreign_fn;
    } k;
};


static Expr *expr_eval(Expr *expr);
static Expr *expr_call(Expr *fn, Expr *arg) {
    fn = expr_eval(fn);
    arg = expr_eval(arg);

    if (fn->kind == ExprKind_ForeignFn) {
        assert(arg->kind == ExprKind_ForeignVal);
        arg->k.foreign_val.payload++;
        return arg;
    }

    if (fn->kind != ExprKind_Fn) {
        assert(0 && "can only evaluate functions");
        return NULL;
    }

    fn->k.fn.var = arg;

    Expr *body = fn->k.fn.body;

    return expr_eval(body);
}

static Expr *expr_eval(Expr *expr) {
    switch (expr->kind) {
        case ExprKind_Var: {
            Expr *fn = expr->k.var.from_fn;
            assert(fn->kind == ExprKind_Fn);
            return fn->k.fn.var;
        } break;

        case ExprKind_Call: {
            return expr_call(expr->k.call.fn, expr->k.call.arg);
        } break;

        case ExprKind_Fn:
        case ExprKind_ForeignVal:
        case ExprKind_ForeignFn: {
            return expr;
        } break;
    }
}

static void expr_fprint(FILE *f, Expr *expr) {
    switch (expr->kind) {
        case ExprKind_Var: {
            Expr *fn = expr->k.var.from_fn;
            fprintf(f, "%c", fn->k.fn.symbol);
        } break;

        case ExprKind_Call: {
            /* you can print prettier lambda programs
             * by moving these parens to be around the fn,
             * but I don't know how to parse that yet */
            fprintf(f, "(");
            expr_fprint(f, expr->k.call.fn);
            fprintf(f, ")");
            fprintf(f, "(");
            expr_fprint(f, expr->k.call.arg);
            fprintf(f, ")");
        } break;

        case ExprKind_Fn: {
            fprintf(f, "λ%c.", expr->k.fn.symbol);
            expr_fprint(f, expr->k.fn.body);
        } break;

        case ExprKind_ForeignVal: {
            fprintf(f, "%d", expr->k.foreign_val.payload);
        } break;

        case ExprKind_ForeignFn: {
            fprintf(f, "+1");
        } break;
    }
}

/* bump allocator */
typedef struct {
    Expr arena[200];
    int arena_i;

    Expr *fn_stack[26];
    int fn_stack_i;
} ExprScan;

Expr *expr_scan_alloc(ExprScan *a) {
    return a->arena + a->arena_i++;
}

static Expr *expr_fscan(ExprScan *a, FILE *f) {
    char c = 0;
    int peek = fgetc(f);

    if ((char)peek == "λ"[0]) {
        assert((char)fgetc(f) == "λ"[1]);
        assert(fscanf(f, "%c.", &c));
        Expr *e = expr_scan_alloc(a);
        e->kind = ExprKind_Fn;
        e->k.fn.symbol = c;

        a->fn_stack[a->fn_stack_i++] = e;
        e->k.fn.body = expr_fscan(a, f);
        a->fn_stack[--a->fn_stack_i] = NULL;

        return e;
    }

    if ((char)peek == '+') {
        assert((char)fgetc(f) == '1');
        Expr *e = expr_scan_alloc(a);
        e->kind = ExprKind_ForeignFn;
        return e;
    }

    if (peek == '(') {
        Expr *fn = expr_fscan(a, f);
        assert((char)fgetc(f) == ')');
        assert((char)fgetc(f) == '(');
        Expr *arg = expr_fscan(a, f);

        Expr *e = expr_scan_alloc(a);
        e->kind = ExprKind_Call;
        e->k.call.fn  = fn;
        e->k.call.arg = arg;

        assert((char)fgetc(f) == ')');

        return e;
    }

    int n = peek - '0';
    if (n >= 0 && n <= 9) {
        Expr *e = expr_scan_alloc(a);
        e->kind = ExprKind_ForeignVal;
        e->k.foreign_val.payload = n;
        return e;
    }

    /* assume variable */

    Expr *e = expr_scan_alloc(a);
    e->kind = ExprKind_Var;

    /* crawl up function stack until we find it */
    for (int i = a->fn_stack_i-1; i >= 0; i--) {
        if (a->fn_stack[i]->k.fn.symbol == peek) {
            e->k.var.from_fn = a->fn_stack[i];
            return e;
        }
    }

    assert(0 && "no such function");
}

int main(int arg_count, char *args[]) {
    {
        puts("input:");

        FILE *in = stdin;
        if (arg_count > 1) {
            in = fmemopen(args[1], strlen(args[1]), "r");
        }

        ExprScan arena = {0};
        Expr *root = expr_fscan(&arena, in);
        printf("parsed %d nodes: ", arena.arena_i);

        expr_fprint(stdout, root);
        printf("\n");

        printf("eval result: ");
        expr_fprint(stdout, expr_eval(root));
        printf("\n");
    }

#if 0
    {
        puts("running (λx.x)(42)");

        struct {
            Expr foreign_42, identity, x;
        } ex;

        ex.identity.kind = ExprKind_Fn;
        ex.identity.debug = "identity";
        ex.identity.k.fn.body = &ex.x;

        ex.x.kind = ExprKind_Var;
        ex.x.debug = "x";
        ex.x.k.var.from_fn = &ex.identity;

        ex.foreign_42.kind = ExprKind_ForeignVal;
        ex.foreign_42.debug = "foreign 42";
        ex.foreign_42.k.foreign_val.payload = 42;

        Expr *out = expr_eval(
            &ex.identity,
            &ex.foreign_42
        );
        printf(
            "got back '%s', payload = %d\n",
            out->debug,
            out->k.foreign_val.payload
        );
    }
#endif

#if 1
    {
        struct {
            Expr foreign_0, pass_0, pass_add, take_f, take_x, call_f, add, x, f;
        } ex = {0};

        ex.foreign_0.kind = ExprKind_ForeignVal;
        ex.foreign_0.debug = "foreign 0";
        ex.foreign_0.k.foreign_val.payload = 0;

        ex.pass_0.kind = ExprKind_Call;
        ex.pass_0.debug = "pass 0";
        ex.pass_0.k.call.arg = &ex.foreign_0;
        ex.pass_0.k.call.fn = &ex.pass_add;

        ex.add.kind = ExprKind_ForeignFn;
        ex.add.debug = "add";

        ex.pass_add.kind = ExprKind_Call;
        ex.pass_add.debug = "pass add";
        ex.pass_add.k.call.arg = &ex.add;
        ex.pass_add.k.call.fn = &ex.take_f;

        ex.take_f.kind = ExprKind_Fn;
        ex.take_f.debug = "take f";
        ex.take_f.k.fn.symbol = 'f';
        ex.take_f.k.fn.body = &ex.take_x;

        ex.take_x.kind = ExprKind_Fn;
        ex.take_x.debug = "take x";
        ex.take_x.k.fn.symbol = 'x';
        ex.take_x.k.fn.body = &ex.call_f;

        ex.call_f.kind = ExprKind_Call;
        ex.call_f.debug = "call f";
        ex.call_f.k.call.fn = &ex.f;
        ex.call_f.k.call.arg = &ex.x;

        ex.f.kind = ExprKind_Var;
        ex.f.debug = "f";
        ex.f.k.var.from_fn = &ex.take_f;

        ex.x.kind = ExprKind_Var;
        ex.x.debug = "x";
        ex.x.k.var.from_fn = &ex.take_x;

        puts("running (λfλx.f x)(λx.x+1)(0)");

        expr_fprint(stdout, &ex.pass_0);
        printf("\n");

        Expr *out = expr_eval(&ex.pass_0);
        printf(
            "got back '%s' payload = %d\n",
            out->debug,
            out->k.foreign_val.payload
        );

    }
#endif
}
