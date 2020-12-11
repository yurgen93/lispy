#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mpc.h"
#include "parsers.h"
#include "lval.h"
#include "lenv.h"
#include "builtin.h"
#include "eval.h"

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

void lval_print(lval * v);
lval * lval_eval_sexpr(lenv * e, lval * v);

lval * lval_pop(lval * v, int i);
lval * lval_take(lval * v, int i);
lval * lval_copy(lval * v);
lval * lval_call(lenv * e, lval * func, lval * args);

lenv * lenv_new(void);
lenv * lenv_copy(lenv * e);
void lenv_del(lenv * e);

lval * builtin_op(lenv * e, lval * v, char * op);

lval * lenv_get(lenv * e, lval * name);
void lenv_put(lenv * e, lval * name, lval * value);
void lenv_def(lenv * e, lval * name, lval * value);

lval * builtin_eval(lenv * e, lval * v);
lval * builtin_var(lenv * e, lval * v, char * op);
lval * builtin_list(lenv * e, lval * v);

void lenv_add_builtin(lenv * e, char * name, lbuiltin fun) {
        lval * name_symbol = lval_sym(name);
        lval * value = lval_fun(fun, name);
        lenv_put(e, name_symbol, value);
        lval_del(name_symbol);
        lval_del(value);
}

void lenv_add_builtins(lenv* e) {
        /* List functions */
        lenv_add_builtin(e, "list", builtin_list);
        lenv_add_builtin(e, "head", builtin_head);
        lenv_add_builtin(e, "tail", builtin_tail);
        lenv_add_builtin(e, "join", builtin_join);
        lenv_add_builtin(e, "eval", builtin_eval);

        /* Mathematical functions */
        lenv_add_builtin(e, "+", builtin_add);
        lenv_add_builtin(e, "-", builtin_sub);
        lenv_add_builtin(e, "*", builtin_mul);
        lenv_add_builtin(e, "/", builtin_div);
        lenv_add_builtin(e, "\%", builtin_mod);
        lenv_add_builtin(e, "^", builtin_power);
        lenv_add_builtin(e, "max", builtin_max);
        lenv_add_builtin(e, "min", builtin_min);

        /* Comparasion functions */
        lenv_add_builtin(e, ">", builtin_gt);
        lenv_add_builtin(e, "<", builtin_lt);
        lenv_add_builtin(e, ">=", builtin_ge);
        lenv_add_builtin(e, "<=", builtin_le);
        lenv_add_builtin(e, "==", builtin_eq);
        lenv_add_builtin(e, "!=", builtin_ne);

        /* Variable functions */
        lenv_add_builtin(e, "def", builtin_def);
        lenv_add_builtin(e, "=", builtin_put);
        lenv_add_builtin(e, "\\", builtin_lambda);
        lenv_add_builtin(e, "fun", builtin_fun);

        /* System functions */
        lenv_add_builtin(e, "exit", builtin_exit);
        lenv_add_builtin(e, "load", builtin_load);
        lenv_add_builtin(e, "print", builtin_print);
        lenv_add_builtin(e, "error", builtin_error);

        /* logical functions */
        lenv_add_builtin(e, "if", builtin_if);
        lenv_add_builtin(e, "&&", builtin_logical_and);
        lenv_add_builtin(e, "||", builtin_logical_or);
        lenv_add_builtin(e, "!", builtin_logical_not);
}

int main(int argc, char** argv) {
        Number = mpc_new("number");
        Symbol = mpc_new("symbol");
        String = mpc_new("string");
        Comment = mpc_new("comment");
        Sexpr = mpc_new("sexpr");
        Qexpr = mpc_new("qexpr");
        Expr = mpc_new("expr");
        Lispy = mpc_new("lispy");

        mpc_result_t r;

        mpca_lang(
                MPCA_LANG_DEFAULT,
                " \
                number : /-?[0-9]+/ ; \
                symbol : /[a-zA-Z0-9_+\\-*\\/\\\\^=<>!&\%|]+/ ; \
                string : /\"((\\\\.)|[^\"])*\"/ ; \
                comment : /;[^\\r\\n]*/ ;  \
                sexpr  : '(' <expr>* ')' ; \
                qexpr  : '{' <expr>* '}' ; \
                expr   : <number> | <symbol> | <string> | <comment> | <sexpr> | <qexpr> ; \
                lispy  : /^/ <expr>* /$/ ; \
                ",
                Number,
                Symbol,
                String,
                Comment,
                Sexpr,
                Qexpr,
                Expr,
                Lispy
        );

        lenv * env = lenv_new();
        lenv_add_builtins(env);

        if (argc >= 2) {
                for (int i = 1; i < argc; i++) {
                        lval * args = lval_add(lval_sexpr(), lval_str(argv[i]));
                        lval * x = builtin_load(env, args);
                        if (x->type == LVAL_ERR) lval_println(x);
                        lval_del(x);
                }
        } else {
                int is_exit = 0;

                puts("Lispy Version 0.0.0.1.0");
                puts("Press Ctrl+c to Exit\n");

                while (!is_exit) {
                        char* input = readline("lispy> ");

                        add_history(input);

                        if (mpc_parse("<stdin>", input , Lispy, &r)) {
                                lval * x = lval_eval(env, lval_read(r.output));
                                is_exit = (x->type == LVAL_ERR) && (x->errtype == L_ERROR_EXIT);
                                lval_println(x);
                                lval_del(x);
                                mpc_ast_delete(r.output);
                        } else {
                                mpc_err_print(r.error);
                                mpc_err_delete(r.error);
                        }

                        free(input);
                }
        }

        lenv_del(env);

        mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Lispy);

        return 0;
}
