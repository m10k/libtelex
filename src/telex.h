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

struct col_expr {
	struct token *pound;
	struct token *integer;
};

struct line_expr {
	struct token *colon;
	struct token *integer;
};

struct stringy {
	struct token *token;
};

struct match_expr {
	struct match_expr *match_expr;
	struct token *or;
	struct stringy *stringy;
};

struct primary_expr {
	struct match_expr *match_expr;
	struct line_expr *line_expr;
	struct col_expr *col_expr;
};

struct subtelex_list {
	struct subtelex_list *subtelex_list;
	struct token *prefix;
	struct primary_expr *primary_expr;
};

struct telex {
	struct token *prefix;
	struct subtelex_list *subtelex_list;
};

#endif /* TELEX_H */
