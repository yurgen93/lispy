#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

mpc_parser_t * Number;
mpc_parser_t * Symbol;
mpc_parser_t * String ;
mpc_parser_t * Comment;
mpc_parser_t * Sexpr;
mpc_parser_t * Qexpr;
mpc_parser_t * Expr;
mpc_parser_t * Lispy;

#include "lval.h"
#include "lenv.h"
#include "builtin.h"

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


/* evaluation functions */

lval * lval_eval(lenv * e, lval * v) {
        if (v->type == LVAL_SYM) {
                lval * x = lenv_get(e, v);
                lval_del(v);
                return x;
        }

        if (v->type == LVAL_SEXPR)
                return lval_eval_sexpr(e, v);

        return v;
}

lval * lval_eval_sexpr(lenv * e, lval * v) {
        for (int i = 0; i < v->count; i++)
                v->cell[i] = lval_eval(e, v->cell[i]);

        for (int i = 0; i < v->count; i++)
                if (v->cell[i]->type == LVAL_ERR)
                        return lval_take(v, i);

        if (v->count == 0) return v;

        if (v->count == 1) return lval_take(v, 0);

        lval * f = lval_pop(v, 0);

        if (f->type != LVAL_FUN) {
                lval * err = lval_err(
                        "S-Expression starts with incorrect type. Got %s, Expected %s.",
                        ltype_name(f->type),
                        ltype_name(LVAL_FUN)
                );
                lval_del(f);
                lval_del(v);
                return err;
        }

        lval * result = lval_call(e, f, v);
        lval_del(f);
        return result;
}

lval *lval_call(lenv * e, lval * func, lval * args) {
        if (func->builtin) return func->builtin(e, args);

        int given = args->count;
        int total = func->formals->count;

        while (args->count > 0) {
                if (func->formals->count == 0) {
                        lval_del(args);
                        return lval_err(
                                "Function passed too many arguments. Got %d, Expected %d.",
                                given,
                                total
                        );
                }

                lval * sym = lval_pop(func->formals, 0);

                if (strcmp(sym->sym, "&") == 0) {
                        if (func->formals->count != 1) {
                                lval_del(args);
                                return lval_err(
                                        "Function format invalid. "
                                        "Symbol '&' not followed by single symbol."
                                );
                        }

                        lval * nsym = lval_pop(func->formals, 0);
                        lenv_put(func->env, nsym, builtin_list(e, args));
                        lval_del(sym);
                        lval_del(nsym);
                        break;
                }

                lval * val = lval_pop(args, 0);

                lenv_put(func->env, sym, val);

                lval_del(sym);

                lval_del(val);
        }

        lval_del(args);

        if (func->formals->count > 0 && strcmp(func->formals->cell[0]->sym, "&") == 0) {
                if (func->formals->count != 2) {
                        return lval_err(
                                "Function format invalid. "
                                "Symbol '&' not followed by single symbol."
                        );
                }

                lval_del(lval_pop(func->formals, 0));

                lval * sym = lval_pop(func->formals, 0);
                lval * value = lval_qexpr();

                lenv_put(func->env, sym, value);
                lval_del(sym);
                lval_del(value);
        }

        if (func->formals->count == 0) {
                func->env->parent = e;
                return builtin_eval(
                        func->env,
                        lval_add(
                                lval_sexpr(),
                                lval_copy(func->body)
                        )
                );

        } else {
                return lval_copy(func);
        }
}

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
