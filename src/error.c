/*
 * error.c - This file is part of libtelex
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
#include <stdlib.h>
#include <stdarg.h>
#include <telex/error.h>
#include "error.h"

struct telex_error* telex_error_new(int line, int col, const char *fmt, ...)
{
	struct telex_error *error;
	va_list args;
	int required_length;

	if (!(error = calloc(1, sizeof(*error)))) {
		perror("calloc");
		return NULL;
	}

	error->line = line;
	error->col = col;

	va_start(args, fmt);

	required_length = vsnprintf(NULL, 0, fmt,  args);

	if ((error->message = malloc(required_length + 1))) {
		vsnprintf(error->message, required_length + 1, fmt, args);
	}

	va_end(args);

	if (!error->message) {
		free(error);
		error = NULL;
	}

	return error;
}

void telex_error_free(struct telex_error **error)
{
	if ((*error)->message) {
		free((*error)->message);
	}

	free(*error);
	*error = NULL;
}

void telex_error_free_all(struct telex_error **errors)
{
	while (*errors) {
		struct telex_error *next;

		next = (*errors)->next;
		telex_error_free(errors);
		*errors = next;
	}
}

const char* telex_error_get_message(struct telex_error *error)
{
	return error->message;
}

int telex_error_get_line(struct telex_error *error)
{
	return error->line;
}

int telex_error_get_col(struct telex_error *error)
{
	return error->col;
}

struct telex_error* telex_error_get_next(struct telex_error *error)
{
	return error->next;
}

void telex_error_set_next(struct telex_error *error, struct telex_error *next)
{
	error->next = next;
}
