/*
 * parser.h - This file is part of libtelex
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

#ifndef PARSER_H
#define PARSER_H

#include <telex/error.h>
#include "token.h"
#include "telex.h"

struct parser {
	struct token *tokens;
	struct telex *telex;
	struct telex_error *errors;
};

struct parser* parser_new(void);
void parser_free(struct parser *parser);
int parser_parse(struct parser *parser, const char *input);

struct telex* parser_get_telex(struct parser *parser);
void parser_debug_telex(struct telex *telex);
struct telex_error* parser_get_errors(struct parser *parser);
void parser_add_error(struct parser *parser, struct telex_error *error);

#endif /* PARSER_H */
