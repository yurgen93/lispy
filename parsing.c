#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include <string.h>

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

typedef enum {
        LVAL_NUM,
        LVAL_ERR,
        LVAL_SYM,
        LVAL_SEXPR
} lval_type;

typedef struct {
        lval_type type;
        long num;

        char * err;
        char * sym;

        int count;
        struct lval ** cell;
} lval;

/* lval CONSTRUCTORS */

lval *lval_num(long x) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_NUM;
        v->num = x;

        return v;
}

lval *lval_err(char * message) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_ERR;
        v->err = malloc(strlen(message) + 1);
        strcpy(v->err, message);

        return v;
}

lval *lval_sym(char * symbol) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_SYM;
        v->sym = malloc(strlen(symbol) + 1);
        strcpy(v->sym, symbol);

        return v;
}

lval *lval_sexpr(void) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_SEXPR;
        v->count = 0;
        v->cell = NULL;

        return v;
}

/* lval DESTRUCTOR */

void lval_del(lval * v) {
        switch (v->type) {
                case LVAL_NUM: break;
                case LVAL_ERR: free(v->err); break;
                case LVAL_SYM: free(v->sym); break;
                case LVAL_SEXPR:
                        for (int i = 0; i < v->count; i++) lval_del(v->cell[i]);
                        free(v->cell);
                        break;
        }

        free(v);
}

/* lval manipulation */

lval * lval_add(lval * v, lval * x) {
        v->count++;
        v->cell = realloc(v->cell, sizeof(lval*) * v->count);
        v->cell[v->count - 1] = x;
        return v;
}

/* CREATE lval from AST element */

lval *lval_read_num(mpc_ast_t * tag) {
        errno = 0;
        long x = strtol(tag->contents, NULL, 10);
        if (errno != ERANGE) return lval_num(x);

        return lval_err("invalid number");
}

lval * lval_read(mpc_ast_t * tag) {
        if (strstr(tag->tag, "number")) return lval_read_num(tag);
        if (strstr(tag->tag, "symbol")) return lval_sym(tag->contents);

        lval * x = NULL;

        // ">" means root
        if (strstr(tag->tag, ">")) x = lval_sexpr();
        if (strstr(tag->tag, "sexpr")) x = lval_sexpr();

        for (int i = 0; i < tag->children_num; i++) {
                mpc_ast_t * child_tag = tag->children[i];

                if (strcmp(child_tag->contents, "(") == 0) continue;
                if (strcmp(child_tag->contents, ")") == 0) continue;
                if (strcmp(child_tag->tag, "regex") == 0) continue;

                x = lval_add(x, lval_read(child_tag));
        }

        return x;
}

/* lval printing functions */

void lval_print(lval * v);

void lval_expr_print(lval * v, char open, char close) {
        putchar(open);

        for (int i = 0; i < v->count; i++) {
                lval_print(v->cell[i]);

                if (i != v->count - 1) putchar(' ');
        }

        putchar(close);
}

void lval_print(lval * v) {
        switch (v->type) {
                case LVAL_NUM:   printf("%li", v->num); break;
                case LVAL_ERR:   printf("Error: %s", v->err); break;
                case LVAL_SYM:   printf("%s", v->sym); break;
                case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
        }
}

void lval_println(lval * v) {
        lval_print(v);
        putchar('\n');
}

/* evaluation functions */

// lval eval_op(lval x, char * op, lval y) {
//         if (x.type == LVAL_ERR) return x;
//         if (y.type == LVAL_ERR) return y;

//         if (strcmp(op, "+") == 0) return lval_num(x.num + y.num);
//         if (strcmp(op, "-") == 0) return lval_num(x.num - y.num);
//         if (strcmp(op, "*") == 0) return lval_num(x.num + y.num);
//         if (strcmp(op, "/") == 0) return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
//         if (strcmp(op, "\%") == 0) return (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num % y.num);

//         return lval_err(LERR_BAD_OP);
// }

// lval eval(mpc_ast_t * tag) {
//         if (strstr(tag->tag, "number")) {
//                 errno = 0;
//                 long x = strtol(tag->contents, NULL, 10);
//                 return errno == ERANGE ? lval_err(LERR_BAD_NUM) : lval_num(x);
//         }

//         char * op = tag->children[1]->contents;

//         lval x = eval(tag->children[2]);

//         int i = 3;
//         while (strstr(tag->children[i]->tag, "expr"))
//         {
//                 x = eval_op(x, op, eval(tag->children[i]));
//                 i++;
//         }

//         return x;
// }

int main(int argc, char** argv) {
        mpc_parser_t
                *Number = mpc_new("number"),
                *Symbol = mpc_new("symbol"),
                *Sexpr = mpc_new("sexpr"),
                *Expr = mpc_new("expr"),
                *Lispy = mpc_new("lispy");

        mpc_result_t r;

        mpca_lang(
                MPCA_LANG_DEFAULT,
                " \
                number: /-?[0-9]+/ ; \
                symbol: '+' | '-' | '*' | '/' | '\%' ; \
                sexpr:  '(' <expr>* ')' ; \
                expr: <number> | <symbol> | <sexpr> ; \
                lispy: /^/ <expr>* /$/ ; \
                ",
                Number,
                Symbol,
                Sexpr,
                Expr,
                Lispy
        );


        puts("Lispy Version 0.0.0.0.1");
        puts("Press Ctrl+c to Exit\n");


        while (1) {
                char* input = readline("lispy> ");

                add_history(input);

                if (mpc_parse("<stdin>", input , Lispy, &r)) {
                        lval * x = lval_read(r.output);
                        lval_println(x);
                        lval_del(x);
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(input);
        }

        mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

        return 0;
}
