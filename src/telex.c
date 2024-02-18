/*
 * telex.c - This file is part of libtelex
 * Copyright (C) 2023 Matthias Kruk
 *
 * libtelex is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 3, or (at your
 * option) any later version.
 *
 * libtelex is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libtelex; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <telex/error.h>
#include <telex/telex.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include "telex.h"
#include "parser.h"

struct telex* telex_new(struct token *prefix,
			struct compound_expr *compound_expr)
{
	struct telex *telex;

	if ((telex = calloc(1, sizeof(*telex)))) {
		telex->prefix = prefix;
		telex->compound_expr = compound_expr;
	}

	return telex;
}

int eval_telex(struct telex *telex, const char *start, const size_t size,
               const char *pos, token_type_t prefix, const char **result);

int telex_parse(struct telex **telex,
                const char *input,
                struct telex_error **errors)
{
	struct parser *parser;
	int have_errors;

	if (!(parser = parser_new())) {
		return -ENOMEM;
	}

	have_errors = parser_parse(parser, input);

	if (!have_errors) {
		*telex = parser_get_telex(parser);
	}
	*errors = parser_get_errors(parser);

	parser_free(parser);

	return have_errors;
}

int telex_rlookup(struct telex **telex, const char *start, const char *pos)
{
	struct telex_error *errors;
	char telex_str[64];
	const char *cur;
	int line;
	int col;
	int err;

	if (!telex || !start || !pos) {
		return -EINVAL;
	}

	line = 1;
	col = 0;

	for (cur = start; cur < pos; cur++) {
		if (*cur == '\n') {
			line++;
			col = 0;
			continue;
		}

		col++;
	}

	snprintf(telex_str, sizeof(telex_str), ":%d>#%d", line, col);

	err = telex_parse(telex, telex_str, &errors);

	if (err < 0 && errors) {
		telex_error_free_all(&errors);
	}

	return err;
}

void telex_debug(struct telex *telex)
{
        parser_debug_telex(telex);
}

int line_expr_to_string(struct line_expr *expr, char *str, const size_t str_size)
{
	int total;
	int written;

	if ((total = token_to_string(expr->colon, str, str_size)) < 0) {
		return total;
	}

	if ((written = token_to_string(expr->integer, str + total, str_size - total)) < 0) {
		return total;
	}

	return total + written;
}

int col_expr_to_string(struct col_expr *expr, char *str, const size_t str_size)
{
	int total;
	int written;

	total = 0;

	if (expr->pound) {
		if ((total = token_to_string(expr->pound, str, str_size)) < 0) {
			return total;
		}
	}

	if ((written = token_to_string(expr->integer, str + total, str_size - total)) < 0) {
		return total;
	}

	return total + written;
}

int stringy_to_string(struct stringy *stringy, char *str, const size_t str_size)
{
	return token_to_string(stringy->token, str, str_size);
}

int primary_expr_to_string(struct primary_expr *expr, char *str, const size_t str_size)
{
	int total;
	int written;

	if (expr->line_expr) {
		return line_expr_to_string(expr->line_expr, str, str_size);
	}

	if (expr->col_expr) {
		return col_expr_to_string(expr->col_expr, str, str_size);
	}

	if (expr->stringy) {
		return stringy_to_string(expr->stringy, str, str_size);
	}

	if ((total = token_to_string(expr->lparen, str, str_size)) < 0) {
		goto done;
	}

	if ((written = telex_to_string(expr->telex, str + total, str_size - total)) < 0) {
		goto done;
	}

	total += written;

	if ((written = token_to_string(expr->rparen, str + total, str_size - total)) < 0) {
		goto done;
	}

	total += written;

done:
	return total;
}

int or_expr_to_string(struct or_expr *expr, char *str, const size_t str_size)
{
	int total;
	int written;

	total = 0;

	if (expr->or_expr) {
		if ((written = or_expr_to_string(expr->or_expr, str,
						 str_size)) < 0) {
			goto done;
		}

		total += written;
	}

	if (expr->or) {
		if ((written = token_to_string(expr->or, str + total,
					       str_size - total)) < 0) {
			goto done;
		}

		total += written;
	}

	if (expr->primary_expr) {
		if ((written = primary_expr_to_string(expr->primary_expr, str + total,
						      str_size - total)) < 0) {
			goto done;
		}

		total += written;
	}

done:
	return total;
}

int compound_expr_to_string(struct compound_expr *expr, char *str, const size_t str_size)
{
	int total;
	int written;

	total = 0;

	if (expr->compound_expr) {
		if ((written = compound_expr_to_string(expr->compound_expr, str, str_size)) < 0) {
			goto done;
		}

		total += written;
	}

	if (expr->prefix) {
		if ((written = token_to_string(expr->prefix, str + total, str_size - total)) < 0) {
			goto done;
		}

		total += written;
	}

	if (expr->or_expr) {
		if ((written = or_expr_to_string(expr->or_expr, str + total, str_size - total)) < 0) {
			goto done;
		}

		total += written;
	}

done:
	return total;
}

int telex_to_string(struct telex *telex, char *str, const size_t str_size)
{
	int written;

	written = telex->prefix ? token_to_string(telex->prefix, str, str_size) : 0;

	return compound_expr_to_string(telex->compound_expr, str + written, str_size - written);
}

static struct primary_expr* primary_expr_from_telex(struct telex *telex)
{
	struct token *lparen;
	struct token *rparen;
	struct primary_expr *expr;

	lparen = NULL;
	rparen = NULL;
	expr = NULL;

	if ((lparen = token_new(TOKEN_LPAREN, "(", 1, -1, -1)) &&
	    (rparen = token_new(TOKEN_RPAREN, ")", 1, -1, -1)) &&
	    (expr = primary_expr_nested_new(lparen, telex, rparen))) {
		return expr;
	}

	token_free(&lparen);
	token_free(&rparen);

	return NULL;
}

static struct or_expr* or_expr_from_telex(struct telex *telex)
{
	struct or_expr *expr;

	if ((expr = calloc(1, sizeof(*expr)))) {
		if (!(expr->primary_expr = primary_expr_from_telex(telex))) {
			free(expr);
			expr = NULL;
		}
	}

	return expr;
}

static struct compound_expr* compound_expr_from_telex(struct telex *telex)
{
	struct compound_expr *compound_expr;

	if ((compound_expr = calloc(1, sizeof(*compound_expr)))) {
		if (!(compound_expr->or_expr = or_expr_from_telex(telex))) {
			free(compound_expr);
			compound_expr = NULL;
		}
	}

	return compound_expr;
}

int telex_combine(struct telex **new, const struct telex *first, const struct telex *second)
{
	struct telex *left;
	struct telex *right;
	struct token *left_op;
	struct token *concat_op;
	struct compound_expr *left_expr;
	struct or_expr *right_expr;
	struct compound_expr *combined_expr;

	if (!new || !first || !second) {
		return -EINVAL;
	}

	/*
	 * If second telex isn't relative, we can't know what
	 * operator to use to concatenate the two expressions.
	 */
	if (!telex_is_relative(second)) {
		return -EBADE;
	}

	if (!(left = telex_clone(first))) {
		return -ENOMEM;
	}
	if (!(right = telex_clone(second))) {
		telex_free(&left);
		return -ENOMEM;
	}

	/*
	 * The value of these two would otherwise be undefined if something in
	 * the if-statement below evaluates to false
	 */
	right_expr = NULL;
	combined_expr = NULL;

	left_op = left->prefix;
	left->prefix = NULL;
	concat_op = right->prefix;
	right->prefix = NULL;

	if ((left_expr = compound_expr_from_telex(left)) &&
	    ((right_expr = or_expr_from_telex(right))) &&
	    ((combined_expr = compound_expr_new(left_expr, concat_op, right_expr))) &&
	    ((*new = telex_new(left_op, combined_expr)))) {
		return 0;
	}

	compound_expr_free(&left_expr);
	or_expr_free(&right_expr);
	compound_expr_free(&combined_expr);
	telex_free(&left);
	telex_free(&right);
	token_free(&left_op);
	token_free(&concat_op);

	return -ENOMEM;
}

void telex_simplify(struct telex *telex)
{
	/* FIXME: Should this be done during parsing? */
	return;
}

const char* telex_lookup(struct telex *telex,
			 const char *start,
			 const size_t size,
			 const char *pos)
{
	const char *result;
	token_type_t prefix;

	result = NULL;
	prefix = telex->prefix ? telex->prefix->type : TOKEN_INVALID;

	if (eval_telex(telex, start, size, pos, prefix, &result) < 0) {
		return NULL;
	}

	return result;
}

const char* telex_lookup_multi(const char *start,
			       const size_t size,
			       const char *pos,
			       const int n, ...)
{
	token_type_t prefix;
	va_list args;
	int i;

	prefix = TOKEN_INVALID;
	va_start(args, n);

	for (i = 0; i < n; i++) {
		struct telex *telex;
		int err;

		telex = va_arg(args, struct telex*);

		if (telex->prefix) {
			prefix = telex->prefix->type;
		}

		err = eval_telex(telex, start, size, pos, prefix, &pos);

		if (err < 0) {
			pos = NULL;
			break;
		}
	}

	va_end(args);

	return pos;
}

struct col_expr* col_expr_clone(struct col_expr *expr)
{
	struct col_expr *clone;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if ((expr->pound && !(clone->pound = token_clone(expr->pound))) ||
		    (expr->integer && !(clone->integer = token_clone(expr->integer)))) {
			col_expr_free(&clone);
		}
	}

	return clone;
}

void col_expr_free(struct col_expr **expr)
{
	if (expr && *expr) {
		if ((*expr)->pound) {
			token_free(&(*expr)->pound);
		}
		if ((*expr)->integer) {
			token_free(&(*expr)->integer);
		}
		free(*expr);
		*expr = NULL;
	}
}

struct line_expr* line_expr_clone(struct line_expr *expr)
{
	struct line_expr *clone;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if ((expr->colon &&
		     !(clone->colon = token_clone(expr->colon))) ||
		    (expr->integer &&
		     !(clone->integer = token_clone(expr->integer)))) {
			line_expr_free(&clone);
		}
	}

	return clone;
}

void line_expr_free(struct line_expr **expr)
{
	if (expr && *expr) {
		if ((*expr)->colon) {
			token_free(&(*expr)->colon);
		}
		if ((*expr)->integer) {
			token_free(&(*expr)->integer);
		}

		free(*expr);
		*expr = NULL;
	}
}

struct stringy* stringy_clone(struct stringy *stringy)
{
	struct stringy *clone;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if (stringy->token &&
		    !(clone->token = token_clone(stringy->token))) {
			stringy_free(&clone);
		}
	}

	return clone;
}

void stringy_free(struct stringy **stringy)
{
	if (stringy && *stringy) {
		if ((*stringy)->token) {
			token_free(&(*stringy)->token);
		}

		free(*stringy);
		*stringy = NULL;
	}
}

struct primary_expr* primary_expr_nested_new(struct token *lparen,
					     struct telex *telex,
					     struct token *rparen)
{
	struct primary_expr *expr;

	if ((expr = calloc(1, sizeof(*expr)))) {
		expr->lparen = lparen;
		expr->telex = telex;
		expr->rparen = rparen;
	}

	return expr;
}

struct primary_expr* primary_expr_clone(struct primary_expr *expr)
{
	struct primary_expr *clone;
	int error;

	error = 1;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if (expr->stringy &&
		    !(clone->stringy = stringy_clone(expr->stringy))) {
			goto cleanup;
		}

		if (expr->line_expr &&
		    !(clone->line_expr = line_expr_clone(expr->line_expr))) {
			goto cleanup;
		}

		if (expr->col_expr &&
		    !(clone->col_expr = col_expr_clone(expr->col_expr))) {
			goto cleanup;
		}

		if (expr->lparen &&
		    !(clone->lparen = token_clone(expr->lparen))) {
			goto cleanup;
		}

		if (expr->telex &&
		    !(clone->telex = telex_clone(expr->telex))) {
			goto cleanup;
		}

		if (expr->rparen &&
		    !(clone->rparen = token_clone(expr->rparen))) {
			goto cleanup;
		}
		error = 0;
	}

cleanup:
	if (error) {
		primary_expr_free(&clone);
	}

	return clone;
}

void primary_expr_free(struct primary_expr **expr)
{
	if (expr && *expr) {
		if ((*expr)->stringy) {
			stringy_free(&(*expr)->stringy);
		}

		if ((*expr)->line_expr) {
			line_expr_free(&(*expr)->line_expr);
		}

		if ((*expr)->col_expr) {
			col_expr_free(&(*expr)->col_expr);
		}

		if ((*expr)->lparen) {
			token_free(&(*expr)->lparen);
		}
		if ((*expr)->telex) {
			telex_free(&(*expr)->telex);
		}
		if ((*expr)->rparen) {
			token_free(&(*expr)->rparen);
		}

		free(*expr);
		*expr = NULL;
	}
}

struct or_expr* or_expr_new(struct or_expr *or_expr,
			    struct token *or,
			    struct primary_expr *primary_expr)
{
	struct or_expr *expr;

	if ((expr = calloc(1, sizeof(*expr)))) {
		expr->or_expr = or_expr;
		expr->or = or;
		expr->primary_expr = primary_expr;
	}

	return expr;
}

struct or_expr* or_expr_clone(struct or_expr *expr)
{
	struct or_expr *clone;
	int error;

	error = 1;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if (expr->or_expr &&
		    !(clone->or_expr = or_expr_clone(expr->or_expr))) {
			goto cleanup;
		}

		if (expr->primary_expr &&
		    !(clone->primary_expr = primary_expr_clone(expr->primary_expr))) {
			goto cleanup;
		}

		if (expr->or &&
		    !(clone->or = token_clone(expr->or))) {
			goto cleanup;
		}
		error = 0;
	}

cleanup:
	if (error) {
		or_expr_free(&clone);
	}

	return clone;
}

void or_expr_free(struct or_expr **expr)
{
	if (expr && *expr) {
		if ((*expr)->or_expr) {
			or_expr_free(&(*expr)->or_expr);
		}

		if ((*expr)->or) {
			token_free(&(*expr)->or);
		}

		if ((*expr)->primary_expr) {
			primary_expr_free(&(*expr)->primary_expr);
		}

		free(*expr);
		*expr = NULL;
	}
}

struct compound_expr* compound_expr_new(struct compound_expr *compound_expr,
					struct token *prefix,
					struct or_expr *or_expr)
{
	struct compound_expr *expr;

	if ((expr = calloc(1, sizeof(*expr)))) {
		expr->compound_expr = compound_expr;
		expr->prefix = prefix;
		expr->or_expr = or_expr;
	}

	return expr;
}

struct compound_expr* compound_expr_clone(struct compound_expr *expr)
{
	struct compound_expr *clone;
	int error;

	error = 1;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if (expr->compound_expr &&
		    !(clone->compound_expr = compound_expr_clone(expr->compound_expr))) {
			goto cleanup;
		}

		if (expr->or_expr &&
		    !(clone->or_expr = or_expr_clone(expr->or_expr))) {
			goto cleanup;
		}

		if (expr->prefix &&
		    !(clone->prefix = token_clone(expr->prefix))) {
			goto cleanup;
		}

		error = 0;
	}

cleanup:
	if (error) {
		compound_expr_free(&clone);
	}

	return clone;
}

void compound_expr_free(struct compound_expr **expr)
{
	if (expr && *expr) {
		if ((*expr)->compound_expr) {
			compound_expr_free(&(*expr)->compound_expr);
		}

		if ((*expr)->prefix) {
			token_free(&(*expr)->prefix);
		}

		if ((*expr)->or_expr) {
			or_expr_free(&(*expr)->or_expr);
		}

		free(*expr);
		*expr = NULL;
	}
}

struct telex* telex_clone(const struct telex *telex)
{
	struct telex *clone;

	if ((clone = calloc(1, sizeof(*clone)))) {
		if ((telex->prefix &&
		     !(clone->prefix = token_clone(telex->prefix))) ||
		    ((telex->compound_expr &&
		      !(clone->compound_expr = compound_expr_clone(telex->compound_expr))))) {
			telex_free(&clone);
		}
	}

	return clone;
}

void telex_free(struct telex **telex)
{
	if (telex && *telex) {
		if ((*telex)->compound_expr) {
			compound_expr_free(&(*telex)->compound_expr);
		}

		if ((*telex)->prefix) {
			token_free(&(*telex)->prefix);
		}

		free(*telex);
		*telex = NULL;
	}
}

int telex_is_relative(const struct telex *telex)
{
	if (!telex) {
		return -EINVAL;
	}

	return telex->prefix != NULL;
}
