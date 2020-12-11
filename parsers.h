#ifndef PARSERS_H
#define PARSERS_H

#include "mpc.h"

extern mpc_parser_t * Number;
extern mpc_parser_t * Symbol;
extern mpc_parser_t * String;
extern mpc_parser_t * Comment;
extern mpc_parser_t * Sexpr;
extern mpc_parser_t * Qexpr;
extern mpc_parser_t * Expr;
extern mpc_parser_t * Lispy;

#endif