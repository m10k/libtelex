/*
 * error.h - This file is part of libtelex
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

#ifndef ERROR_H
#define ERROR_H

#include <telex/error.h>

struct telex_error* telex_error_new(int line, int col, const char *fmt, ...);

#endif /* ERROR_H */
