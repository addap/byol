#include <stdlib.h>

#include "lval.h"
#include "macros.h"
#include "parser.h"

lval *builtin_op(lenv *e, lval *rands, char *rator) {
    LASSERT(rands, (rands->count >= 1), 0, NULL, rator, "needs at least 1 argument");

    /* Check if all numbers */
    for (unsigned int i = 0; i < rands->count; i++) {
        TASSERT(rands, i, LVAL_NUM, 0, NULL, rator);
    }

    lval *x = lval_pop(rands, 0);

    /* Perform unary negation if necessary */
    if ((strcmp(rator, "-") == 0)
        && (rands->count == 0)) {
        x->num = -x->num;
    }

    while (rands->count > 0) {
        lval *y = lval_pop(rands, 0);

        if (strcmp(rator, "+") == 0) { x->num += y->num; }
        else if (strcmp(rator, "-") == 0) { x->num -= y->num; }
        else if (strcmp(rator, "*") == 0) { x->num *= y->num; }
        else if (strcmp(rator, "/") == 0) {
            if (y->num == 0) {
                lval_del(3, x, rands, y);
                return lval_err("Division by zero!");
            } else {
                x->num /= y->num;
            }
        }

        lval_del(1, y);
    }

    lval_del(1, rands);
    return x;
}

lval *builtin_add(lenv *e, lval *a) {
    return builtin_op(e, a, "+");
}

lval *builtin_sub(lenv *e, lval *s) {
    return builtin_op(e, s, "-");
}

lval *builtin_mul(lenv *e, lval *m) {
    return builtin_op(e, m, "*");
}

lval *builtin_div(lenv *e, lval *d) {
    return builtin_op(e, d, "/");
}

lval *builtin_head(lenv *e, lval *v) {
    CASSERT(v, 1, 0, NULL, "builtin_head");
    TASSERT(v, 0, LVAL_QEXPR, 0, NULL, "builtin_head");
    EASSERT(v, 0, NULL, "builtin_head");

    return lval_take(lval_take(v, 0), 0);
}

lval *builtin_tail(lenv *e, lval *v) {
    CASSERT(v, 1, 0, NULL, "builtin_tail");
    TASSERT(v, 0, LVAL_QEXPR, 0, NULL, "builtin_tail");
    EASSERT(v, 0, NULL, "builtin_tail");

    lval *a = lval_take(v, 0);
    lval_del(1, lval_pop(a, 0));

    return a;
}

lval *builtin_list(lenv *e, lval *v) {
    v->type = LVAL_QEXPR;
    return v;
}

lval *builtin_eval(lenv *e, lval *v) {
    CASSERT(v, 1, 0, NULL, "builtin_eval");
    TASSERT(v, 0, LVAL_QEXPR, 0, NULL, "builtin_eval");
    lval *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}

lval *builtin_print_env(lenv *e, lval *v) {
    lenv_print(e);
    lval_del(1, v);
    return lval_sexpr();
}

lval *builtin_join(lenv *e, lval *x) {
    for (int i = 0; i < x->count; i++) {
        TASSERT(x, i, LVAL_QEXPR, 0, NULL, "builtin_join");
    }

    lval *a = lval_pop(x, 0);

    while (x->count) {
        a = lval_join(a, lval_pop(x, 0));
    }

    lval_del(1, x);
    return a;
}

lval *builtin_cons(lenv *e, lval *v) {
    CASSERT(v, 2, 0, NULL, "builtin_cons");
    TASSERT(v, 1, LVAL_QEXPR, 0, NULL, "builtin_cons");

    lval *val = lval_pop(v, 0);
    lval *qexpr = lval_pop(v, 0);
    lval_del(1, v);

    return lval_prepend(qexpr, val);
}

lval *builtin_len(lenv *e, lval *v) {
    CASSERT(v, 1, 0, NULL, "builtin_len");
    TASSERT(v, 0, LVAL_QEXPR, 0, NULL, "builtin_len");

    lval *len = lval_num(v->cell[0]->count);
    lval_del(1, v);

    return len;
}

lval *builtin_init(lenv *e, lval *v) {
    CASSERT(v, 1, 0, NULL, "builtin_init");
    TASSERT(v, 0, LVAL_QEXPR, 0, NULL, "builtin_init");

    lval *a = lval_take(v, 0);
    lval_del(1, lval_pop(a, a->count - 1));
    return a;
}

/*
 * Binds a list of variables to a list of values one by one.
 * It takes a var_func that takes care in which environment the binding will occur
 */
lval *builtin_var(lenv *e, lval *a, void (*var_func)(lenv *, lval *, lval *)) {
    TASSERT(a, 0, LVAL_QEXPR, 0, NULL, "builtin_var");

    lval *symbols = lval_pop(a, 0);

    for (int i = 0; i < symbols->count; i++) {
        TASSERT(symbols, i, LVAL_SYM, 1, a, "builtin_var");
    }

    CASSERT(symbols, a->count, 1, a, "builtin_var");

    for (int i = 0; i < symbols->count; i++) {
        (*var_func)(e, symbols->cell[i], a->cell[i]);
    }

    lval_del(2, symbols, a);
    return lval_sexpr();
}

lval *builtin_def(lenv *e, lval *a) {
    return builtin_var(e, a, &lenv_def);
}

lval *builtin_put(lenv *e, lval *a) {
    return builtin_var(e, a, &lenv_put);
}

lval *builtin_exit(lenv *e, lval *a) {
    exit(0);
    return lval_sexpr();
}

lval *builtin_print(lenv *e, lval *a) {
    for (int i = 0; i < a->count; i++) {
        lval_print(a->cell[i]);
        putchar(' ');
    }

    putchar('\n');
    lval_del(1, a);

    return lval_sexpr();
}

lval *builtin_lambda(lenv *e, lval *a) {
    CASSERT(a, 2, 0, NULL, "\\");
    TASSERT(a, 0, LVAL_QEXPR, 0, NULL, "\\");
    TASSERT(a, 1, LVAL_QEXPR, 0, NULL, "\\");

    for (int i = 0; i < a->cell[0]->count; i++) {
        TASSERT(a->cell[0], i, LVAL_SYM, 0, NULL, "\\");
    }

    lval *formals = lval_pop(a, 0);
    lval *body = lval_pop(a, 0);
    lval_del(1, a);

    return lval_lambda(formals, body);
}

lval *builtin_ord(lenv *e, lval *a, char* op) {
    CASSERT(a, 2, 0, NULL, op);
    TASSERT(a, 0, LVAL_NUM, 0, NULL, op);
    TASSERT(a, 1, LVAL_NUM, 0, NULL, op);

    int r;
    if (strcmp(op, ">") == 0) {
        r = (a->cell[0]->num > a->cell[1]->num);
    } else if (strcmp(op, "<") == 0) {
        r = (a->cell[0]->num < a->cell[1]->num);
    } else if (strcmp(op, ">=") == 0) {
        r = (a->cell[0]->num >= a->cell[1]->num);
    } else if (strcmp(op, "<=") == 0) {
        r = (a->cell[0]->num <= a->cell[1]->num);
    } else {
        lval_del(1, a);
        return lval_err("Bad design. Op is %s", op);
    }
    lval_del(1, a);
    return lval_num(r);
}

lval *builtin_gt(lenv *e, lval *a) {
    return builtin_ord(e, a, ">");
}

lval *builtin_lt(lenv *e, lval *a) {
    return builtin_ord(e, a, "<");
}

lval *builtin_ge(lenv *e, lval *a) {
    return builtin_ord(e, a, "=>");
}

lval *builtin_le(lenv *e, lval *a) {
    return builtin_ord(e, a, "<=");
}

int lval_eq(lval *x, lval *y) {
    /* Different types are always unequal */
    if (x->type != y->type) {
        return 0;
    }

    /* Compare based on type */
    switch(x->type) {
        case LVAL_NUM: return (x->num == y->num);
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return (strcmp(x->str, y->str) == 0);
        /* If builtin, compare function pointers */
        case LVAL_BUILTIN: return (x->builtin == y->builtin);
        /* If lambda, compare parameters and body */
        case LVAL_LAMBDA: return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if (x->count != y->count) { return 0; }
            for (int i = 0; i < x->count; i++) {
                if (!lval_eq(x->cell[i], y->cell[i])) { return 0; }
            }
            return 1;
    }
    return 0;
}

lval *builtin_cmp(lenv *e, lval *a, char *op) {
    CASSERT(a, 2, 0, NULL, op);
    lval *x = lval_pop(a, 0);
    lval *y = lval_pop(a, 0);

    int r;
    if (strcmp(op, "==") == 0) {
        r = lval_eq(x, y);
    } else if (strcmp(op, "!=") == 0) {
        r = !lval_eq(x, y);
    } else {
        lval_del(3, a, x, y);
        return lval_err("Bad design. Op is %s", op);
    }

    lval_del(3, a, x, y);
    return lval_num(r);
}

lval *builtin_eq(lenv *e, lval *a) {
    return builtin_cmp(e, a, "==");
}

lval *builtin_neq(lenv *e, lval *a) {
    return builtin_cmp(e, a, "!=");
}

lval *builtin_if(lenv *e, lval *a) {
    CASSERT(a, 3, 0, NULL, "if");
    TASSERT(a, 0, LVAL_NUM, 0, NULL, "if");
    TASSERT(a, 1, LVAL_QEXPR, 0, NULL, "if");
    TASSERT(a, 2, LVAL_QEXPR, 0, NULL, "if");

    /* Turn qexprs into sexprs */
    a->cell[1]->type = LVAL_SEXPR;
    a->cell[2]->type = LVAL_SEXPR;

    lval *x;
    if (a->cell[0]->num) {
        x = lval_eval(e, lval_pop(a, 1));
    } else {
        x = lval_eval(e, lval_pop(a, 2));
    }

    lval_del(1, a);
    return x;
}

lval *builtin_load(lenv *e, lval *a, mpc_parser_t *lispy) {
    CASSERT(a, 1, 0, NULL, "load");
    TASSERT(a, 0, LVAL_STR, 0, NULL, "load");

    /* Parser file given by string name */
    mpc_result_t r;
    if (mpc_parse_contents(a->cell[0]->str, lispy, &r)) {
        lval *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        /* Evaluate each epression */
        while (expr->count) {
            lval *v = lval_pop(expr, 0);
            lval *x = lval_eval(e, v);
            /* If it's an error print it */
            if (x->type == LVAL_ERR) { lval_println(x); }
            lval_del(1, x);
        }

        lval_del(2, expr, a);
        return lval_sexpr();
    } else {
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        lval *err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(1, a);

        return err;
    }
}

