/*
 * token.h - This file is part of libtelex
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

#ifndef TOKEN_H
#define TOKEN_H

#include <telex/error.h>

typedef enum {
	TOKEN_INVALID = 0,
	TOKEN_NEWLINE,
	TOKEN_SPACE,
	TOKEN_TAB,
	TOKEN_STRING,
	TOKEN_REGEX,
	TOKEN_INTEGER,
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_LESS,
	TOKEN_DLESS,
	TOKEN_GREATER,
	TOKEN_DGREATER,
	TOKEN_COLON,
	TOKEN_POUND,
	TOKEN_OR,
	TOKEN_EOF,
	TOKEN_ANY
} token_type_t;

struct token;

const char* token_type_str(token_type_t type);

struct token* tokenize(const char *input, struct telex_error **error);
void token_free_all(struct token *token_list);
const char *token_to_string(struct token *token);
struct token* get_token(struct token **token_list, ...);
int have_token(struct token **token_list, ...);
void debug_token_list(struct token *token_list);

#endif /* TOKEN_H */
