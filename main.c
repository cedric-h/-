#include <stdio.h>
#include <assert.h>

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
        struct { Expr *body, *var; } fn;
        struct { Expr *from_fn; } var;
        struct { int payload; } foreign_val;
        struct { int kind; } foreign_fn;
    } k;
};

static Expr *expr_resolve(Expr *expr) {
    switch (expr->kind) {
        case ExprKind_Var: {
            Expr *fn = expr->k.var.from_fn;
            assert(fn->kind == ExprKind_Fn);
            return fn->k.fn.var;
        } break;

        case ExprKind_Call:
        case ExprKind_Fn:
        case ExprKind_ForeignVal:
        case ExprKind_ForeignFn: {
            return expr;
        } break;
    }
}

static Expr *expr_eval(Expr *fn, Expr *arg) {
    fn = expr_resolve(fn);
    arg = expr_resolve(fn);

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

    switch (body->kind) {
        case ExprKind_ForeignVal:
        case ExprKind_ForeignFn:
        case ExprKind_Fn:
        case ExprKind_Var: {
            return expr_resolve(body);
        } break;

        case ExprKind_Call: {
            return expr_eval(
                body->k.call.fn,
                body->k.call.arg
            );
        };
    }
}

int main(int arg_count, char *args[]) {
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
        puts("running (λfλx.f x)(λx.x+1)(0)");

        struct {
            Expr pass_add, take_f, take_x, call_f, add, x, f;
        } ex = {0};

        ex.add.kind = ExprKind_ForeignFn;
        ex.add.debug = "add";

        ex.pass_add.kind = ExprKind_Call;
        ex.pass_add.debug = "pass add";
        ex.pass_add.k.call.arg = &ex.add;
        ex.pass_add.k.call.fn = &ex.take_f;

        ex.take_f.kind = ExprKind_Fn;
        ex.take_f.debug = "take f";
        ex.take_f.k.fn.body = &ex.take_x;

        ex.take_x.kind = ExprKind_Fn;
        ex.take_x.debug = "take x";
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

        Expr *out = expr_eval(&ex.take_f, &ex.add);
        out = expr_eval(
            out,
            &(Expr) {
                .kind = ExprKind_ForeignVal,
                .debug = "foreign 42",
                .k.foreign_val.payload = 42,
            }
        );
        printf(
            "got back '%s'",
            out->debug
        );

    }
#endif
}
