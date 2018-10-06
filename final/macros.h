
#ifndef CH12_MACROS_H
#define CH12_MACROS_H

#include "lval.h"

 /* Assert condition holds */
 //todo err is varargs but only two format specifiers in format string
#define LASSERT(args, cond, n, rest, err...)								\
	if (!(cond)) { lval_del(n+1, args, rest); return lval_err("%s: condition %s does not hold!", err); }   

/* Assert lval has cnt cells */
#define CASSERT(args, cnt, n, rest, name)							\
	if (args->count != cnt) { lval_del(n+1, args, rest); return lval_err("%s: lval has wrong number of members! Expected %u, got %u", name, cnt, args->count); }

/* Assert lval does no contain the empty expression */
#define EASSERT(args, n, rest, name)											\
	for (int i = 0; i < args->count; i++) {								\
	if (args->cell[i]->count == 0) { lval_del(n+1, args, rest); return lval_err("%s: lval contains empty sexpr at position %u!", name, i); } \
}

/* Assert lval is of type t */
#define TASSERT(args, i, t, n, rest, name)								\
	if (args->cell[i]->type != t) {										\
		lval* err = lval_err("%s: lval has wrong type! Expected %s, got %s at position %u", name, ltype_name(t), ltype_name(args->cell[i]->type), i); \
		lval_del(n+1, args, rest);										\
		return err; }

#endif