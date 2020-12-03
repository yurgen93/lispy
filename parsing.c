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

#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

typedef enum {
        LVAL_NUM,
        LVAL_ERR,
        LVAL_SYM,
        LVAL_SEXPR,
        LVAL_QEXPR
} lval_type;

typedef struct lval {
        lval_type type;
        long num;

        char * err;
        char * sym;

        int count;
        struct lval ** cell;
} lval;

void lval_print(lval * v);
lval * lval_eval_sexpr(lval * v);

lval * lval_pop(lval * v, int i);
lval * lval_take(lval * v, int i);

lval * builtin_op(lval * v, char * op);

lval * builtin(lval * v, char * func);

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

lval *lval_qexpr(void) {
        lval * v = malloc(sizeof(lval));

        v->type = LVAL_QEXPR;
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
                case LVAL_QEXPR:
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

lval * lval_pop(lval * v, int i) {
        lval * x = v->cell[i];

        memmove(&v->cell[i], &v->cell[i + 1], sizeof(lval *) * (v->count - i - 1));
        v->count--;

        v->cell = realloc(v->cell, sizeof(lval *) * v->count);
        return x;
}

lval * lval_take(lval * v, int i) {
        lval * x = lval_pop(v, i);
        lval_del(v);
        return x;
}

lval *lval_join(lval * x, lval * y) {
        while (y->count)
                x = lval_add(x, lval_pop(y, 0));

        lval_del(y);
        return x;
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
        if (strcmp(tag->tag, ">") == 0) x = lval_sexpr();
        if (strstr(tag->tag, "sexpr")) x = lval_sexpr();
        if (strstr(tag->tag, "qexpr")) x = lval_qexpr();

        for (int i = 0; i < tag->children_num; i++) {
                mpc_ast_t * child_tag = tag->children[i];

                if (strcmp(child_tag->contents, "(") == 0) continue;
                if (strcmp(child_tag->contents, ")") == 0) continue;
                if (strcmp(child_tag->contents, "{") == 0) continue;
                if (strcmp(child_tag->contents, "}") == 0) continue;
                if (strcmp(child_tag->tag, "regex") == 0) continue;

                x = lval_add(x, lval_read(child_tag));
        }

        return x;
}

/* lval printing functions */

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
                case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
        }
}

void lval_println(lval * v) {
        lval_print(v);
        putchar('\n');
}

/* evaluation functions */

lval * lval_eval(lval * v) {
        if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);
        return v;
}

lval * lval_eval_sexpr(lval * v) {
        for (int i = 0; i < v->count; i++)
                v->cell[i] = lval_eval(v->cell[i]);

        for (int i = 0; i < v->count; i++)
                if (v->cell[i]->type == LVAL_ERR)
                        return lval_take(v, i);

        if (v->count == 0) return v;

        if (v->count == 1) return lval_take(v, 0);

        lval * f = lval_pop(v, 0);

        if (f->type != LVAL_SYM) {
                lval_del(f);
                lval_del(v);
                return lval_err("S-expression Does not start with symbol!");
        }

        lval * result = builtin(v, f->sym);
        lval_del(f);
        return result;
}

lval * builtin_op(lval * v, char * op) {
        for (int i = 0; i < v->count; i++) {
                if (v->cell[i]->type != LVAL_NUM) {
                        lval_del(v);
                        return lval_err("Cannot operate on non-number!");
                }
        }

        lval * x = lval_pop(v, 0);

        if ((strcmp(op, "-") == 0) && v->count == 0)
                x->num = - x->num;

        while (v->count > 0) {

                lval * y = lval_pop(v, 0);

                if (strcmp(op, "+") == 0) x->num += y->num;
                if (strcmp(op, "-") == 0) x->num -= y->num;
                if (strcmp(op, "*") == 0) x->num *= y->num;
                if (strcmp(op, "/") == 0) {
                        if (y->num == 0) {
                                lval_del(x);
                                lval_del(y);
                                x = lval_err("Division By Zero!");
                                break;
                        }

                        x->num /= y->num;
                }
                if (strcmp(op, "%") == 0) {
                        if (y->num == 0) {
                                lval_del(x);
                                lval_del(y);
                                x = lval_err("Division By Zero!");
                                break;
                        }

                        x->num %= y->num;
                }

                lval_del(y);

        }

        lval_del(v);

        return x;
}

lval *builtin_head(lval * v) {
        LASSERT(v, v->count == 1, "Function 'head' passed too many arguments!");
        LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'head' passed incorrect types!");
        LASSERT(v, v->cell[0]->count != 0, "Function 'head' passed {}!");

        lval * x = lval_take(v, 0);

        while (x->count > 1) lval_del(lval_pop(x, 1));
        return x;
}

lval *builtin_tail(lval * v) {
        LASSERT(v, v->count == 1, "Function 'tail' passed too many arguments!");
        LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'tail' passed incorrect types!");
        LASSERT(v, v->cell[0]->count != 0, "Function 'tail' passed {}!");

        lval * x = lval_take(v, 0);

        lval_del(lval_pop(x, 0));

        return x;
}

lval *builtin_list(lval * v) {
        v->type = LVAL_QEXPR;
        return v;
}

lval *builtin_eval(lval * v) {
        LASSERT(v, v->count == 1, "Function 'eval' passed too many arguments!");
        LASSERT(v, v->cell[0]->type == LVAL_QEXPR, "Function 'eval' passed incorrect type!");

        lval * x = lval_take(v, 0);
        x->type = LVAL_SEXPR;
        return lval_eval(x);
}

lval *builtin_join(lval * v) {
        for (int i = 0; i < v->count; i++) {
                LASSERT(v, v->cell[i]->type == LVAL_QEXPR, "Function 'join' passed incorrect type!");
        }

        lval * x = lval_pop(v, 0);

        while(v->count)
                x = lval_join(x, lval_pop(v, 0));

        lval_del(v);

        return x;
}

lval *builtin(lval * v, char * func) {
        if (strcmp("head", func) == 0) return builtin_head(v);
        if (strcmp("tail", func) == 0) return builtin_tail(v);
        if (strcmp("list", func) == 0) return builtin_list(v);
        if (strcmp("join", func) == 0) return builtin_join(v);
        if (strcmp("eval", func) == 0) return builtin_eval(v);
        if (strstr("+-*/\%", func))      return builtin_op(v, func);

        lval_del(v);
        return lval_err("Unknown Function!");
}

int main(int argc, char** argv) {
        mpc_parser_t
                *Number = mpc_new("number"),
                *Symbol = mpc_new("symbol"),
                *Sexpr = mpc_new("sexpr"),
                *Qexpr = mpc_new("qexpr"),
                *Expr = mpc_new("expr"),
                *Lispy = mpc_new("lispy");

        mpc_result_t r;

        mpca_lang(
                MPCA_LANG_DEFAULT,
                " \
                number : /-?[0-9]+/ ; \
                symbol : '+' | '-' | '*' | '/' | '\%' | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" ; \
                sexpr  : '(' <expr>* ')' ; \
                qexpr  : '{' <expr>* '}' ; \
                expr   : <number> | <symbol> | <sexpr> | <qexpr> ; \
                lispy  : /^/ <expr>* /$/ ; \
                ",
                Number,
                Symbol,
                Sexpr,
                Qexpr,
                Expr,
                Lispy
        );


        puts("Lispy Version 0.0.0.0.6");
        puts("Press Ctrl+c to Exit\n");


        while (1) {
                char* input = readline("lispy> ");

                add_history(input);

                if (mpc_parse("<stdin>", input , Lispy, &r)) {
                        lval * y = lval_read(r.output);
                        lval * x = lval_eval(y);
                        lval_println(x);
                        lval_del(x);
                        mpc_ast_delete(r.output);
                } else {
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                free(input);
        }

        mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

        return 0;
}
