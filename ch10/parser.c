#include <stdio.h>
#include <assert.h>

#include <editline/readline.h>
#include "mpc.h"

#define LASSERT(args, cond, err)							\
	if (!(cond)) { lval_del(args); return lval_err(err); }

#define CASSERT(args, cnt, err)											\
	if (args->count != cnt) { lval_del(args); return lval_err(err); }

#define EASSERT(args, err)												\
	for (int i = 0; i < args->count; i++) {								\
		if (args->cell[i]->count == 0) { lval_del(args); return lval_err(err); } \
	}

#define TASSERT(args, n, t, err)				\
	if (args->cell[n]->type != t) { lval_del(args); return lval_err(err); }


/* Declare new Lisp Value struct*/
typedef struct lval {
    unsigned int type;
    long num;
    /* error and symbol lvals have a string*/
    char* err;
    char* sym;
    /* double pointer to the children*/
    struct lval** cell;
    unsigned int count;
} lval;

/* Declare enum for possible lval types*/
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

/* Creates a new number lval*/
lval* lval_num (long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* Creates a new error lval*/
lval* lval_err (char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

lval* lval_sym (char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(m) + 1);
    strcpy(v->sym, m);
    return v;
}

lval* lval_sexpr (void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr (void) {
    lval* v = malloc (sizeof (lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {
    switch (v->type) {
		/* Do nothing special for number type*/
    case LVAL_NUM: break;
		/* For Err or Sym free the string data*/
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;
		/* If Sexpr then delete all elements inside*/
    case LVAL_SEXPR:
    case LVAL_QEXPR:
		for (int i = 0; i < v->count; i++) {
			lval_del(v->cell[i]);
		}
		/* Also free the memory allocated to contain the pointers*/
		free(v->cell);
		break;
    }
    /* Free the memory allocated for the "lval" struct itself*/
    free(v);
}

void lval_print (lval* v);

void lval_print_expr (lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {
		lval_print(v->cell[i]);

		if (i != v->count-1) {
			putchar(' ');
		}
    }
    putchar(close);
} 

/* Print an "lval"*/
void lval_print(lval* v) {
    switch (v->type) {
		/* In the case the type is a number print it*/
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_SYM: printf("%s", v->sym); break;
    case LVAL_ERR: printf("Error: %s", v->err); break;
    case LVAL_SEXPR: lval_print_expr (v, '(', ')'); break;
    case LVAL_QEXPR: lval_print_expr (v, '{', '}');
    }
}

/* Print an "lval" followed by a newline*/
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* lval_read_num (mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number"); 
}

/* Change to return null and set errno on error */
lval* lval_pop (lval* v, unsigned int i) {
    LASSERT(v, i < v->count, "POP: index to large")
    
    lval* x = v->cell[i];
    memmove (&v->cell[i], &v->cell[i+1], sizeof (lval*) * (v->count - i - 1));
    v->count--;
    v->cell = realloc (v->cell, sizeof (lval*) * v->count);
    return x;
}

lval* lval_take (lval* v, unsigned int i) {
    lval* x = lval_pop (v, i);
    lval_del (v);
    return x;
}

lval* builtin_op (lval* rands, char* rator) {
    /* Check if all numbers */
    for (unsigned int i = 0; i < rands->count; i++) {
		if (rands->cell[i]->type != LVAL_NUM) {
			lval_del (rands);
			return lval_err ("Cannot operate on non-numbers.");
		}
    }

    lval* x = lval_pop (rands, 0);

    /* Perform unary negation if necessary */
    if ((strcmp (rator, "-") == 0)
		&& (rands->count == 0)) {
		x->num = - x->num;
    }

    while (rands->count > 0) {
		lval* y = lval_pop (rands, 0);

		if (strcmp (rator, "+") == 0) { x->num += y->num; }
		else if (strcmp (rator, "-") == 0) { x->num -= y->num; }
		else if (strcmp (rator, "*") == 0) { x->num *= y->num; }
		else if (strcmp (rator, "/") == 0) {
			if (y->num == 0) {
				lval_del (x);
				lval_del (rands);
				lval_del (y);
				return lval_err ("Division by zero!");
			} else {
				x->num /= y->num;
			}
		}

		lval_del (y);
    }

    lval_del (rands);
    return x;
}

lval* builtin_head (lval* v) {
	CASSERT(v, 1, "Head passed wrong number of arguments!");
	TASSERT(v, 0, LVAL_QEXPR, "Head passed incorrect type!");
	EASSERT(v, "Head passed nil!");
  
	return lval_take (lval_take (v, 0), 0);
}

lval* builtin_tail (lval* v) {
	CASSERT(v, 1, "Tail passed wrong number of arguments!");
	TASSERT(v, 0, LVAL_QEXPR, "Tail passed incorrect type!");
	EASSERT(v, "Tail passed nil!");

    lval* a = lval_take (v, 0);
    lval_del (lval_pop (a, 0));

    return a;
}

lval* builtin_list (lval* v) {
	v->type = LVAL_QEXPR;
	return v;
}

lval* lval_eval (lval* v);

lval* builtin_eval (lval* v) {
	CASSERT(v, 1, "Eval passed wrong number of arguments!");
	TASSERT(v, 0, LVAL_QEXPR, "Eval passed incorrect type!");
	lval* a = lval_take(v, 0);
	a->type = LVAL_SEXPR;
	return lval_eval(a);
}

/* Transform AST to lval   */
lval* lval_add (lval* v, lval* w) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = w;
    return v;
}

lval* lval_prepend(lval* v, lval* w) {
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	memmove(v->cell + 1, v->cell, sizeof(lval*) * (v->count - 1));
	v->cell[0] = w;
	return v;
}

lval* lval_join (lval* x, lval* y) {
	while (y->count) {
		x = lval_add(x, lval_pop(y, 0));
	}

	return x;
}

lval* builtin_join (lval* x) {
	for (int i = 0; i < x->count; i++) {
		TASSERT(x, i, LVAL_QEXPR, "Join passed non-QEXPR!");
	}

	lval* a = lval_pop(x, 0);

	while (x->count) {
		a = lval_join(a, lval_pop(x, 0));
	}
	
	lval_del(x);
	return a;
}

lval* builtin_cons (lval* v) {
	CASSERT(v, 2, "Cons passed wrong number of arguments!");
	TASSERT(v, 1, LVAL_QEXPR, "Cons passed non-QEXPR!");

	lval* val = lval_pop(v, 0);
	lval* qexpr = lval_pop(v, 0);
	lval_del(v);

	return lval_prepend(qexpr, val);
}

lval* builtin_len (lval* v) {
	CASSERT(v, 1, "Len passed wrong number of arguments!");
	TASSERT(v, 0, LVAL_QEXPR, "Len passed non-QEXPR!");

	return lval_num(v->cell[0]->count);
}

lval* builtin_init (lval* v) {
	CASSERT(v, 1, "Init passed wrong number of arguments!");
	TASSERT(v, 0, LVAL_QEXPR, "Init passed non-QEXPR!");

	lval* a = lval_take(v, 0);
	lval_del(lval_pop(a, a->count - 1));
	return a;
}		

lval* builtin (lval* x, char* func) {
	if (strcmp("head", func) == 0) { return builtin_head(x); }
	else if (strcmp("tail", func) == 0) { return builtin_tail(x); }
	else if (strcmp("list", func) == 0) { return builtin_list(x); }
	else if (strcmp("eval", func) == 0) { return builtin_eval(x); }
	else if (strcmp("join", func) == 0) { return builtin_join(x); }
	else if (strcmp("cons", func) == 0) { return builtin_cons(x); }
	else if (strcmp("len", func) == 0) { return builtin_len(x); }
	else if (strcmp("init", func) == 0) { return builtin_init(x); }
	else if (strstr("+/*-", func)) { return builtin_op(x, func); }
	else {
		lval_del(x);
		return lval_err("Unknown builtin operation!");
	}
}

lval* lval_eval_sexpr (lval* v) {

    /* First evaluate the children */
    for (unsigned int i = 0; i < v->count; i++) {
		v->cell[i] = lval_eval (v->cell[i]);
    }

    /* Return the first error we find */
    for (unsigned int i = 0; i < v->count; i++) {
		if (v->cell[i]->type == LVAL_ERR) { return lval_take (v, i); }
    }

    /* Empty expressions are just returned  */
    if (v->count == 0) { return v; }

    /* todo: Why would you just return single expressions. they should be evaluated */
    if (v->count == 1) { return lval_take (v, 0); }

    /* Ensure first element is symbol */
    /* todo numbers should be symbol and every symbol should have a function and a value slot */
    lval* f = lval_pop (v, 0);
    if (f->type != LVAL_SYM) {
		lval_del (f);
		lval_del (v);
		return lval_err ("S-expression does not start with symbol");
    }

    /* Call builtin_op with operator */
    lval* result = builtin (v, f->sym);
    lval_del (f);
    return result;
}

/* Eval lval */
lval* lval_eval (lval* v) {
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr (v); }

    /* All other lvals stay the same */
    return v;
}

lval* lval_read (mpc_ast_t* t) {

    /* If number or symbol convert node to that type*/
    if (strstr (t->tag, "number")) { return lval_read_num (t); }
    if (strstr (t->tag, "symbol")) { return lval_sym(t->contents); }

    lval* x = NULL;

    /* todo allow for several expressions at top level. We prob need a (begin ...) construct for that  */
    if (strcmp (t->tag, ">") == 0) { return lval_read (t->children[1]); }
    /* if it's a sexpr create an empty list of sub lvals*/
    if (strstr (t->tag, "sexpr")) { x = lval_sexpr (); }
    else if (strstr (t->tag, "qexpr")) { x = lval_qexpr (); }
  
    /* Fill the list with any subexpression*/
    for (int i = 0; i < t->children_num; i++) {
		if (strcmp (t->children[i]->contents, "(") == 0) { continue; }
		else if (strcmp (t->children[i]->contents, ")") == 0) { continue; }
		else if (strcmp (t->children[i]->contents, "{") == 0) { continue; }
		else if (strcmp (t->children[i]->contents, "}") == 0) { continue; }
		else if (strcmp (t->children[i]->tag, "regex") == 0) { continue; }
		x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}


int main (int argc, char** argv) {
    /* maybe get rid of expr and represent everything as sexpr*/
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language*/
    mpca_lang(MPCA_LANG_DEFAULT,
			  "                                                  \
              number: /-?[0-9]+/ ;								 \
              symbol: \"list\" | \"head\" | \"tail\" | \"join\"  \
	              | \"eval\" | \"cons\" | \"len\" | \"init\"     \
                  | '+' | '-' | '*' | '/' | '%' | '^' ;			 \
              sexpr: '(' <expr>* ')' ;                           \
              qexpr: '{' <expr>* '}' ;                           \
              expr: <number> | <symbol> | <sexpr> | <qexpr> ;    \
              lispy: /^/ <expr> /$/ ;                            \
            ",
			  Number, Symbol, Sexpr, Qexpr, Expr, Lispy); 
  
    puts ("Lispy version 0.4");
    puts ("Exit with Ctrl-c or Ctrl-d");

    while (1) {
    	char* input = readline ("lispy> ");
    	if (input == NULL) {
    	    mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
			return 1;
    	}
    
    	add_history (input);

    	mpc_result_t r;

    	/* Attempt to parser user input*/
    	if (mpc_parse("<stdin>", input, Lispy, &r)) {
    	    lval* res_read = lval_read (r.output);
    	    lval* res_eval = lval_eval (res_read);
    	    lval_println (res_eval);

    	    lval_del (res_eval);
	    
    	    mpc_ast_delete(r.output);
    	} else {
    	    mpc_err_print(r.error);
    	    mpc_err_delete(r.error);
    	}

    	free (input);
    }

    mpc_cleanup(5, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
    return 0;
}

   
