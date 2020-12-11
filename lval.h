#ifndef LVAL_H
#define LVAL_H

#include "base_types.h"
#include "lenv.h"
#include "mpc.h"

struct lval {
        lval_type type;

        /* Basic */
        long num;
        char * err;
        char * sym;
        char * str;
        l_error_type errtype;

        /* Expression */
        int count;
        struct lval ** cell;

        /* Function */
        lbuiltin builtin;
        char * builtin_name;
        lenv * env;
        lval * formals;
        lval * body;
};

/* lval CONSTRUCTORS */
lval * lval_num(long x);
lval * lval_err(char * fmt, ...);
lval * lval_sym(char * symbol);
lval * lval_sexpr(void);
lval * lval_qexpr(void);
lval * lval_fun(lbuiltin fun, char * fun_name);
lval * lval_lambda(lval * formals, lval * body);
lval * lval_exit_error(void);
lval * lval_str(char * s);

/* lval DESTRUCTOR */
void lval_del(lval * v);

/* lval manipulation */
lval * lval_add(lval * v, lval * x);
lval * lval_pop(lval * v, int i);
lval * lval_take(lval * v, int i);
lval * lval_join(lval * x, lval * y);
lval * lval_copy(lval * v);
int lval_eq(lval * x, lval * y);

char * ltype_name(lval_type t);

/* CREATE lval from AST element */
lval *lval_read_num(mpc_ast_t * tag);
lval * lval_read_str(mpc_ast_t * tag);
lval * lval_read(mpc_ast_t * tag);

/* lval printing functions */

void lval_print_str(lval* v);
void lval_expr_print(lval * v, char open, char close);
void lval_print(lval * v);
void lval_println(lval * v);

#endif
