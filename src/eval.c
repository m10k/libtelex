#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "telex.h"

struct match {
	const char *start;
	const char *end;
};

int eval_telex(struct telex *telex, const char *start, const size_t size,
               const char *pos, token_type_t prefix, const char **result);

static const char* rstrstr(const char *haystack, const char *pos, const char *needle, const size_t len)
{
	/* FIXME: Write a faster implementation. This is the slowest possible one. */

	while (pos >= haystack) {
		if (strncmp(pos, needle, len) == 0) {
			return pos;
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
	} else {
		new_pos = strstr(pos, string->lexeme);
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
	if (steps < 0) {
		dir = -dir;
		steps = -steps;
	}

	while (steps--) {
		const char *new_pos;

		new_pos = find_char(start, pos, dir, '\n');

		if (!new_pos) {
			if (dir < 0) {
				*result = start;
			} else {
				*result = pos;
			}
			return 0;
		}

		pos = new_pos + 1;
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

	if (expr->prefix) {
		prefix = expr->prefix->type;
	}

	return eval_or_expr(expr->or_expr, start, size, pos, prefix, result);
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

	if (prefix == TOKEN_INVALID) {
		effective_prefix = telex->prefix ? telex->prefix->type : TOKEN_GREATER;
	} else {
		effective_prefix = prefix;
	}

	return eval_compound_expr(telex->compound_expr, start, size, pos, effective_prefix, result);
}
