//
// Created by adrian on 29.09.18.
//

#ifndef CH12_BUILTINS_H
#define CH12_BUILTINS_H

#include "lval.h"

lval *builtin_op(lenv *e, lval *rands, char *rator);

lval *builtin_add(lenv *e, lval *a);

lval *builtin_sub(lenv *e, lval *s);

lval *builtin_mul(lenv *e, lval *m);

lval *builtin_div(lenv *e, lval *d);

lval *builtin_head(lenv *e, lval *v);

lval *builtin_tail(lenv *e, lval *v);

lval *builtin_list(lenv *e, lval *v);

lval *builtin_eval(lenv *e, lval *v);

lval *builtin_print_env(lenv *e, lval *v);

lval *builtin_join(lenv *e, lval *x);

lval *builtin_cons(lenv *e, lval *v);

lval *builtin_len(lenv *e, lval *v);

lval *builtin_init(lenv *e, lval *v);

lval *builtin_def(lenv *e, lval *a);

lval *builtin_put(lenv *e, lval *a);

lval *builtin_exit(lenv *e, lval *a);

lval *builtin_print(lenv *e, lval *a);

lval *builtin_lambda(lenv *e, lval *a);

lval *builtin_lt(lenv *e, lval *a);
lval *builtin_gt(lenv *e, lval *a);
lval *builtin_le(lenv *e, lval *a);
lval *builtin_ge(lenv *e, lval *a);
lval *builtin_eq(lenv *e, lval *a);
lval *builtin_neq(lenv *e, lval *a);

lval *builtin_if(lenv *e, lval *a);

lval *builtin_load(lenv *e, lval *a, mpc_parser_t *lispy);


#endif //CH12_BUILTINS_H
