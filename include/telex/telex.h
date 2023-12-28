/*
 * telex/telex.h - This file is part of libtelex
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

#ifndef TELEX_TELEX_H
#define TELEX_TELEX_H

#include <telex/error.h>
#include <stddef.h>

struct telex;

int telex_parse(struct telex **telex,
                const char *input,
                struct telex_error **errors);
void telex_debug(struct telex *telex);
void telex_free(struct telex *telex);

int telex_to_string(struct telex *telex, char *str, const size_t str_size);
int telex_combine(struct telex **combined, struct telex *left, struct telex *right);
int telex_clone(struct telex **new, struct telex *old);
void telex_simplify(struct telex *telex);

const char* telex_lookup(struct telex *telex, const char *start,
                         const size_t size, const char *pos);
const char* telex_lookup_multi(const char *start, const size_t size,
                               const char *pos, int n, ...);

#endif /* TELEX_TELEX_H */
