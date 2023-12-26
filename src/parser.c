/*
 * parser.c - This file is part of libtelex
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <telex/error.h>
#include "parser.h"
#include "token.h"
#include "telex.h"
#include "error.h"

#define EXPECTED_GRAMMAR(msg, token_list) do {				\
		struct telex_error *error;				\
		struct token *next;					\
		next = next_relevant_token(tokens);			\
		error = telex_error_new(next->line, next->col,		\
					"Expected %s but found `%s'",	\
					msg,				\
					token_to_string(next));		\
		parser_add_error(context, error);			\
	} while (0);

struct token* parse_prefix(struct token **tokens, struct parser *context)
{
	return get_token(tokens, TOKEN_LESS, TOKEN_DLESS, TOKEN_GREATER, TOKEN_DGREATER, 0);
}

struct col_expr* parse_col_expr(struct token **tokens, struct parser *context)
{
	struct col_expr *expr;

	/*
	 * col_expr = '#' integer
	 *          | integer
	 */

	assert(tokens);
	assert(*tokens);

	expr = calloc(1, sizeof(*expr));
	assert(expr);

	expr->pound = get_token(tokens, TOKEN_POUND);
	if (!(expr->integer = get_token(tokens, TOKEN_INTEGER))) {
		EXPECTED_GRAMMAR("integer", tokens);
		assert(0);
	}

	return expr;
}

struct line_expr* parse_line_expr(struct token **tokens, struct parser *context)
{
	struct line_expr *expr;

	/*
	 * line_expr = ':' integer
	 */

	assert(tokens);
	assert(*tokens);

	expr = calloc(1, sizeof(*expr));
	assert(expr);

	if (!(expr->colon = get_token(tokens, TOKEN_COLON))) {
		EXPECTED_GRAMMAR("colon", tokens);
		assert(0);
	}

	if (!(expr->integer = get_token(tokens, TOKEN_INTEGER))) {
		EXPECTED_GRAMMAR("integer", tokens);
		assert(0);
	}

	return expr;
}

struct stringy* parse_stringy(struct token **tokens, struct parser *context)
{
	struct stringy *stringy;

	assert(tokens);
	assert(*tokens);

	stringy = calloc(1, sizeof(*stringy));
	assert(stringy);

        if (!(stringy->token = get_token(tokens, TOKEN_STRING, TOKEN_REGEX, 0))) {
		EXPECTED_GRAMMAR("string or regex", *tokens);
		assert(0);
	}

	return stringy;
}

struct match_expr* parse_match_expr(struct token **tokens, struct parser *context)
{
	struct match_expr *top;
	struct token *or;

	assert(tokens);
	assert(*tokens);

	top = NULL;
	or = NULL;

	do {
		struct match_expr *expr;

		expr = calloc(1, sizeof(*expr));
		assert(expr);

		if (!(expr->stringy = parse_stringy(tokens, context))) {
			EXPECTED_GRAMMAR("stringy", *tokens);
			assert(0);
		}

		expr->match_expr = top;
		expr->or = or;
		top = expr;

		or = get_token(tokens, TOKEN_OR, 0);
	} while(or);

	return top;
}

struct primary_expr* parse_primary_expr(struct token **tokens, struct parser *context)
{
	struct primary_expr *expr;

	assert(tokens);
	assert(*tokens);

	expr = calloc(1, sizeof(*expr));
	assert(expr);

	if (have_token(tokens, TOKEN_STRING, TOKEN_REGEX, 0)) {
		expr->match_expr = parse_match_expr(tokens, context);
	} else if(have_token(tokens, TOKEN_COLON, 0)) {
		expr->line_expr = parse_line_expr(tokens, context);
	} else if(have_token(tokens, TOKEN_POUND, TOKEN_INTEGER, 0)) {
		expr->col_expr = parse_col_expr(tokens, context);
	} else {
		EXPECTED_GRAMMAR("match, line, or column expression", *tokens);
		assert(0);
	}

	return expr;
}

struct subtelex_list* parse_subtelex_list(struct token **tokens, struct parser *context)
{
	struct subtelex_list *top;
	struct token *prefix;

	/*
	 * subtelex-list = subtelex-list prefix primary-telex
	 *               | primary-telex
	 */

	assert(tokens);
	assert(*tokens);

	top = NULL;
	prefix = NULL;

	do {
	        struct subtelex_list *list;

		list = calloc(1, sizeof(*list));
		assert(list);

		if (!(list->primary_expr = parse_primary_expr(tokens, context))) {
			EXPECTED_GRAMMAR("primary-expr", *tokens);
			assert(0);
		}

		list->prefix = prefix;
		list->subtelex_list = top;

		top = list;
		prefix = parse_prefix(tokens, context);
	} while (prefix);

	return top;
}

struct telex* parse_telex(struct token **tokens, struct parser *context)
{
	struct telex *telex;

	/*
	 * telex = prefix subtelex-list
	 *       | subtelex-list
	 */

	assert(tokens);
	assert(*tokens);

	telex = calloc(1, sizeof(*telex));
	assert(telex);

	/* no error checking because prefix is optional */
	telex->prefix = parse_prefix(tokens, context);

	if (!(telex->subtelex_list = parse_subtelex_list(tokens, context))) {
		EXPECTED_GRAMMAR("subtelex_list", *tokens);
		assert(0);
	}

	return telex;
}

void debug_stringy(struct stringy *expr, int depth)
{
	if (!expr) {
		return;
	}

	fprintf(stderr, "%*sstringy [ %s ]\n", depth, "",
		expr->token ? expr->token->lexeme : "(null)");
}

void debug_match_expr(struct match_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	if (expr->or) {
		fprintf(stderr, "%*smatch_expr [ %p %s %p ]\n", depth, "",
			expr->match_expr, expr->or->lexeme, expr->stringy);
	} else {
		fprintf(stderr, "%*smatch_expr [ %p ]\n", depth, "",
			expr->stringy);
	}

	debug_match_expr(expr->match_expr, depth + 1);
	debug_stringy(expr->stringy, depth + 1);
}

void debug_line_expr(struct line_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	fprintf(stderr, "%*sline_expr [ %s %s ]\n", depth, "",
		expr->colon->lexeme, expr->integer->lexeme);
}

void debug_col_expr(struct col_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	fprintf(stderr, "%*scol_expr [ %s %s ]\n", depth, "",
		expr->pound ? expr->pound->lexeme : "",
		expr->integer->lexeme);
}

void debug_primary_expr(struct primary_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	fprintf(stderr, "%*sprimary_expr [ %p %p %p ]\n", depth, "",
		expr->match_expr,
		expr->line_expr,
		expr->col_expr);

	debug_match_expr(expr->match_expr, depth + 1);
	debug_line_expr(expr->line_expr, depth + 1);
	debug_col_expr(expr->col_expr, depth + 1);
}

void debug_subtelex_list(struct subtelex_list *list, int depth)
{
	if (!list) {
		return;
	}

	if (list->prefix) {
		fprintf(stderr, "%*ssubtelex_list [ %p %s %p ]\n", depth, "",
			list->subtelex_list,
			list->prefix->lexeme,
			list->primary_expr);
	} else {
		fprintf(stderr, "%*ssubtelex_list [ %p ]\n", depth, "",
			list->primary_expr);
	}

	debug_subtelex_list(list->subtelex_list, depth + 1);
	debug_primary_expr(list->primary_expr, depth + 1);
}

void parser_debug_telex(struct telex *telex, int depth)
{
	if (!telex) {
		return;
	}

	fprintf(stderr, "%*stelex [ %s, %p ]\n", depth, "",
		telex->prefix ? telex->prefix->lexeme : "(null)",
		telex->subtelex_list);

	debug_subtelex_list(telex->subtelex_list, depth + 1);
}

struct parser* parser_new(void)
{
	return calloc(1, sizeof(struct parser));
}

void parser_free(struct parser *parser)
{
	if (parser) {
		if (parser->tokens) {
			token_free_all(parser->tokens);
			parser->tokens = NULL;
		}

		free(parser);
	}
}

int parser_parse(struct parser *parser, const char *input)
{
	struct token *tokens;

	if (parser->tokens) {
		return -EALREADY;
	}

	if (!(parser->tokens = tokenize(input, &parser->errors))) {
		return -EBADMSG;
	}

	tokens = parser->tokens;
	if (!(parser->telex = parse_telex(&tokens, parser))) {
		return -EBADMSG;
	}

	return 0;
}

void parser_add_error(struct parser *parser, struct telex_error *error)
{
	struct telex_error **last;

	for (last = &parser->errors; *last; last = &(*last)->next);
	*last = error;
}

struct telex* parser_get_telex(struct parser *parser)
{
	return parser->telex;
}

struct telex_error* parser_get_errors(struct parser *parser)
{
	return parser->errors;
}
