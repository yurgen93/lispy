#ifndef LENV_H
#define LENV_H

#include "base_types.h"
#include "lval.h"

struct lenv {
        lenv * parent;
        int count;
        lval ** vals;
        char ** syms;
};

/* lenv CONSTRUCOR */
lenv * lenv_new(void);

/* lenv DESTRUCTOR */
void lenv_del(lenv * e);


lval * lenv_get(lenv * e, lval * name);

/* define variable locally */
void lenv_put(lenv * e, lval * name, lval * value);

/* define variable globally */
void lenv_def(lenv * e, lval * name, lval * value);

lenv * lenv_copy(lenv * e);

#endif
