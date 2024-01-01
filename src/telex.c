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

int telex_combine(struct telex **new, struct telex *first, struct telex *second)
{
	struct telex *combined;

	if (!new || !first || !second) {
		return -EINVAL;
	}

	if (!(combined = calloc(1, sizeof(*combined)))) {
		return -ENOMEM;
	}


	return -ENOSYS;
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
		clone->pound = expr->pound;
		clone->integer = expr->integer;
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
		clone->colon = expr->colon;
		clone->integer = expr->integer;
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
		clone->token = stringy->token;
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

		clone->lparen = expr->lparen;

		if (expr->telex &&
		    !(clone->telex = telex_clone(expr->telex))) {
			goto cleanup;
		}

		clone->rparen = expr->rparen;
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

		clone->or = expr->or;
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

		clone->prefix = expr->prefix;
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

struct telex* telex_clone(struct telex *telex)
{
	struct telex *clone;

	if ((clone = calloc(1, sizeof(*clone)))) {
		clone->prefix = telex->prefix;

		if (telex->compound_expr &&
		    !(clone->compound_expr = compound_expr_clone(telex->compound_expr))) {
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

int telex_is_relative(struct telex *telex)
{
	if (!telex) {
		return -EINVAL;
	}

	return telex->prefix != NULL;
}
