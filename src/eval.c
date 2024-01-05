#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "telex.h"

int eval_telex(struct telex *telex, const char *start, const size_t size,
               const char *pos, token_type_t prefix, const char **result);

static const char* rstrstr(const char *haystack, const char *pos, const char *needle, const size_t len)
{
	/* FIXME: Write a faster implementation. This is the slowest possible one. */

	while (pos >= haystack) {
		if (strncmp(pos, needle, len) == 0) {
			return pos + len;
		}

		pos--;
	}

	return NULL;
}

int eval_string(struct token *string, const char *start, const size_t size,
		const char *pos, token_type_t prefix, const char **result)
{
	const char *new_pos;

	if (!string || !start || !pos || !result) {
		return -EINVAL;
	}

	if (prefix == TOKEN_LESS || prefix == TOKEN_DLESS) {
		new_pos = rstrstr(start, pos, string->lexeme, string->lexeme_len);

		if (new_pos && prefix == TOKEN_DLESS) {
			new_pos -= string->lexeme_len;
		}
	} else {
		new_pos = strstr(pos, string->lexeme);

		if (new_pos && prefix == TOKEN_DGREATER) {
			new_pos += string->lexeme_len;
		}
	}

	if (!new_pos) {
		return -ENOENT;
	}

	*result = new_pos;
	return 0;
}

int eval_regex(struct token *regex, const char *start, const size_t size,
	       const char *pos, token_type_t prefix, const char **result)
{
	if (!regex || !start || !pos || !result) {
		return -EINVAL;
	}

	return -ENOSYS;
}

static const char* find_char(const char *start,
                             const char *pos,
                             int dir,
                             int chr)
{
	if (dir > 0) {
		return strchr(pos, chr);
	}

	while (pos >= start) {
		if (*pos == chr) {
			return pos;
		}

		pos--;
	}

	return NULL;
}

int eval_line_expr(struct line_expr *expr, const char *start, const size_t size,
		   const char *pos, token_type_t prefix, const char **result)
{
	long long steps;
	int dir;

	if (!expr || !start || !pos || !result) {
		return -EINVAL;
	}

	if (!expr->integer) {
		return -EBADFD;
	}

	steps = expr->integer->integer;
	dir = (prefix == TOKEN_LESS || prefix == TOKEN_DLESS) ? -1 : +1;

	if (prefix == TOKEN_DLESS || prefix == TOKEN_DGREATER) {
		steps++;
	} else if (prefix == TOKEN_INVALID) {
		/* when making absolute movements, :1 is the first line, not :0 */
		steps--;
	}

	if (dir < 0) {
		while (steps--) {
			const char *new_pos;

			if (*pos == '\n' && --pos < start) {
				*result = start;
				return 0;
			}

			if (!(new_pos = find_char(start, pos, dir, '\n'))) {
				*result = start;
				return 0;
			}

			pos = new_pos;
		}

		if (prefix == TOKEN_DLESS) {
			pos++;
		}
	} else {
		while (steps--) {
			const char *new_pos;

			if (!(new_pos = find_char(start, pos, dir, '\n'))) {
				*result = pos;
				return 0;
			}

			pos = new_pos + 1;
		}

		if (prefix == TOKEN_DGREATER) {
			pos--;
		}
	}

	*result = pos;
	return 0;
}

int eval_col_expr(struct col_expr *expr, const char *start, const size_t size,
		  const char *pos, token_type_t prefix, const char **result)
{
	long long steps;
	int dir;

	if (!expr || !start || !pos || !result) {
		return -EINVAL;
	}

	if (!expr->integer) {
		return -EBADFD;
	}

	steps = expr->integer->integer;
	dir = (prefix == TOKEN_LESS || prefix == TOKEN_DLESS) ? -1 : +1;
	if (steps < 0) {
		dir = -dir;
		steps = -steps;
	}

	while (steps--) {
		const char *new_pos;

		new_pos = pos + dir;

		if (new_pos < start || new_pos > (start + size) || *new_pos == '\n') {
			if (*new_pos == '\n' && dir > 0) {
				pos = new_pos;
			}
			break;
		}

		pos = new_pos;
	}

	*result = pos;
	return 0;
}

int eval_stringy(struct stringy *stringy, const char *start, const size_t size,
		 const char *pos, token_type_t prefix, const char **result)
{
	if (!stringy || !start || !pos || !result) {
		return -EINVAL;
	}

	switch (stringy->token->type) {
	case TOKEN_STRING:
		return eval_string(stringy->token, start, size, pos, prefix, result);

	case TOKEN_REGEX:
		return eval_regex(stringy->token, start, size, pos, prefix, result);

	default:
		return -EBADFD;
	}
}

int eval_primary_expr(struct primary_expr *expr, const char *start, const size_t size,
		      const char *pos, token_type_t prefix, const char **result)
{
	if (!expr || !start || !pos || !result) {
		return -EINVAL;
	}

	if (expr->stringy) {
		return eval_stringy(expr->stringy, start, size, pos, prefix, result);
	}

	if (expr->line_expr) {
		return eval_line_expr(expr->line_expr, start, size, pos, prefix, result);
	}

	if (expr->col_expr) {
		return eval_col_expr(expr->col_expr, start, size, pos, prefix, result);
	}

	if (expr->telex) {
		return eval_telex(expr->telex, start, size, pos, prefix, result);
	}

	return -EBADFD;
}

int eval_or_expr(struct or_expr *expr, const char *start, const size_t size,
		 const char *pos, token_type_t prefix, const char **result)
{
	if (!expr || !start || !pos || !result) {
		return -EINVAL;
	}

	if (expr->or_expr) {
		int err;

		err = eval_or_expr(expr->or_expr, start, size, pos, prefix, result);

		if (err >= 0) {
			return err;
		}
	}

	return eval_primary_expr(expr->primary_expr, start, size, pos, prefix, result);
}

int eval_compound_expr(struct compound_expr *expr, const char *start, const size_t size,
                       const char *pos, token_type_t prefix, const char **result)
{
	token_type_t effective_prefix;

	if (!expr || !start || !pos || !result) {
		return -EINVAL;
	}

	if (expr->compound_expr) {
		int err;

		err = eval_compound_expr(expr->compound_expr, start, size, pos, prefix, &pos);

		if (err < 0) {
			return err;
		}
	}

	effective_prefix = expr->prefix ? expr->prefix->type : prefix;

	return eval_or_expr(expr->or_expr, start, size, pos, effective_prefix, result);
}

int eval_telex(struct telex *telex, const char *start, const size_t size,
               const char *pos, token_type_t prefix, const char **result)
{
	token_type_t effective_prefix;

	if (!telex || !start || !result) {
		return -EINVAL;
	}

	if (telex->prefix && !pos) {
		return -EBADMSG;
	}

	if (!pos) {
		pos = start;
	}

	effective_prefix = telex->prefix ? telex->prefix->type : prefix;

	return eval_compound_expr(telex->compound_expr, start, size, pos, effective_prefix, result);
}
