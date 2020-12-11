#include <stdlib.h>

#include "lenv.h"


/* lenv CONSTRUCOR */

lenv * lenv_new(void) {
        lenv * v = malloc(sizeof(lenv));

        v->parent = NULL;
        v->count = 0;
        v->syms = NULL;
        v->vals = NULL;

        return v;
}

/* lenv DESTRUCTOR */

void lenv_del(lenv * e) {
        for (int i = 0; i < e->count; i++) {
                free(e->syms[i]);
                lval_del(e->vals[i]);
        }

        free(e->syms);
        free(e->vals);
        free(e);
}

/* lenv interface */

lval * lenv_get(lenv * e, lval * name) {
        for (int i = 0; i < e->count; i++)
                if (strcmp(e->syms[i], name->sym) == 0)
                        return lval_copy(e->vals[i]);

        if (e->parent)
                return lenv_get(e->parent, name);
        else
                return lval_err("Undound Symbol '%s'", name->sym);
}

/* define variable locally */
void lenv_put(lenv * e, lval * name, lval * value) {
        for (int i = 0; i < e->count; i++) {
                if (strcmp(e->syms[i], name->sym) == 0) {
                        lval_del(e->vals[i]);
                        e->vals[i] = lval_copy(value);
                        return;
                }
        }

        e->count++;

        e->syms = realloc(e->syms, sizeof(char*) * e->count);
        e->vals = realloc(e->vals, sizeof(lval*) * e->count);

        e->syms[e->count - 1] = malloc(strlen(name->sym) + 1);
        strcpy(e->syms[e->count - 1], name->sym);

        e->vals[e->count - 1] = lval_copy(value);
}

/* define variable globally */
void lenv_def(lenv * e, lval * name, lval * value) {
        while (e->parent) e = e->parent;
        lenv_put(e, name, value);
}

lenv * lenv_copy(lenv * e) {
        lenv * copy = malloc(sizeof(lenv));
        copy->parent = e->parent;
        copy->count = e->count;
        copy->syms = malloc(sizeof(char*) * copy->count);
        copy->vals = malloc(sizeof(lval*) * copy->count);

        for (int i = 0; i < e->count; i++) {
                copy->syms[i] = malloc(strlen(e->syms[i]) + 1);
                strcpy(copy->syms[i], e->syms[i]);
                copy->vals[i] = lval_copy(e->vals[i]);
        }

        return copy;
}
