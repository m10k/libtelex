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
#include <errno.h>
#include "telex.h"
#include "parser.h"

int telex_parse(const char *input,
		struct telex **telex,
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
	parser_debug_telex(telex, 0);
}

void telex_free(struct telex *telex)
{
	/* FIXME: Implement me */
	return;
}
