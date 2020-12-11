#ifndef BUILTIN_H
#define BUILTIN_H

#include "lval.h"
#include "lenv.h"
#include "parsers.h"

/* BUILTIN FUNCTIONS */

/* Mathematical functions */

lval * builtin_add(lenv * e, lval * a);
lval * builtin_sub(lenv * e, lval * a);
lval * builtin_mul(lenv * e, lval * a);
lval * builtin_div(lenv * e, lval * a);
lval * builtin_mod(lenv * e, lval * a);
lval * builtin_power(lenv * e, lval * a);
lval * builtin_max(lenv * e, lval * a);
lval * builtin_min(lenv * e, lval * a);

/* List functions */
lval * builtin_head(lenv * e, lval * v);
lval * builtin_tail(lenv * e, lval * v);
lval * builtin_list(lenv * e, lval * v);
lval * builtin_eval(lenv * e, lval * v);
lval * builtin_join(lenv * e, lval * v);

/* define functions */
lval * builtin_lambda(lenv * e, lval * v);
lval * builtin_def(lenv * e, lval * v);
lval * builtin_put(lenv * e, lval * v);
lval * builtin_var(lenv* e, lval * v, char* func);
lval * builtin_fun(lenv * e, lval * v);

lval * builtin_exit(lenv * e, lval * v);

/* ordering functions */

lval * builtin_gt(lenv * e, lval * v);
lval * builtin_ge(lenv * e, lval * v);
lval * builtin_lt(lenv * e, lval * v);
lval * builtin_le(lenv * e, lval * v);
lval * builtin_eq(lenv * e, lval * v);
lval * builtin_ne(lenv * e, lval * v);

/* logical operators */
lval * builtin_logical_and(lenv * e, lval * v);
lval * builtin_logical_or(lenv * e, lval * v);
lval * builtin_logical_not(lenv * e, lval * v);

/* operators */
lval * builtin_if(lenv * e, lval * v);
lval * builtin_load(lenv * e, lval * v);
lval * builtin_print(lenv * e, lval * v);
lval * builtin_error(lenv * e, lval * v);

#endif
