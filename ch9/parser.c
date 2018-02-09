#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <editline/readline.h>
#include "mpc.h"

/* Declare new Lisp Value struct*/
typedef struct lval {
  int type;
  long num;
  /* error and symbol lvals have a string*/
  char* err;
  char* sym;
  /* double pointer to the children*/
  struct lval** cell;
  int count;
} lval;

/* Declare enum for possible lval types*/
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
/* Decalre enum for possible error typer*/
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM, LERR_BAD_DEC, LERR_NEG_POWER, LERR_BAD_TYPE };

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

void lval_del(lval* v) {
  switch (v->type) {
    /* Do nothing special for number type*/
  case LVAL_NUM: break;
    /* For Err or Sym free the string data*/
  case LVAL_ERR: free(v->err); break;
  case LVAL_SYM: free(v->sym); break;
    /* If Sexpr then delete all elements inside*/
  case LVAL_SEXPR:
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
  case LVAL_SEXPR: lval_print_expr (v, '(', ')');
  }
}

/* Print an "lval" followed by a newline*/
void lval_println(lval* v) { lval_print(v); putchar('\n'); }

long max (long a, long b) {
  if (a > b) { return a; }
  else { return b; }
}

long min (long a, long b) {
  if (a < b) { return a; }
  else { return b; }
}

long power (long a, long b) {
  if (b <= 0) { return 1; }
  else { return a* power (a, b - 1); }
}

lval* lval_read_num (mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ? lval_num(x) : lval_err("invalid number"); 
}

lval* lval_pop(lval* v, int i) {
  /* Find the item at "i" */
  lval* x = v->cell[i];
  /* Shift memory after the item at "i" over the top */memmove(&v->cell[i], &v->cell[i+1],
							       sizeof(lval*) * (v->count-i-1));
  /* Decrease the count of items in the list */
  v->count--;
  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}

lval* builtin_op(lval* a, char* op) {
  /* Ensure all arguments are numbers */
  for (int i = 0; i < a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-number!");
    }
  }
  /* Pop the first element */
  lval* x = lval_pop(a, 0);
  /* If no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->num = -x->num;
  }
  /* While there are still elements remaining */
  while (a->count > 0) {/* Pop the next element */
    lval* y = lval_pop(a, 0);
    /* Perform operation */
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if (y->num == 0) {
	lval_del(x); lval_del(y);
	x = lval_err("Division By Zero.");
	break;
      }
      x->num /= y->num;
    }
    /* Delete element now finished with */
    lval_del(y);
  }
  /* Delete input expression and return result */
  lval_del(a);
  return x;
}
lval* lval_eval(lval* v);
lval* lval_eval_sexpr(lval* v) {
  /* Evaluate Children */
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }
  /* Error Checking */
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }
  /* Empty Expression */
  if (v->count == 0) { return v; }
  /* Single Expression */
  if (v->count == 1) { return lval_take(v, 0); }
  /* Ensure First Element is Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S-expression Does not start with symbol.");
  }
  /* Call builtin with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;
}
lval* lval_eval(lval* v) {
  /* Evaluate Sexpressions */
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
  /* All other lval types remain the same */
  return v;
}

/* Transform AST to lval   */
lval* lval_add (lval* v, lval* w) {
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = w;
  return v;
}

lval* lval_read (mpc_ast_t* t) {

  /* If number or symbol convert node to that type*/
  if (strstr(t->tag, "number")) { return lval_read_num (t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  lval* x = NULL;
  /* todo allow for several expressions at top level. We prob need a (begin ...) construct for that  */
  if (strcmp(t->tag, ">") == 0) { return lval_read (t->children[1]); }
  /* if it's a sexpr create an empty list of sub lvals*/
  if (strstr(t->tag, "sexpr")) { x = lval_sexpr (); }
  
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

int main (int argc, char**argv) {
  /* maybe get rid of expr and represent everything as sexpr*/
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* Define them with the following Language*/
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                    \
              number: /-?[0-9]+/ ;                               \
              symbol: '+' | '-' | '*' | '/' | '%' | '^' ;        \
              sexpr: '(' <expr>* ')' ;                           \
              expr: <number> | <symbol> | <sexpr> ;              \
              lispy: /^/ <expr> /$/ ;                          \
            ",
	    Number, Symbol, Sexpr, Expr, Lispy); 
  
  puts ("Lispy version 0.4");
  puts ("Exit with Ctrl-c or Ctrl-d");

  while (1) {
    char* input = readline ("lispy> ");
    /* if (input == NULL) { */
    /*   return 0; */
    /* } */
    
    add_history (input);

    mpc_result_t r;

    /* Attempt to parser user input*/
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      mpc_ast_print (r.output);

      lval* res_read = lval_read (r.output);
      lval_println (res_read);
      
      lval* res_eval = lval_eval (res_read);
      lval_println (res_eval);

      lval_del (res_read);
      lval_del (res_eval);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free (input);
  }

  /* Undefine and Delete our Parsers*/
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

  return 0;
}
   
