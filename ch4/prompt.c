#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <editline/readline.h>
// #include <histedit.h>

int main (int argc, char **argv) {
  puts ("Lispy version 0.2");
  puts ("Exit with Ctrl-c");

  while (1) {
    char *input = readline ("lispy> ");
    
    add_history (input);

    printf ("No you're a %s\n", input);

    free (input);
  }

  return 0;
}
   
