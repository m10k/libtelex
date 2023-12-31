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
					next->lexeme);			\
		parser_add_error(context, error);			\
	} while (0);

struct telex* parse_telex(struct token **tokens, struct parser *context);
void debug_telex(struct telex *telex, int depth);

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

	expr->pound = get_token(tokens, TOKEN_POUND, 0);
	if (!(expr->integer = get_token(tokens, TOKEN_INTEGER, 0))) {
		EXPECTED_GRAMMAR("integer", tokens);
		col_expr_free(&expr);
	}

	return expr;
}

struct line_expr* parse_line_expr(struct token **tokens, struct parser *context)
{
	struct line_expr *expr;
	int error;

	/*
	 * line_expr = ':' integer
	 */

	assert(tokens);
	assert(*tokens);

	error = 0;

	if (!(expr = calloc(1, sizeof(*expr)))) {
		error = -ENOMEM;
	} else if (!(expr->colon = get_token(tokens, TOKEN_COLON, 0))) {
		EXPECTED_GRAMMAR("colon", tokens);
		error = -EBADMSG;
	} else if (!(expr->integer = get_token(tokens, TOKEN_INTEGER, 0))) {
		EXPECTED_GRAMMAR("integer", tokens);
		error = -EBADMSG;
	}

	if (error) {
	        line_expr_free(&expr);
	}

	return expr;
}

struct stringy* parse_stringy(struct token **tokens, struct parser *context)
{
	struct stringy *stringy;

	assert(tokens);
	assert(*tokens);

	if ((stringy = calloc(1, sizeof(*stringy)))) {
		if (!(stringy->token = get_token(tokens, TOKEN_STRING, TOKEN_REGEX, 0))) {
			EXPECTED_GRAMMAR("string or regex", *tokens);
			stringy_free(&stringy);
		}
	}

	return stringy;
}

struct primary_expr* parse_primary_expr(struct token **tokens, struct parser *context)
{
	struct primary_expr *expr;
	int error;

	assert(tokens);
	assert(*tokens);

	if (!(expr = calloc(1, sizeof(*expr)))) {
		return NULL;
	}
	error = 0;

	if (have_token(tokens, TOKEN_STRING, TOKEN_REGEX, 0)) {
		expr->stringy = parse_stringy(tokens, context);
	} else if(have_token(tokens, TOKEN_COLON, 0)) {
		expr->line_expr = parse_line_expr(tokens, context);
	} else if(have_token(tokens, TOKEN_POUND, TOKEN_INTEGER, 0)) {
		expr->col_expr = parse_col_expr(tokens, context);
	} else if(have_token(tokens, TOKEN_LPAREN, 0)) {
		expr->lparen = get_token(tokens, TOKEN_LPAREN, 0);

		if (!(expr->telex = parse_telex(tokens, context))) {
			EXPECTED_GRAMMAR("telex", tokens);
			error = -EBADMSG;
		} else if (!(expr->rparen = get_token(tokens, TOKEN_RPAREN, 0))) {
			EXPECTED_GRAMMAR("`)'", tokens);
			error = -EBADMSG;
		}
	} else {
		EXPECTED_GRAMMAR("match, line, or column expression, or nested telex", tokens);
		error = -EBADMSG;
	}

	if (error) {
		primary_expr_free(&expr);
	}

	return expr;
}

struct or_expr* parse_or_expr(struct token **tokens, struct parser *context)
{
	struct or_expr *top;
	struct token *or;
	int error;

	/*
	 * or-expr = or-expr '|' primary-expr
	 *         | primary-expr
	 */

	assert(tokens);
	assert(*tokens);

	top = NULL;
	or = NULL;
	error = 0;

	do {
		struct or_expr *expr;

		expr = calloc(1, sizeof(*expr));
		assert(expr);

		if (!(expr->primary_expr = parse_primary_expr(tokens, context))) {
			EXPECTED_GRAMMAR("primary-expr", tokens);
			error = -EBADMSG;
			break;
		}

		expr->or = or;
		expr->or_expr = top;

		top = expr;
		or = get_token(tokens, TOKEN_OR, 0);
	} while (or);

	if (error && top) {
		or_expr_free(&top);
	}

	return top;
}

struct compound_expr* parse_compound_expr(struct token **tokens, struct parser *context)
{
	struct compound_expr *top;
	struct token *prefix;
	int error;

	/*
	 * compound-expr = compound-expr prefix or-expr
	 *               | or-expr
	 */

	assert(tokens);
	assert(*tokens);

	top = NULL;
	prefix = NULL;
	error = 0;

	do {
	        struct compound_expr *expr;

		if (!(expr = calloc(1, sizeof(*expr)))) {
			error = -ENOMEM;
			break;
		}

		if (!(expr->or_expr = parse_or_expr(tokens, context))) {
			EXPECTED_GRAMMAR("or-expr", tokens);
			error = -EBADMSG;
			break;
		}

		expr->prefix = prefix;
		expr->compound_expr = top;

		top = expr;
		prefix = parse_prefix(tokens, context);
	} while (prefix);

	if (error && top) {
		compound_expr_free(&top);
	}

	return top;
}

struct telex* parse_telex(struct token **tokens, struct parser *context)
{
	struct telex *telex;

	/*
	 * telex = prefix compound-expr
	 *       | compound-expr
	 */

	assert(tokens);
	assert(*tokens);

	if ((telex = calloc(1, sizeof(*telex)))) {
		/* no error checking because prefix is optional */
		telex->prefix = parse_prefix(tokens, context);

		if (!(telex->compound_expr = parse_compound_expr(tokens, context))) {
			EXPECTED_GRAMMAR("compound_expr", *tokens);
			telex_free(&telex);
		}
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

	fprintf(stderr, "%*sprimary_expr [ %p %p %p %p ]\n", depth, "",
		(void*)expr->stringy,
		(void*)expr->line_expr,
		(void*)expr->col_expr,
		(void*)expr->telex);

	debug_stringy(expr->stringy, depth + 1);
	debug_line_expr(expr->line_expr, depth + 1);
	debug_col_expr(expr->col_expr, depth + 1);
	debug_telex(expr->telex, depth + 1);
}

void debug_or_expr(struct or_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	if (expr->or) {
		fprintf(stderr, "%*sor_expr [ %p %s %p ]\n", depth, "",
			(void*)expr->or_expr, expr->or->lexeme, (void*)expr->primary_expr);
	} else {
		fprintf(stderr, "%*sor_expr [ %p ]\n", depth, "",
			(void*)expr->primary_expr);
	}

	debug_or_expr(expr->or_expr, depth + 1);
	debug_primary_expr(expr->primary_expr, depth + 1);
}

void debug_compound_expr(struct compound_expr *expr, int depth)
{
	if (!expr) {
		return;
	}

	if (expr->prefix) {
		fprintf(stderr, "%*scompound_expr [ %p %s %p ]\n", depth, "",
			(void*)expr->compound_expr,
			expr->prefix->lexeme,
			(void*)expr->or_expr);
	} else {
		fprintf(stderr, "%*scompound_expr [ %p ]\n", depth, "",
		        (void*)expr->or_expr);
	}

	debug_compound_expr(expr->compound_expr, depth + 1);
	debug_or_expr(expr->or_expr, depth + 1);
}

void debug_telex(struct telex *telex, int depth)
{
	if (!telex) {
		return;
	}

	fprintf(stderr, "%*stelex [ %s, %p ]\n", depth, "",
		telex->prefix ? telex->prefix->lexeme : "(null)",
		(void*)telex->compound_expr);

	debug_compound_expr(telex->compound_expr, depth + 1);
}

void parser_debug_telex(struct telex *telex)
{
	debug_telex(telex, 0);
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

	if (!parser || !input) {
		return -EINVAL;
	}

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
