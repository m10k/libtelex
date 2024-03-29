/*
 * telex/error.h - This file is part of libtelex
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

#ifndef TELEX_ERROR_H
#define TELEX_ERROR_H

struct telex_error;

const char *telex_error_get_message(struct telex_error *error);
int telex_error_get_line(struct telex_error *error);
int telex_error_get_col(struct telex_error *error);
struct telex_error* telex_error_get_next(struct telex_error *error);

void telex_error_free(struct telex_error **error);
void telex_error_free_all(struct telex_error **errors);

#endif /* TELEX_ERROR_H */
