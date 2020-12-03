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

long eval_op(long x, char * op, long y) {
        if (strcmp(op, "+") == 0) return x + y;
        if (strcmp(op, "-") == 0) return x - y;
        if (strcmp(op, "*") == 0) return x * y;
        if (strcmp(op, "/") == 0) return x / y;

        return 0;
}

long eval(mpc_ast_t * tag) {
        if (strstr(tag->tag, "number"))
                return atoi(tag->contents);

        char * op = tag->children[1]->contents;

        long x = eval(tag->children[2]);

        int i = 3;
        while (strstr(tag->children[i]->tag, "expr"))
        {
                x = eval_op(x, op, eval(tag->children[i]));
                i++;
        }

        return x;
}

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
                        // mpc_ast_print(r.output);
                        long result = eval(r.output);
                        printf("%ld\n", result);
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
