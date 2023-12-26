/*
 * token.c - This file is part of libtelex
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

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <telex/error.h>
#include "error.h"
#include "token.h"

struct token {
	struct token *next;

	token_type_t type;
	const char *lexeme;
	size_t lexeme_len;
	int line;
	int col;
};

static const char *_token_names[] = {
	"TOKEN_INVALID",
	"TOKEN_NEWLINE",
	"TOKEN_SPACE",
	"TOKEN_TAB",
	"TOKEN_STRING",
	"TOKEN_REGEX",
	"TOKEN_INTEGER",
	"TOKEN_LPAREN",
	"TOKEN_RPAREN",
	"TOKEN_LESS",
	"TOKEN_DLESS",
	"TOKEN_GREATER",
	"TOKEN_DGREATER",
	"TOKEN_COLON",
	"TOKEN_POUND",
	"TOKEN_OR",
	"TOKEN_EOF",
	"TOKEN_ANY",
	NULL
};

const char* token_type_str(token_type_t type)
{
	return _token_names[type];
}

static token_type_t _token_identify_regex(const char *start, const char **end);
static token_type_t _token_identify_string(const char *start, const char **end);
static token_type_t _token_identify_string_escape(const char *start, const char **end);
static token_type_t _token_identify_integer(const char *start, const char **end);
static token_type_t _token_identify_less(const char *start, const char **end);
static token_type_t _token_identify_greater(const char *start, const char **end);
static token_type_t _token_identify(const char *start, const char **end);

static token_type_t _token_identify_regex(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_INVALID;
	*end = NULL;

	switch (head) {
	case '\'':
		*end = start;
		type = TOKEN_REGEX;
		break;

	default:
		if (head) {
			type = _token_identify_regex(tail, end);
		}
		break;
	}

	return type;
}

static token_type_t _token_identify_string(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_INVALID;

	switch (head) {
	case '\\':
		type = _token_identify_string_escape(tail, end);
		break;

	case '"':
		*end = tail;
		type = TOKEN_STRING;
		break;

	default:
		if (head) {
			type = _token_identify_string(tail, end);
		}
		break;
	}

	return type;
}

static token_type_t _token_identify_string_escape(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_INVALID;

	if (head) {
		type = _token_identify_string(tail, end);
	}

	return type;
}

static token_type_t _token_identify_integer(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_INVALID;

	if (head >= '0' && head <= '9') {
		type = _token_identify_integer(tail, end);
	} else {
		*end = start;
		type = TOKEN_INTEGER;
	}

	return type;
}

static token_type_t _token_identify_less(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_LESS;
	*end = start;

	if (head == '<') {
		type = TOKEN_DLESS;
		*end = tail;
	}

	return type;
}

static token_type_t _token_identify_greater(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_GREATER;
	*end = start;

	if (head == '>') {
		type = TOKEN_DGREATER;
		*end = tail;
	}

	return type;
}

static const token_type_t _direct_map[] = {
	['\0'] = TOKEN_EOF,
	['\n'] = TOKEN_NEWLINE,
	['\t'] = TOKEN_TAB,
	[' '] = TOKEN_SPACE,
	[':'] = TOKEN_COLON,
	['#'] = TOKEN_POUND,
	['('] = TOKEN_LPAREN,
	[')'] = TOKEN_RPAREN,
	['|'] = TOKEN_OR
};

static token_type_t _token_identify(const char *start, const char **end)
{
	char head;
	const char *tail;
	token_type_t type;

	head = *start;
	tail = start + 1;
	type = TOKEN_INVALID;
	*end = NULL;

	switch (head) {
	case '\n':
	case '\t':
	case ' ':
	case ':':
	case '#':
	case '(':
	case ')':
	case '|':
		*end = tail;
		/* fall through */
	case '\0':
		type = _direct_map[(int)head];
		break;

	case '"':
		type = _token_identify_string(tail, end);
		break;

	case '\'':
		type = _token_identify_regex(tail, end);
		break;

	case '<':
		type = _token_identify_less(tail, end);
		break;

	case '>':
		type = _token_identify_greater(tail, end);
		break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		type = _token_identify_integer(tail, end);
		break;

	default:
		type = TOKEN_INVALID;
		break;
	}

	return type;
}

static struct token* token_new(token_type_t type,
			       const char *lexeme,
			       size_t lexeme_len,
			       int line,
			       int col)
{
	struct token *token;

	if ((token = calloc(1, sizeof(*token)))) {
		token->type = type;
		token->lexeme_len = lexeme_len;
		token->line = line;
		token->col = col;

		if (!(token->lexeme = strndup(lexeme, lexeme_len))) {
			free(token);
			token = NULL;
		}
	}

	return token;
}

void token_free_all(struct token *token_list)
{
	while (token_list) {
		struct token *next;

		next = token_list->next;
		free(token_list);
		token_list = next;
	}
}

struct token* tokenize(const char *input, struct telex_error **error)
{
	struct token *tokens;
	struct token **last;
	const char *cur;
	int line;
	int col;

	tokens = NULL;
	last = &tokens;
	cur = input;
	line = 1;
	col = 1;

	while (cur) {
		token_type_t type;
		const char *next;
		int len;

		next = NULL;
		type = _token_identify(cur, &next);

		if (type == TOKEN_INVALID) {
			if (next) {
				*error = telex_error_new(line, col, "Could not recognize token: `%s'\n",
							 next);
			} else {
				*error = telex_error_new(line, col, "Could not recognize token");
			}
			token_free_all(tokens);
			tokens = NULL;
			break;
		}

		if (next) {
			len = (ptrdiff_t)next - (ptrdiff_t)cur;
		} else {
			len = strlen(cur);
		}

		if (type == TOKEN_NEWLINE) {
			line++;
			col = 1;
		}

		if (!(*last = token_new(type, cur, len, line, col))) {
			token_free_all(tokens);
			tokens = NULL;
			break;
		}

		col += len;
		last = &(*last)->next;
		cur = next;
	}

	return tokens;
}

void debug_token_list(struct token *list)
{
	printf("Tokens in list %p:\n", list);

	while (list) {
		printf("%s: %s\n", token_type_str(list->type), list->lexeme);
		list = list->next;
	}

	printf("\n");
}

const char* token_to_string(struct token *token)
{
	return token ? token->lexeme : "(null)";
}

struct token* next_relevant_token(struct token **tokens)
{
	struct token *next;

	next = tokens ? *tokens : NULL;

	/* newlines, spaces, and tabs are not significant and can be skipped */
	while (next && (next->type == TOKEN_NEWLINE ||
			next->type == TOKEN_SPACE ||
			next->type == TOKEN_TAB)) {
		next = next->next;
	}

	return next;
}

int have_token(struct token **token_list, ...)
{
	va_list args;
	struct token *token;
	token_type_t expected;
	int matches;

	token = next_relevant_token(token_list);
	matches = 0;

	if (!token) {
		return 0;
	}

	va_start(args, token_list);

	while ((expected = va_arg(args, token_type_t)) > TOKEN_INVALID) {
		if (token->type == expected) {
			matches = 1;
			break;
		}
	}

	va_end(args);

	return matches;
}

struct token* get_token(struct token **token_list, ...)
{
	va_list args;
	struct token *token;
	token_type_t expected;
	int matches;

	token = next_relevant_token(token_list);
	matches = 0;

	if (!token) {
		return NULL;
	}

	va_start(args, token_list);

	while ((expected = va_arg(args, token_type_t)) > TOKEN_INVALID) {
		if (token->type == expected || expected == TOKEN_ANY) {
			matches = 1;
			break;
		}
	}

	va_end(args);

	if (matches) {
		*token_list = token->next;
		return token;
	}

	return NULL;
}
