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

enum {
        LVAL_NUM,
        LVAL_ERR
};

enum {
        LERR_DIV_ZERO,
        LERR_BAD_OP,
        LERR_BAD_NUM
};

typedef struct {
        int type;
        long num;
        int err;
} lval;

lval lval_num(int x) {
        lval v;
        v.type = LVAL_NUM;
        v.num = x;
        return v;
}

lval lval_err(int x) {
        lval v;
        v.type = LVAL_ERR;
        v.err = x;
        return v;
}

void lval_print(lval v) {
        switch(v.type) {
                case LVAL_NUM: printf("%li", v.num); break;
                case LVAL_ERR:
                        switch(v.err) {
                                case LERR_DIV_ZERO: printf("Error: Division By Zero!"); break;
                                case LERR_BAD_OP: printf("Error: Invalid Operator!"); break;
                                case LERR_BAD_NUM: printf("Error: Invalid Number!"); break;
                        };
                        break;
        };
}

void lval_println(lval v) {
        lval_print(v);
        putchar('\n');
}

lval eval_op(lval x, char * op, lval y) {
        if (x.type == LVAL_ERR) return x;
        if (y.type == LVAL_ERR) return y;

        if (strcmp(op, "+") == 0) return lval_num(x.num + y.num);
        if (strcmp(op, "-") == 0) return lval_num(x.num - y.num);
        if (strcmp(op, "*") == 0) return lval_num(x.num + y.num);
        if (strcmp(op, "/") == 0) return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
        if (strcmp(op, "\%") == 0) return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);

        return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t * tag) {
        if (strstr(tag->tag, "number")) {
                errno = 0;
                long x = strtol(tag->contents, NULL, 10);
                return errno == ERANGE ? lval_err(LERR_BAD_NUM) : lval_num(x);
        }

        char * op = tag->children[1]->contents;

        lval x = eval(tag->children[2]);

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
                operator: '+' | '-' | '*' | '/' | '\%' ; \
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
                        lval result = eval(r.output);
                        lval_println(result);
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
