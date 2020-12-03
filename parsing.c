#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#endif

int main(int argc, char** argv) {
        mpc_parser_t
                *Number = mpc_new("number"),
                *Operator = mpc_new("operator"),
                *Expr = mpc_new("expr"),
                *Lispy = mpc_new("lispy");

        mpc_result_t r;

        mpca_lang(
                MPCA_LANG_DEFAULT,
                " \
                number: /-?[0-9]+/ ; \
                operator: '+' | '-' | '*' | '/' ; \
                expr: <number> | '(' <operator>  <expr>+ ')' ; \
                lispy: /^/ <operator> <expr>+ /$/ ; \
                ",
                Number,
                Operator,
                Expr,
                Lispy
        );


        puts("Lispy Version 0.0.0.0.1");
        puts("Press Ctrl+c to Exit\n");


        while (1) {
                char* input = readline("lispy> ");

                add_history(input);

                if (mpc_parse("<stdin>", input , Lispy, &r)) {
                        mpc_ast_print(r.output);
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(input);
        }

        mpc_cleanup(4, Number, Operator, Expr, Lispy);

        return 0;
}
