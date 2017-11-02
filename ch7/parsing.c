#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>
#include "mpc.h"

long int eval_op (long x, char * op, long y) {
  if (strcmp (op, "+") == 0) { return x + y; }
  if (strcmp (op, "-") == 0) { return x - y; }
  if (strcmp (op, "/") == 0) { return x / y; }
  if (strcmp (op, "*") == 0) { return x * y; }
  return 0;
}

long eval (mpc_ast_t * t) {

  /* If tagged as a number, just return it */
  if (strstr (t->tag, "number")) {
    return atoi (t->contents);
  }

  /* The operator is the second child */
  char * op = t->children[1]->contents;

  /* Store the first expression in x */
  long x = eval (t->children[2]);

  /* Iterate over the rest of the expressions, akkumuting the result */
  int i = 3;
  while (strstr (t->children[i]->tag, "expr")) {
    x = eval_op (x, op, eval (t->children[i++]));
  }

  return x;
}

int main (int argc, char **argv) {
  mpc_parser_t * Number = mpc_new("number");
  mpc_parser_t * Operator = mpc_new("operator");
  mpc_parser_t * Expr = mpc_new("expr");
  mpc_parser_t * Lispy = mpc_new("lispy");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                    \
              number: /-?[0-9]+/ /[.][0-9]+/? ;                  \
              operator : '+' | '-' | '*' | '/' | '%'             \
                         | \"add\" | \"sub\" | \"mul\"           \
                         | \"div\" | \"mod\" ;			 \
              expr: <number> | '(' <operator> <expr>+ ')' ;      \
              lispy: /^/ <operator> <expr>+ /$/ ;                \
            ",
	    Number, Operator, Expr, Lispy); 
  
  puts ("Lispy version 0.4");
  puts ("Exit with Ctrl-c");

  while (1) {
    char *input = readline ("lispy> ");
    
    add_history (input);

    mpc_result_t r;

    /* Attempt to parser user input */
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* On sucess print AST */
      long result = eval (r.output);
      printf ("%li\n", result);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free (input);
  }

  /* Undefine and Delete our Parsers */
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

  return 0;
}
   
