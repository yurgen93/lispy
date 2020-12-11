#include "eval.h"
#include "builtin.h"
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
