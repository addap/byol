#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>
#include "mpc.h"

/*
  An Adjective is either "wow", "many", "so" or "such".
› A Noun is either "lisp", "language", "c", "book" or "build".
› A Phrase is an Adjective followed by a Noun.
› A Doge is zero or more Phrases.
We can start by trying to define Adject
*/

int main (int argc, char **argv) {
  mpc_parser_t * Adjective = mpc_new("adjective");
  mpc_parser_t * Noun = mpc_new("noun");
  mpc_parser_t * Phrase = mpc_new("phrase");
  mpc_parser_t * Doge = mpc_new("doge");

  /* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
	    "                                                                \
              adjective: \"wow\" | \"many\" | \"so\" | \"such\" ;            \
              noun: \"lisp\" | \"language\" | \"c\" | \"book\" | \"build\" ; \
              phrase: <adjective> <noun> ;                                   \
              doge: /^/ <phrase>* /$/ ;                                              \
            ",
	    Adjective, Noun, Phrase, Doge); 
  
  puts ("Wow such version 0.0");
  puts ("Exit with Ctrl-c");

  while (1) {
    char *input = readline ("wow> ");
    
    add_history (input);

    mpc_result_t r;

    /* Attempt to parser user input */
    if (mpc_parse("<stdin>", input, Doge, &r)) {
      /* On sucess print AST */
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free (input);
  }

  /* Undefine and Delete our Parsers */
    mpc_cleanup(4, Adjective, Noun, Phrase, Doge); 
  return 0;
}
   
