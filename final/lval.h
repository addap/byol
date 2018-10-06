#ifndef CH12_LVAL_H
#define CH12_LVAL_H

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* Declare enum for possible lval types*/
enum {
    LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_LAMBDA, LVAL_BUILTIN, LVAL_STR
};

char *ltype_name(int t);

typedef lval *(*lbuiltin)(lenv *, lval *);

/* Declare new Lisp Value struct*/
struct lval {
    unsigned int type;
    long num;
    /* error and symbol lvals have a string*/
    char *err;
    char *sym;
    char *str;
    /* function pointer */
    lbuiltin builtin;
    lenv *env;
    lval *formals;
    lval *body;

    /* double pointer to the children*/
    lval **cell;
    unsigned int count;
};

struct lenv {
    int count;
    lenv *parent;
    char **symbols;
    lval **lvals;
};

lval *lval_num(long x);

lval *lval_sym(char *m);

lval *lval_str(char *s);

lval *lval_sexpr(void);

lval *lval_qexpr(void);

lval *lval_err(char *fmt, ...);

lval *lval_builtin(lbuiltin builtin);

/* if we want a declaration dependent environment, like in sml, we need to pass this here from builtin_lambda */
lval *lval_lambda(lval *formals, lval *body);

void lenv_del(lenv *);

void lval_del(int n, ...);

lenv *lenv_new(void);

lval *lval_copy(lval *);

lenv *lenv_copy(lenv *e);

void lval_print(lval *v);

void lval_print_expr(lval *v, char open, char close);

/* Print an "lval" followed by a newline*/
void lval_println(lval *v);

lval *lval_read_num(mpc_ast_t *t);

/* Change to return null and set errno on error */
lval *lval_pop(lval *v, unsigned int i);

lval *lval_take(lval *v, unsigned int i);

lval *lenv_get(lenv *env, lval *s);

void lenv_put(lenv *env, lval *s, lval *v);

void lenv_def(lenv *e, lval *k, lval *v);

void lenv_print(lenv *e);

/* Transform AST to lval   */
lval *lval_add(lval *v, lval *w);

lval *lval_prepend(lval *v, lval *w);

lval *lval_join(lval *x, lval *y);

lval *lval_call(lenv *e, lval *v);

lval *lval_eval_sexpr(lenv *e, lval *v);

/* Eval lval */
lval *lval_eval(lenv *e, lval *v);

/* todo: merge numbers and symbols, so that each symbol can have a valu and function slot */
lval *lval_read(mpc_ast_t *t);

void lenv_add_builtin(lenv *e, char *name, lbuiltin func);

void lenv_add_builtins(lenv *e);

void lenv_load_stdlib(lenv *e, mpc_parser_t *lispy);

void lenv_load_lib(lenv *e, char *lib, mpc_parser_t *lispy);

#endif //CH12_LVAL_H