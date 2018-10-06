#include <stdio.h>
#include <assert.h>
#include <editline/readline.h>

#include "parser.h"
#include "lval.h"
#include "builtins.h"

int main (int argc, char** argv) {
    /* maybe get rid of expr and represent everything as sexpr*/
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* String = mpc_new("string");
    mpc_parser_t* Comment = mpc_new("comment");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language*/
    mpca_lang(MPCA_LANG_DEFAULT,
			  "                                                  \
              number: /-?[0-9]+/ ;								 \
              symbol: /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;		 \
              string: /\"(\\\\.|[^\"])*\"/ ;                     \
              comment: /;[^\\r\\n]*/ ;                           \
              sexpr: '(' <expr>* ')' ;                           \
              qexpr: '{' <expr>* '}' ;                           \
              expr: <number> | <symbol> | <sexpr>                \
                    | <qexpr> | <string> | <comment> ;           \
              lispy: /^/ <expr>* /$/ ;							 \
              ",
			  Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

	lenv* e = lenv_new();
	lenv_add_builtins(e);
	lenv_load_stdlib(e, Lispy);

	/* If we got some files to evaluate */
	if (argc > 2) {
	    for (int i = 1; i < argc; i++) {
	        lenv_load_lib(e, argv[i], Lispy);
	    }
	}
  
    puts ("Lispy version 0.8");
    puts ("Exit with Ctrl-c or Ctrl-d");

    while (1) {
    	char* input = readline ("lispy> ");
    	if (input == NULL) {
    	    break;
    	}
    
    	add_history (input);

    	mpc_result_t r;

    	/* Attempt to parser user input*/
    	if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval* in = lval_read (r.output);
			int in_count = in->count;
			for (int i = 0; i < in_count; i++) {
				lval* result = lval_eval (e, lval_pop(in, 0));
				lval_println (result);
				lval_del(1, result);
			}
    	    lval_del (1, in);
    	    mpc_ast_delete(r.output);
    	} else {
    	    mpc_err_print(r.error);
    	    mpc_err_delete(r.error);
    	}

    	free (input);
    }

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);
    putchar('\n');
    return 0;
}

   
