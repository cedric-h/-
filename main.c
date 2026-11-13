#include <stdio.h>
#include <assert.h>

typedef struct Expr Expr;
typedef enum {
    ExprKind_Variable,
    ExprKind_Abstraction,
    ExprKind_Application,
    ExprKind_Foreign,
} ExprKind;
struct Expr {
    ExprKind kind;
    char *debug;
    union {
        struct { Expr *bound_by; } variable;
        struct { Expr *binds, *body; } abstraction;
        struct { Expr *fn, *arg; } application;
        struct { int payload; } foreign;
    } k;
};

static Expr *expr_evaluate(Expr *expr) {
    switch (expr->kind) {
        case ExprKind_Foreign: {
            return expr;
        } break;

        case ExprKind_Variable: {
            Expr *bound_by =expr->k.variable.bound_by;
            assert(bound_by->kind == ExprKind_Abstraction);
            return bound_by->k.abstraction.binds;
        } break;

        case ExprKind_Abstraction: {
            return expr_evaluate(expr->k.abstraction.body);
        } break;

        case ExprKind_Application: {
            Expr *fn = expr->k.application.fn;
            assert(fn->kind == ExprKind_Abstraction);
            fn->k.abstraction.binds = expr->k.application.arg;
            return expr_evaluate(fn);
        } break;
    }
}

int main(int argc, char **argv) {

    // this runs (%x.x)(42)
#if 1
    {
        struct {
            Expr application, abstraction, variable, foreign;
        } ex = {0};

        ex.foreign.kind = ExprKind_Foreign;
        ex.foreign.debug = "foreign";
        ex.foreign.k.foreign.payload = 42;

        ex.application.kind = ExprKind_Application;
        ex.application.debug = "application";
        ex.application.k.application.fn = &ex.abstraction;
        ex.application.k.application.arg = &ex.foreign;

        ex.abstraction.kind = ExprKind_Abstraction;
        ex.abstraction.debug = "abstraction";
        ex.abstraction.k.abstraction.body = &ex.variable;

        ex.variable.kind = ExprKind_Variable;
        ex.variable.k.variable.bound_by = &ex.abstraction;
        ex.variable.debug = "one integer literal";

        Expr *ret = expr_evaluate(&ex.application);
        printf(
            "got back '%s', payload = %d\n",
            ret->debug,
            ret->k.foreign.payload
        );
    }
#endif

    // this runs %f%x.f x
#if 0
    {
        struct { Expr pass_f, pass_x, f, x, invoke_f; } ex = {0};

        ex.pass_f.kind = ExprKind_Application;
        ex.pass_f.debug = "pass_f";
        ex.pass_f.k.application.fn = &ex.pass_x;
        ex.pass_f.k.application.arg = &ex.pass_x;

        ex.abstraction.kind = ExprKind_Abstraction;
        ex.abstraction.debug = "abstraction";
        ex.abstraction.k.abstraction.body = &ex.variable;

        ex.variable.kind = ExprKind_Variable;
        ex.variable.debug = "one integer literal";

        Expr *ret = expr_evaluate(&ex.application);
        puts(ret->debug);
    }
#endif
}
