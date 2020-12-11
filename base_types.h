#ifndef BASE_TYPES_H
#define BASE_TYPES_H

struct lval;
struct lenv;

typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);

/* basic user types of Lispy */
typedef enum {
        LVAL_NUM,
        LVAL_ERR,
        LVAL_SYM,
        LVAL_STR,
        LVAL_FUN,
        LVAL_SEXPR,
        LVAL_QEXPR
} lval_type;

/* error types */
typedef enum {
        L_ERROR_EXIT,
        L_ERROR_STANDARD,
} l_error_type;

#endif
