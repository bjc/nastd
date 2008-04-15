#include "conf.h"
#include "array.h"
#include "log.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

string_t *
string_new(int slen, char *strdata)
{
	string_t *tmp;

	tmp = malloc(sizeof(string_t));
	if (tmp == NULL) {
		log_err("Couldn't allocate data for string: %s.",
			strerror(errno));
		return NULL;
	}

	tmp->str = malloc(slen * sizeof(char *));
	if (tmp->str == NULL) {
		log_err("Couldn't allocate string data: %s.",
			strerror(errno));
		return NULL;
	}
	memcpy(tmp->str, strdata, slen);
	tmp->strlen = slen;

	return tmp;
}

void
string_delete(string_t *string)
{
	if (string == NULL)
		return;

	if (string->str != NULL)
		free(string->str);
	string->strlen = 0;
	string->str = NULL;
	free(string);
}

array_t *
array_new()
{
	array_t *tmp;

	tmp = malloc(sizeof(array_t));
	if (tmp == NULL)
		return NULL;

	tmp->nitems = 0;
	tmp->items = NULL;
	return tmp;
}

void
array_delete(array_t *aa)
{
	int i;

	if (aa == NULL)
		return;

	for (i = 0; i < aa->nitems; i++)
		string_delete(aa->items[i]);
	free(aa->items);
	aa->items = NULL;
	free(aa);
}

int
va_array_add(array_t *aa, va_list ap)
{
	const int GRANULARITY = 10;
	int slen;
	char *s;

	slen = va_arg(ap, int);
	if (slen != ARRTERM)
		s = va_arg(ap, char *);
	else
		s = NULL;
	while (s) {
		if (aa->nitems % GRANULARITY == 0) {
			aa->items = realloc(aa->items, sizeof(string_t *) *
					    (GRANULARITY + aa->nitems));
			if (aa->items == NULL)
				return -1;
		}
		aa->nitems++;
		aa->items[aa->nitems-1] = string_new(slen, s);
		if (aa->items[aa->nitems-1] == NULL)
			return -1;
		slen = va_arg(ap, int);
		if (slen != ARRTERM)
			s = va_arg(ap, char *);
		else
			s = NULL;
	}

	return 0;
}

int
array_add(array_t *aa, ...)
{
	va_list ap;
	int rc;

	va_start(ap, aa);
	rc = va_array_add(aa, ap);
	va_end(ap);

	return rc;
}

int
array_dup(array_t *dst, array_t *src)
{
	int i;

	if (dst == NULL)
		return -1;

	dst->nitems = src->nitems;
	dst->items = malloc(dst->nitems * sizeof(string_t *));
	if (dst->items == NULL) {
		log_err("Couldn't allocate dup array items list: %s.",
			strerror(errno));
		return -1;
	}

	for (i = 0; i < dst->nitems; i++) {
		dst->items[i] = string_new(src->items[i]->strlen,
					   src->items[i]->str);
		if (dst->items[i] == NULL) {
			dst->nitems = i;
			return -1;
		}
	}

	return 0;
}
