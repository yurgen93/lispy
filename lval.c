#include <stdio.h>
#include <stdlib.h>

#include "lval.h"

/* lval CONSTRUCTORS */

lval * lval_num(long x) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_NUM;
        v->num = x;

        return v;
}

lval * lval_err(char * fmt, ...) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_ERR;

        va_list va;
        va_start(va, fmt);

        v->err = malloc(512);

        vsnprintf(v->err, 511, fmt, va);

        v->err = realloc(v->err, strlen(v->err) + 1);

        va_end(va);

        return v;
}

lval * lval_sym(char * symbol) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_SYM;
        v->sym = malloc(strlen(symbol) + 1);
        strcpy(v->sym, symbol);

        return v;
}

lval * lval_sexpr(void) {
        lval *v = malloc(sizeof(lval));

        v->type = LVAL_SEXPR;
        v->count = 0;
        v->cell = NULL;

        return v;
}

lval * lval_qexpr(void) {
        lval * v = malloc(sizeof(lval));

        v->type = LVAL_QEXPR;
        v->count = 0;
        v->cell = NULL;

        return v;
}

lval * lval_fun(lbuiltin fun, char * fun_name) {
        lval * v = malloc(sizeof(lval));

        v->type = LVAL_FUN;
        v->builtin = fun;
        v->builtin_name = NULL;

        // builtin functions need a name
        if (fun_name == NULL) {
                v->builtin_name = NULL;
        } else {
                v->builtin_name = malloc(512);
                strcpy(v->builtin_name, fun_name);
                v->builtin_name = realloc(v->builtin_name, strlen(v->builtin_name) + 1);
        }

        return v;
}

lval * lval_lambda(lval * formals, lval * body) {
        lval * v = malloc(sizeof(lval));

        v->type = LVAL_FUN;
        v->builtin = NULL;
        v->builtin_name = NULL;

        v->env = lenv_new();

        v->formals = lval_copy(formals);
        v->body = lval_copy(body);

        return v;
}

lval * lval_exit_error(void) {
        lval * exit_error = lval_err("EXIT");
        exit_error->errtype = L_ERROR_EXIT;
        return exit_error;
}

lval * lval_str(char * s) {
        lval * v = malloc(sizeof(lval));

        v->type = LVAL_STR;
        v->str = malloc(strlen(s) + 1);
        strcpy(v->str, s);

        return v;
}

/* lval DESTRUCTOR */

void lval_del(lval * v) {
        switch (v->type) {
                case LVAL_NUM: break;
                case LVAL_FUN:
                        if (v->builtin == NULL) {
                                lenv_del(v->env);
                                lval_del(v->formals);
                                lval_del(v->body);
                        }
                        if (v->builtin_name != NULL)
                                free(v->builtin_name);
                        break;
                case LVAL_ERR: free(v->err); break;
                case LVAL_SYM: free(v->sym); break;
                case LVAL_STR: free(v->str); break;
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

lval * lval_join(lval * x, lval * y) {
        while (y->count)
                x = lval_add(x, lval_pop(y, 0));

        lval_del(y);
        return x;
}

lval * lval_copy(lval * v) {
        lval * copy = malloc(sizeof(lval));

        copy->type = v->type;

        switch(v->type) {
                case LVAL_NUM: copy->num = v->num; break;
                case LVAL_FUN:
                        copy->builtin = v->builtin;

                        if (v->builtin_name != NULL) {
                                copy->builtin_name = malloc(strlen(v->builtin_name) + 1);
                                strcpy(copy->builtin_name, v->builtin_name);
                        } else {
                                copy->builtin_name = NULL;
                        }

                        if (v->builtin == NULL) {
                                copy->env = lenv_copy(v->env);
                                copy->formals = lval_copy(v->formals);
                                copy->body = lval_copy(v->body);
                        }
                        break;
                case LVAL_SYM:
                        copy->sym = malloc(strlen(v->sym) + 1);
                        strcpy(copy->sym, v->sym);
                        break;
                case LVAL_ERR:
                        copy->err = malloc(strlen(v->err) + 1);
                        strcpy(copy->err, v->err);
                        break;
                case LVAL_STR:
                        copy->str = malloc(strlen(v->str) + 1);
                        strcpy(copy->str, v->str);
                        break;
                case LVAL_SEXPR:
                case LVAL_QEXPR:
                        copy->count = v->count;
                        copy->cell = malloc(sizeof(lval *) * v->count);
                        for (int i = 0; i < v->count; i++)
                                copy->cell[i] = lval_copy(v->cell[i]);
                        break;
        }

        return copy;
}

char * ltype_name(lval_type t) {
        switch(t) {
                case LVAL_FUN: return "Function";
                case LVAL_NUM: return "Number";
                case LVAL_ERR: return "Error";
                case LVAL_SYM: return "Symbol";
                case LVAL_STR: return "String";
                case LVAL_SEXPR: return "S-Expression";
                case LVAL_QEXPR: return "Q-Expression";
                default: return "Unknown";
        }
}

int lval_eq(lval * x, lval * y) {
        if (x->type != y->type) return 0;

        switch (x->type)
        {
        case LVAL_NUM: return x->num == y->num;
        case LVAL_ERR: return (strcmp(x->err, y->err) == 0);
        case LVAL_SYM: return (strcmp(x->sym, y->sym) == 0);
        case LVAL_STR: return (strcmp(x->str, y->str) == 0);
        case LVAL_FUN:
                if (x->builtin || y->builtin) {
                        return x->builtin == y->builtin;
                } else {
                        return lval_eq(x->formals, y->formals) && lval_eq(x->body, y->body);
                }
        case LVAL_QEXPR:
        case LVAL_SEXPR:
                if (x->count != y->count) return 0;
                for (int i = 0; i < x->count; i++)
                        if (!lval_eq(x->cell[i], y->cell[i]))
                                return 0;
                return 1;
        }
        return 0;
}


/* CREATE lval from AST element */

lval *lval_read_num(mpc_ast_t * tag) {
        errno = 0;
        long x = strtol(tag->contents, NULL, 10);
        if (errno != ERANGE) return lval_num(x);

        return lval_err("invalid number");
}

lval * lval_read_str(mpc_ast_t * tag) {
        tag->contents[strlen(tag->contents) - 1] = '\0';

        char * unescaped = malloc(strlen(tag->contents + 1) + 1);
        strcpy(unescaped, tag->contents + 1);

        unescaped = mpcf_unescape(unescaped);

        lval * str = lval_str(unescaped);

        free(unescaped);

        return str;
}

lval * lval_read(mpc_ast_t * tag) {
        if (strstr(tag->tag, "number")) return lval_read_num(tag);
        if (strstr(tag->tag, "symbol")) return lval_sym(tag->contents);
        if (strstr(tag->tag, "string")) return lval_read_str(tag);

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
                if (strstr(child_tag->tag, "comment")) continue;

                x = lval_add(x, lval_read(child_tag));
        }

        return x;
}

/* lval printing functions */

void lval_print_str(lval* v) {
        char * escaped = malloc(strlen(v->str) + 1);
        strcpy(escaped, v->str);
        escaped = mpcf_escape(escaped);
        printf("\"%s\"", escaped);
        free(escaped);
}

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
                case LVAL_STR:   lval_print_str(v); break;
                case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
                case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
                case LVAL_FUN:
                        if (v->builtin != NULL)
                                printf("<builtin: %s>", (v->builtin_name == NULL) ? "unknown" : v->builtin_name);
                        else {
                                printf("(\\ ");
                                lval_print(v->formals);
                                putchar(' ');
                                lval_print(v->body);
                                putchar(')');
                        }
                        break;
        }
}

void lval_println(lval * v) {
        lval_print(v);
        putchar('\n');
}
