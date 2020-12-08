#include <stdio.h>
#include <stdlib.h>
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

#define LASSERT_NOT_EMPTY(func, args, index) \
                LASSERT(args, args->count != 0, \
                "Function '%s' passed {} for argument %d", func, index)

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);

typedef enum {
        LVAL_NUM,
        LVAL_ERR,
        LVAL_SYM,
        LVAL_STR,
        LVAL_FUN,
        LVAL_SEXPR,
        LVAL_QEXPR
} lval_type;

typedef enum {
        L_ERROR_EXIT,
        L_ERROR_STANDARD,
} l_error_type;

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

struct lenv {
        lenv * parent;
        int count;
        lval ** vals;
        char ** syms;
};

mpc_parser_t * Number;
mpc_parser_t * Symbol;
mpc_parser_t * String ;
mpc_parser_t * Comment;
mpc_parser_t * Sexpr;
mpc_parser_t * Qexpr;
mpc_parser_t * Expr;
mpc_parser_t * Lispy;

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

/* BUILTIN MATHEMATICAL FUNCTIONS */

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
        LASSERT_NUM("&&", v, 2);
        LASSERT_TYPE("&&", v, 0, LVAL_NUM);
        LASSERT_TYPE("&&", v, 1, LVAL_NUM);

        int r = v->cell[0]->num && v->cell[1]->num;

        lval_del(v);
        return lval_num(r);
}

lval * builtin_logical_or(lenv * e, lval * v) {
        LASSERT_NUM("||", v, 2);
        LASSERT_TYPE("||", v, 0, LVAL_NUM);
        LASSERT_TYPE("||", v, 1, LVAL_NUM);

        int r = v->cell[0]->num || v->cell[1]->num;

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

lenv *lenv_copy(lenv * e) {
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
                symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&\%|]+/ ; \
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

                puts("Lispy Version 0.0.0.0.8");
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
