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

void telex_free(struct telex *telex)
{
	/* FIXME: Implement me */
	return;
}

int telex_to_string(struct telex *telex, char *str, const size_t str_size)
{
	return -ENOSYS;
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

int telex_clone(struct telex **new, struct telex *old)
{
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
