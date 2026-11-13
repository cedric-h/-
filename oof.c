

typedef struct Expr Expr;
typedef enum {
    ExprKind_Variable,
    ExprKind_Abstraction,
    ExprKind_ForeignAbstraction,
    ExprKind_Application,
    ExprKind_Foreign,
} ExprKind;
struct Expr {
    ExprKind kind;
    char *debug;
    union {
        struct { Expr *bound_by; } variable;
        struct { Expr *binds, *body; } abstraction;
        struct { Expr *binds; } foreign_abstraction;
        struct { Expr *fn, *arg; } application;
        struct { int payload; } foreign;
    } k;
};

static Expr *expr_evaluate(Expr *expr) {
    switch (expr->kind) {
        case ExprKind_Foreign: {
            return expr;
        } break;

        case ExprKind_ForeignAbstraction: {
            Expr *binds = expr->k.foreign_abstraction.binds;
            binds = expr_evaluate(binds);
            assert(binds->kind == ExprKind_Foreign);
            binds->k.foreign.payload += 1;
            return binds;
        } break;

        case ExprKind_Variable: {
            Expr *bound_by = expr->k.variable.bound_by;
            assert(bound_by->kind == ExprKind_Abstraction);
            return bound_by->k.abstraction.binds;
        } break;

        case ExprKind_Abstraction: {
            return expr->k.abstraction.body;
        } break;

        case ExprKind_Application: {
            Expr *fn = expr->k.application.fn;
            if (
                fn->kind != ExprKind_Abstraction &&
                fn->kind != ExprKind_ForeignAbstraction
            ) {
                fn = expr_evaluate(fn);
                assert(
                    fn->kind == ExprKind_Abstraction ||
                    fn->kind == ExprKind_ForeignAbstraction &&
                    "can only apply to a function or something "
                        "which evaluates to a function"
                );
            }

            fn->k.abstraction.binds = expr->k.application.arg;
            return expr_evaluate(fn);
        } break;
    }
}

int main(int argc, char **argv) {

#if 0
    {
        puts("running (λx.x)(42)");

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
            /* it gave us back a variable, so to
             * get a real payload we need to evaluate it */
            ret->k.foreign.payload
        );
    }
#endif

    // this runs (%f%x.f x)(%x.x+1)(0)
#if 1
    {
        puts("running (λfλx.f x)(λx.x+1)(0)");
        struct {
            Expr take_f, take_x, f, x, invoke_f;
            Expr add, zero, pass_add, pass_zero, start;
        } ex = {0};

        /* λfλx.f x */
        {
            ex.take_f.kind = ExprKind_Abstraction;
            ex.take_f.debug = "take f";
            ex.take_f.k.abstraction.body = &ex.take_x;

            ex.f.kind = ExprKind_Variable;
            ex.f.k.variable.bound_by = &ex.take_f;
            ex.f.debug = "f variable";

            ex.take_x.kind = ExprKind_Abstraction;
            ex.take_x.debug = "take x";
            ex.take_x.k.abstraction.body = &ex.invoke_f;

            ex.x.kind = ExprKind_Variable;
            ex.x.k.variable.bound_by = &ex.take_x;
            ex.x.debug = "x variable";

            ex.invoke_f.kind = ExprKind_Application;
            ex.invoke_f.debug = "invoke_f";
            ex.invoke_f.k.application.fn = &ex.f;
            ex.invoke_f.k.application.arg = &ex.x;
        }

        {
            ex.add.kind = ExprKind_ForeignAbstraction;
            ex.add.debug = "add";

            ex.zero.kind = ExprKind_Foreign;
            ex.zero.debug = "zero";
            ex.zero.k.foreign.payload = 0;

            ex.pass_add.kind = ExprKind_Application;
            ex.pass_add.debug = "pass add";
            ex.pass_add.k.application.fn = &ex.take_f;
            ex.pass_add.k.application.arg = &ex.add;

            ex.pass_zero.kind = ExprKind_Application;
            ex.pass_zero.debug = "pass zero";
            ex.pass_zero.k.application.fn = &ex.pass_add;
            ex.pass_zero.k.application.arg = &ex.zero;
        }

        Expr *ret = expr_evaluate(&ex.pass_zero);
        printf(
            "got back '%s', payload = %d\n",
            ret->debug,
            ret->k.foreign.payload
        );
    }
#endif
}
