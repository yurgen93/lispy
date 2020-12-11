#include <stdlib.h>

#include "mpc.h"
#include "parsers.h"
#include "builtin.h"

/* BUILTIN MATHEMATICAL FUNCTIONS */

#define LASSERT(args, cond, fmt, ...) \
        if (!(cond)) { \
                lval * err = lval_err(fmt, ##__VA_ARGS__) ; \
                lval_del(args); \
                return err; \
        }

#define LASSERT_TYPE(func, args, index, expect) \
                LASSERT(args, args->cell[index]->type == expect, \
                "Function '%s' passed incorrect type of argument %d. Got %s, expected %s", \
                func, index, ltype_name(args->cell[index]->type), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
                LASSERT(args, args->count == num, \
                "Function '%s' passed incorrect number of arguments. Got %d, expected %d", func, args->count, num)

#define LASSERT_NUM_AT_LEAST(func, args, num) \
                LASSERT(args, args->count >= num, \
                "Function '%s' passed incorrect number of arguments. Got %d, expected %d or greater", func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
                LASSERT(args, args->count != 0, \
                "Function '%s' passed {} for argument %d", func, index)

lval * builtin_op(lenv * e, lval * v, char * op) {
        for (int i = 0; i < v->count; i++) {
                LASSERT_TYPE(op, v, i, LVAL_NUM);
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
                if (strcmp(op, "^") == 0) x->num = (int)pow(x->num, y->num);
                if (strcmp(op, "max") == 0) {
                        if (x->num < y->num) {
                                x->num = y->num;
                        }
                }
                if (strcmp(op, "min") == 0) {
                        if (x->num > y->num) {
                                x->num = y->num;
                        }
                }

                lval_del(y);

        }

        lval_del(v);

        return x;
}

lval * builtin_add(lenv * e, lval * a) {
        return builtin_op(e, a, "+");
}

lval * builtin_sub(lenv * e, lval * a) {
        return builtin_op(e, a, "-");
}

lval * builtin_mul(lenv * e, lval * a) {
        return builtin_op(e, a, "*");
}

lval * builtin_div(lenv * e, lval * a) {
        return builtin_op(e, a, "/");
}

lval * builtin_mod(lenv * e, lval * a) {
        return builtin_op(e, a, "\%");
}

lval * builtin_power(lenv * e, lval * a) {
        return builtin_op(e, a, "^");
}

lval * builtin_max(lenv * e, lval * a) {
        return builtin_op(e, a, "max");
}

lval * builtin_min(lenv * e, lval * a) {
        return builtin_op(e, a, "min");
}

/* BUILTIN FUNCTIONS */

lval * builtin_head(lenv * e, lval * v) {
        LASSERT_NUM("head", v, 1);
        LASSERT_TYPE("head", v, 0, LVAL_QEXPR);
        LASSERT_NOT_EMPTY("head", v, 0);

        lval * x = lval_take(v, 0);

        while (x->count > 1) lval_del(lval_pop(x, 1));
        return x;
}

lval * builtin_tail(lenv * e, lval * v) {
        LASSERT_NUM("tail", v, 1);
        LASSERT_TYPE("tail", v, 0, LVAL_QEXPR);
        LASSERT_NOT_EMPTY("tail", v, 0);

        lval * x = lval_take(v, 0);

        lval_del(lval_pop(x, 0));

        return x;
}

lval * builtin_list(lenv * e, lval * v) {
        v->type = LVAL_QEXPR;
        return v;
}

lval * builtin_eval(lenv * e, lval * v) {
        LASSERT_NUM("eval", v, 1);
        LASSERT_TYPE("eval", v, 0, LVAL_QEXPR);

        lval * x = lval_take(v, 0);
        x->type = LVAL_SEXPR;
        return lval_eval(e, x);
}

lval * builtin_join(lenv * e, lval * v) {
        for (int i = 0; i < v->count; i++) {
                LASSERT_TYPE("join", v, i, LVAL_QEXPR);
        }

        lval * x = lval_pop(v, 0);

        while(v->count)
                x = lval_join(x, lval_pop(v, 0));

        lval_del(v);

        return x;
}

lval * builtin_lambda(lenv * e, lval * v) {
        LASSERT_NUM("\\", v, 2);
        LASSERT_TYPE("\\", v, 0, LVAL_QEXPR);
        LASSERT_TYPE("\\", v, 1, LVAL_QEXPR);

        for (int i = 0; i < v->cell[0]->count; i++)
                LASSERT(v, (v->cell[0]->cell[i]->type == LVAL_SYM),
                "Cannot define non-symbol. Got %s, Expected %s.",
                ltype_name(v->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));

        lval * formals = lval_pop(v, 0);
        lval * body = lval_pop(v, 0);
        lval_del(v);

        return lval_lambda(formals, body);
}

lval * builtin_def(lenv * e, lval * v) {
        return builtin_var(e, v, "def");
}

lval * builtin_put(lenv * e, lval * v) {
        return builtin_var(e, v, "=");
}

lval * builtin_var(lenv* e, lval * v, char* func) {
        LASSERT_TYPE(func, v, 0, LVAL_QEXPR);

        lval * syms = v->cell[0];

        for (int i = 0; i < syms->count; i++) {
                LASSERT(v, (syms->cell[i]->type == LVAL_SYM),
                "Function '%s' cannot define non-symbol. "
                "Got %s, Expected %s.",
                func,
                ltype_name(syms->cell[i]->type),
                ltype_name(LVAL_SYM)
                );
        }

        LASSERT(v, (syms->count == v->count - 1),
                "Function '%s' passed too many arguments for symbols. "
                "Got %i, Expected %i.",
                func,
                syms->count,
                v->count-1);

        for (int i = 0; i < syms->count; i++) {
                if (strcmp(func, "def") == 0)
                        lenv_def(e, syms->cell[i], v->cell[i + 1]);

                if (strcmp(func, "=") == 0)
                        lenv_put(e, syms->cell[i], v->cell[i + 1]);
        }

        lval_del(v);

        return lval_sexpr();
}

lval * builtin_exit(lenv * e, lval * v) {
        lval * exit = lval_exit_error();
        lval_del(v);
        return exit;
}

lval * builtin_fun(lenv * e, lval * v) {
        LASSERT_NUM("fun", v, 2);
        LASSERT_TYPE("fun", v, 0, LVAL_QEXPR);
        LASSERT_TYPE("fun", v, 1, LVAL_QEXPR);

        for (int i = 0; i < v->cell[0]->count; i++) {
                LASSERT(v, (v->cell[0]->cell[i]->type == LVAL_SYM),
                "Function 'fun' cannot define non-symbol. "
                "Got %s, expected %s",
                ltype_name(v->cell[0]->cell[i]->type),
                ltype_name(LVAL_SYM)
                );
        }

        LASSERT(v, v->cell[0]->count != 0, "Function 'fun' first argument cannot be '{}'");

        lval * func_name = lval_pop(v->cell[0], 0);
        lval * formals = v->cell[0];
        lval * body = v->cell[1];

        lenv_def(e, func_name, lval_lambda(formals, body));

        return lval_sexpr();
}

/* ordering functions */

lval * builtin_ord(lenv * e, lval * v, char * op) {
        LASSERT_NUM(op, v, 2);
        LASSERT_TYPE(op, v, 0, LVAL_NUM);
        LASSERT_TYPE(op, v, 1, LVAL_NUM);

        int r;

        if (strcmp(op, ">") == 0)
                r = v->cell[0]->num > v->cell[1]->num;
        if (strcmp(op, ">=") == 0)
                r = v->cell[0]->num >= v->cell[1]->num;
        if (strcmp(op, "<") == 0)
                r = v->cell[0]->num < v->cell[1]->num;
        if (strcmp(op, "<=") == 0)
                r = v->cell[0]->num <= v->cell[1]->num;

        lval_del(v);
        return lval_num(r);
}

lval * builtin_gt(lenv * e, lval * v) {
        return builtin_ord(e, v, ">");
}

lval * builtin_ge(lenv * e, lval * v) {
        return builtin_ord(e, v, ">=");
}

lval * builtin_lt(lenv * e, lval * v) {
        return builtin_ord(e, v, "<");
}

lval * builtin_le(lenv * e, lval * v) {
        return builtin_ord(e, v, "<=");
}

lval * builtin_cmp(lenv * e, lval * v, char * op) {
        LASSERT_NUM(op, v, 2);

        int r;
        if (strcmp(op, "==") == 0) r = lval_eq(v->cell[0], v->cell[1]);
        if (strcmp(op, "!=") == 0) r = lval_eq(v->cell[0], v->cell[1]);

        lval_del(v);
        return lval_num(r);
}

lval * builtin_eq(lenv * e, lval * v) {
        return builtin_cmp(e, v, "==");
}

lval * builtin_ne(lenv * e, lval * v) {
        return builtin_cmp(e, v, "!=");
}

/* logical operators */
lval * builtin_logical_and(lenv * e, lval * v) {
        LASSERT_NUM_AT_LEAST("&&", v, 2);

        for (int i = 0; i < v->count; i++)
                LASSERT_TYPE("&&", v, i, LVAL_NUM);

        int r = 1;

        for (int i = 0; i < v->count; i++)
                r = r && v->cell[i]->num;

        lval_del(v);
        return lval_num(r);
}

lval * builtin_logical_or(lenv * e, lval * v) {
        LASSERT_NUM_AT_LEAST("||", v, 2);

        for (int i = 0; i < v->count; i++)
                LASSERT_TYPE("||", v, i, LVAL_NUM);


        int r = 0;

        for (int i = 0; i < v->count; i++)
                r = r || v->cell[i]->num;

        lval_del(v);
        return lval_num(r);
}

lval * builtin_logical_not(lenv * e, lval * v) {
        LASSERT_NUM("!", v, 1);
        LASSERT_TYPE("!", v, 0, LVAL_NUM);

        int r = !v->cell[0]->num;

        lval_del(v);
        return lval_num(r);
}

/* operators */

lval * builtin_if(lenv * e, lval * v) {
        LASSERT(
                v,
                (v->count == 2 || v->count == 3),
                "Function 'if' passed incorrect number of argument. Got %d, expected 2 or 3.",
                v->count
        );
        LASSERT_TYPE("if", v, 0, LVAL_QEXPR);
        LASSERT_TYPE("if", v , 1, LVAL_QEXPR);
        // if else branch is present
        if (v->count == 3) LASSERT_TYPE("if", v, 2, LVAL_QEXPR);

        lval * cond_expr = lval_pop(v, 0);

        cond_expr->type = LVAL_SEXPR;

        lval * cond_res = lval_eval(e, cond_expr);

        if (cond_res->type == LVAL_ERR) {
                lval_del(v);
                return cond_res;
        }

        lval * branch; // if or else branch

        // treat not number like TRUE
        if (cond_res->type != LVAL_NUM) branch = lval_pop(v, 0);
        else if (cond_res->num != 0) branch = lval_pop(v, 0);
        else if (v->count == 2) branch = lval_pop(v, 1);
        else branch = NULL;

        lval_del(v);
        lval_del(cond_res);

        if (branch == NULL) return lval_sexpr();

        branch->type = LVAL_SEXPR;
        return lval_eval(e, branch);
}

lval* builtin_load(lenv * e, lval * v) {
        LASSERT_NUM("load", v, 1);
        LASSERT_TYPE("load", v, 0, LVAL_STR);

        /* Parse File given by string name */
        mpc_result_t r;

        if (mpc_parse_contents(v->cell[0]->str, Lispy, &r)) {
                lval * expr = lval_read(r.output);
                mpc_ast_delete(r.output);

                while (expr->count) {
                        lval * x = lval_eval(e, lval_pop(expr, 0));
                        if (x->type == LVAL_ERR) lval_println(x);
                        lval_del(x);
                }

                lval_del(expr);
                lval_del(v);

                return lval_sexpr();
        } else {
                char * err_msg = mpc_err_string(r.error);
                mpc_err_delete(r.error);

                lval * err = lval_err("Could not load Library %s", err_msg);

                free(err_msg);
                lval_del(v);

                return err;
        }
}

lval * builtin_print(lenv * e, lval * v) {
        for (int i = 0; i < v->count; i++) {
                lval_print(v->cell[i]);
                putchar(' ');
        }

        putchar('\n');
        lval_del(v);

        return lval_sexpr();
}

lval * builtin_error(lenv * e, lval * v) {
        LASSERT_NUM("error", v, 1);
        LASSERT_TYPE("error", v, 0, LVAL_STR);

        lval * err = lval_err(v->cell[0]->str);

        lval_del(v);

        return err;
}
