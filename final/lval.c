#include <stdlib.h>
#include <stdio.h>

#include "builtins.h"
#include "lval.h"
#include "macros.h"

char *ltype_name(int t) {
    switch (t) {
        case LVAL_NUM:
            return "Number";
        case LVAL_ERR:
            return "Error";
        case LVAL_SYM:
            return "Symbol";
        case LVAL_STR:
            return "String";
        case LVAL_SEXPR:
            return "S-expression";
        case LVAL_QEXPR:
            return "Q-expression";
        case LVAL_LAMBDA:
            return "Function";
        case LVAL_BUILTIN:
            return "Builtin function";
        default:
            return "Unknown type";
    }
}

/* Creates a new number lval*/
lval *lval_num(long x) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Creates a new error lval*/
lval *lval_err(char *fmt, ...) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_ERR;

    va_list va;
    va_start(va, fmt);
    v->err = malloc(512);
    vsnprintf(v->err, 511, fmt, va);
    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);
    return v;
}

lval *lval_sym(char *m) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(m) + 1);
    strcpy(v->sym, m);
    return v;
}

lval *lval_sexpr(void) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_qexpr(void) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval *lval_str(char* str) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_STR;
    v->str = malloc(strlen(str) + 1);
    strcpy(v->str, str);
    return v;
}

lval *lval_builtin(lbuiltin builtin) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_BUILTIN;
    v->builtin = builtin;
    return v;
}

/* if we want a declaration dependent environment, like in sml, we need to pass this here from builtin_lambda */
lval *lval_lambda(lval *formals, lval *body) {
    lval *v = calloc(1, sizeof(lval));
    v->type = LVAL_LAMBDA;
    v->formals = formals;
    v->body = body;
    v->env = lenv_new();
    return v;
}

void lval_del(int n, ...) {
    va_list list;
    va_start(list, n);

    for (int i = 0; i < n; i++) {
        lval *v = va_arg(list, lval*);

        switch (v->type) {
            /* Do nothing special for number and lbuiltin type*/
            case LVAL_NUM:
                break;
            case LVAL_BUILTIN:
                break;
            case LVAL_LAMBDA:
                lenv_del(v->env);
                lval_del(2, v->formals, v->body);
                break;
                /* For Err or Sym free the string data*/
            case LVAL_STR:
                free(v->str);
                break;
            case LVAL_ERR:
                free(v->err);
                break;
            case LVAL_SYM:
                free(v->sym);
                break;
                /* If Sexpr then delete all elements inside*/
            case LVAL_SEXPR:
            case LVAL_QEXPR:
                for (int j = 0; j < v->count; j++) {
                    lval_del(1, v->cell[j]);
                }
                /* Also free the memory allocated to contain the pointers*/
                free(v->cell);
                break;
        }
        /* Free the memory allocated for the "lval" struct itself*/
        free(v);
    }

    va_end(list);
}

lenv *lenv_new(void) {
    lenv *e = calloc(1, sizeof(lenv));
    e->count = 0;
    e->symbols = NULL;
    e->lvals = NULL;
    e->parent = NULL;
    return e;
}

void lenv_del(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        free(e->symbols[i]);
        lval_del(1, e->lvals[i]);
    }
    free(e->symbols);
    free(e->lvals);
    free(e);
}

lenv *lenv_copy(lenv *e) {
    lenv *env = lenv_new();
    env->count = e->count;
    env->symbols = malloc(env->count * sizeof(char *));
    env->lvals = malloc(env->count * sizeof(lval *));

    for (int i = 0; i < e->count; i++) {
        env->symbols[i] = malloc(strlen(e->symbols[i]) + 1);
        strcpy(env->symbols[i], e->symbols[i]);
        env->lvals[i] = lval_copy(e->lvals[i]);
    }

    return env;
}

void lval_print_expr(lval *v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
        lval_print(v->cell[i]);

        if (i != v->count - 1) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print_str(lval *v) {
    char *escaped = malloc(strlen(v->str) + 1);
    strcpy(escaped, v->str);
    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);
    free(escaped);
}

/* Print an "lval"*/
void lval_print(lval *v) {
    switch (v->type) {
        /* In the case the type is a number print it*/
        case LVAL_NUM:
            printf("%li", v->num);
            break;
        case LVAL_SYM:
            printf("%s", v->sym);
            break;
        case LVAL_STR:
            lval_print_str(v);
            break;
        case LVAL_ERR:
            printf("Error: %s", v->err);
            break;
        case LVAL_SEXPR:
            lval_print_expr(v, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_print_expr(v, '{', '}');
            break;
        case LVAL_BUILTIN:
            printf("<builtin function>");
            break;
        case LVAL_LAMBDA:
            printf("(\\ ");
            lval_print(v->formals);
            putchar(' ');
            lval_print(v->body);
            putchar(')');
            break;
    }
}

/* Print an "lval" followed by a newline*/
void lval_println(lval *v) {
    lval_print(v);
    putchar('\n');
}

/* Change to return null and set errno on error */
lval *lval_pop(lval *v, unsigned int i) {
    LASSERT(v, i < v->count, 0, NULL, "lval_pop", "index in range of v->count");

    lval *x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    return x;
}

lval *lval_take(lval *v, unsigned int i) {
    lval *x = lval_pop(v, i);
    lval_del(1, v);
    return x;
}

lval *lval_copy(lval *v) {

    lval *w = calloc(1, sizeof(lval));
    w->type = v->type;

    switch (v->type) {
        case LVAL_NUM:
            w->num = v->num;
            break;
        case LVAL_BUILTIN:
            w->builtin = v->builtin;
            break;
        case LVAL_LAMBDA:
            w->env = lenv_copy(v->env);
            w->formals = lval_copy(v->formals);
            w->body = lval_copy(v->body);
            break;
        case LVAL_SYM:
            w->sym = malloc(strlen(v->sym) + 1);
            strcpy(w->sym, v->sym);
            break;
        case LVAL_ERR:
            w->err = malloc(strlen(v->err) + 1);
            strcpy(w->err, v->err);
            break;
        case LVAL_STR:
            w->str = malloc(strlen(v->str) + 1);
            strcpy(w->str, v->str);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            w->count = v->count;
            w->cell = calloc(v->count, sizeof(lval *));
            for (int i = 0; i < v->count; i++) {
                w->cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return w;
}

lval *lenv_get(lenv *env, lval *s) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], s->sym) == 0) {
            return lval_copy(env->lvals[i]);
        }
    }

    if (env->parent) {
        return lenv_get(env->parent, s);
    } else {
        return lval_err("Unbound symbol! %s", s->sym);
    }
}

void lenv_put(lenv *env, lval *s, lval *v) {
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], s->sym) == 0) {
            lval_del(1, env->lvals[i]);
            env->lvals[i] = lval_copy(v);
            return;
        }
    }

    env->count++;
    env->symbols = realloc(env->symbols, sizeof(char *) * env->count);
    env->lvals = realloc(env->lvals, sizeof(lval *) * env->count);

    env->symbols[env->count - 1] = malloc(strlen(s->sym) + 1);
    strcpy(env->symbols[env->count - 1], s->sym);
    env->lvals[env->count - 1] = lval_copy(v);
}

void lenv_def(lenv *e, lval *k, lval *v) {
    while (e->parent) { e = e->parent; }
    lenv_put(e, k, v);
}

void lenv_print(lenv *e) {
    for (int i = 0; i < e->count; i++) {
        printf("Name: %s, Value: ", e->symbols[i]);
        lval_println(e->lvals[i]);
    }
}

/* Transform AST to lval   */
lval *lval_add(lval *v, lval *w) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    v->cell[v->count - 1] = w;
    return v;
}

lval *lval_prepend(lval *v, lval *w) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval *) * v->count);
    memmove(v->cell + 1, v->cell, sizeof(lval *) * (v->count - 1));
    v->cell[0] = w;
    return v;
}

lval *lval_join(lval *x, lval *y) {
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    return x;
}

lval *lval_call(lenv *e, lval *v) {
    /* Ensure the first element maps to a function in the environment */
    lval *f = lval_pop(v, 0);
    if ((f->type != LVAL_LAMBDA) && (f->type != LVAL_BUILTIN)) {
        lval_del(2, f, v);
        return lval_err("S-expression does not start with function");
    }

    /* Call function */
    lval *result;
    if (f->type == LVAL_LAMBDA) {
        unsigned int given = v->count;
        unsigned int total = f->formals->count;

        /* While there are still arguments to process */
        while(v->count) {
            /* If the current function already has all the arguments bound */
            if (f->formals->count == 0) {
                lval_del(1, v);
                return lval_err("Function passed too many arguments. Got %u, Expected %u.", given, total);
            }

            /* Get formal arg and value to bind to */
            lval *sym = lval_pop(f->formals, 0);
            if (strcmp(sym->sym, "&") == 0) {
                /* Ensure '&' is followed by exactly one symbol */
                //todo move that check to builtin_lambda so we can check that when we first create the lambda
                if (f->formals->count != 1) {
                    lval_del(2, f, v);
                    return lval_err("Function format invalid. Symbol '&' not followed by exactly one symbol");
                }

                /* Next formal parameter is bound to remaining arguments */
                lval *nsym = lval_pop(f->formals, 0);
                lenv_put(f->env, nsym, builtin_list(e, v));
                lval_del(2, sym, nsym);
                break;
            }

            lval *val = lval_pop(v, 0);

            lenv_put(f->env, sym, val);

            lval_del(2, sym, val);
        }

        lval_del(1, v);

        /* If '&' remains in formal list bind to empty list */
        if (f->formals->count > 0 &&
            strcmp(f->formals->cell[0]->sym, "&") == 0) {
            /* Check to ensure that & is not passed invalidly. */
            if (f->formals->count != 2) {
                return lval_err("Function format invalid. "
                                "Symbol '&' not followed by single symbol.");
            }
            /* Pop and delete '&' symbol */
            lval_del(1, lval_pop(f->formals, 0));
            /* Pop next symbol and create empty list */
            lval* sym = lval_pop(f->formals, 0);
            lval* val = lval_qexpr();
            /* Bind to environment and delete */
            lenv_put(f->env, sym, val);
            lval_del(2, sym, val);
        }


        /* If all formals have been bound we evaluate the function */
        if (f->formals->count == 0) {
            f->env->parent = e;
            result = builtin_eval(f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
        } else {
            result = lval_copy(f);
        }
    } else {
        result = f->builtin(e, v);
    }

    lval_del(1, f);
    return result;
}

lval *lval_eval_sexpr(lenv *e, lval *v) {

    /* First evaluate the children */
    for (unsigned int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(e, v->cell[i]);
    }

    /* Return the first error we find */
    for (unsigned int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* Empty expressions are just returned  */
    if (v->count == 0) { return v; }

    /* Otherwise we assume it's a function so we call it */
    return lval_call(e, v);
}

/* Eval lval */
lval *lval_eval(lenv *e, lval *v) {
    if (v->type == LVAL_SYM) {
        lval *x = lenv_get(e, v);
        lval_del(1, v);
        return x;
    }

    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }

    /* All other lvals stay the same */
    return v;
}

lval *lval_read_num(mpc_ast_t *t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval *lval_read_str(mpc_ast_t *t) {
    t->contents[strlen(t->contents)-1] = '\0';
    char *unescaped = malloc(strlen(t->contents+1)+1);
    strcpy(unescaped, t->contents+1);
    unescaped = mpcf_unescape(unescaped);
    lval *str = lval_str(unescaped);
    free(unescaped);
    return str;
}

/* todo: merge numbers and symbols, so that each symbol can have a value and function slot */
lval *lval_read(mpc_ast_t *t) {

    /* If number or symbol convert node to that type*/
    if (strstr(t->tag, "number")) { return lval_read_num(t); }
    if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }
    if (strstr(t->tag, "string")) { return lval_read_str(t); }

    lval *x = NULL;

    /* todo allow for several expressions at top level. We prob need a (begin ...) construct for that  */
    if (strcmp(t->tag, ">") == 0) {
        lval *sexpr = lval_sexpr();
        for (int i = 1; i < t->children_num - 1; i++) {
            if (strstr(t->children[i]->tag, "comment")) { continue; }
            lval_add(sexpr, lval_read(t->children[i]));
        }
        return sexpr;
    }
    /* if it's a sexpr create an empty list of sub lvals*/
    if (strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    else if (strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

    /* Fill the list with any subexpression*/
    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
        else if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
        else if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
        else if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
        else if (strstr(t->children[i]->tag, "comment")) { continue; }
        else if (strcmp(t->children[i]->tag, "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


void lenv_add_builtin(lenv *e, char *name, lbuiltin func) {
    lval *n = lval_sym(name);
    lval *f = lval_builtin(func);
    lenv_put(e, n, f);
    lval_del(2, n, f);
}

void lenv_add_builtins(lenv *e) {
    lenv_add_builtin(e, "+", builtin_add);
    lenv_add_builtin(e, "-", builtin_sub);
    lenv_add_builtin(e, "*", builtin_mul);
    lenv_add_builtin(e, "/", builtin_div);

    lenv_add_builtin(e, "head", builtin_head);
    lenv_add_builtin(e, "tail", builtin_tail);
    lenv_add_builtin(e, "join", builtin_join);
    lenv_add_builtin(e, "cons", builtin_cons);
    lenv_add_builtin(e, "len", builtin_len);
    lenv_add_builtin(e, "init", builtin_init);
    lenv_add_builtin(e, "eval", builtin_eval);
    lenv_add_builtin(e, "list", builtin_list);
    lenv_add_builtin(e, "def", builtin_def);
    lenv_add_builtin(e, "\\", builtin_lambda);
    lenv_add_builtin(e, "print-env", builtin_print_env);
    lenv_add_builtin(e, "exit", builtin_exit);
    lenv_add_builtin(e, "print", builtin_print);
    lenv_add_builtin(e, "=", builtin_put);
    lenv_add_builtin(e, "if", builtin_if);
    lenv_add_builtin(e, "load", builtin_load);

    lenv_add_builtin(e, "<", builtin_lt);
    lenv_add_builtin(e, ">", builtin_gt);
    lenv_add_builtin(e, "<=", builtin_le);
    lenv_add_builtin(e, "=>", builtin_ge);
    lenv_add_builtin(e, "==", builtin_eq);
    lenv_add_builtin(e, "!=", builtin_neq);
}

void lenv_load_lib(lenv *e, char *lib, mpc_parser_t *lispy) {
    lval *args = lval_add(lval_sexpr(), lval_str(lib));
    lval *x = builtin_load(e, args, lispy);

    if (x->type == LVAL_ERR) {
        lval_print(x);
    }

    lval_del(1, x);
}

void lenv_load_stdlib(lenv *e, mpc_parser_t *lispy) {
    lenv_load_lib(e, "../stdlib.txt", lispy);
}
