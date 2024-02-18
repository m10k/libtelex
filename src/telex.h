/*
 * telex.h - This file is part of libtelex
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

#ifndef TELEX_H
#define TELEX_H

#include <telex/telex.h>
#include "token.h"

struct telex;

struct col_expr {
	struct token *pound;
	struct token *integer;
};

struct col_expr* col_expr_clone(struct col_expr *expr);
void col_expr_free(struct col_expr **expr);

struct line_expr {
	struct token *colon;
	struct token *integer;
};

struct line_expr* line_expr_clone(struct line_expr *expr);
void line_expr_free(struct line_expr **expr);

struct stringy {
	struct token *token;
};

struct stringy* stringy_clone(struct stringy *stringy);
void stringy_free(struct stringy **stringy);

struct primary_expr {
	struct stringy *stringy;
	struct line_expr *line_expr;
	struct col_expr *col_expr;

	struct token *lparen;
	struct telex *telex;
	struct token *rparen;
};

struct primary_expr* primary_expr_nested_new(struct token *lparen,
					     struct telex *telex,
					     struct token *rparen);
struct primary_expr* primary_expr_clone(struct primary_expr *expr);
void primary_expr_free(struct primary_expr **expr);

struct or_expr {
	struct or_expr *or_expr;
	struct token *or;
	struct primary_expr *primary_expr;
};

struct or_expr* or_expr_new(struct or_expr *or_expr,
			    struct token *or,
			    struct primary_expr *primary_expr);
struct or_expr* or_expr_clone(struct or_expr *expr);
void or_expr_free(struct or_expr **expr);

struct compound_expr {
	struct compound_expr *compound_expr;
	struct token *prefix;
	struct or_expr *or_expr;
};

struct compound_expr* compound_expr_new(struct compound_expr *compound_expr,
					struct token *prefix,
					struct or_expr *or_expr);
struct compound_expr* compound_expr_clone(struct compound_expr *expr);
void compound_expr_free(struct compound_expr **expr);

struct telex {
	struct token *prefix;
	struct compound_expr *compound_expr;
};

struct telex* telex_new(struct token *prefix,
			struct compound_expr *compound_expr);

#endif /* TELEX_H */
