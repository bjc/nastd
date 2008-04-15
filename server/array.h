#ifndef ARRAY_H
#define ARRAY_H

#include <stdarg.h>

#define ARRTERM -1

struct _string_t {
	char *str;
	int strlen;
};
typedef struct _string_t string_t;

struct _array_t {
	int nitems;
	string_t **items;
};
typedef struct _array_t array_t;

string_t *string_new(int slen, char *strdata);
void string_delete(string_t *string);

array_t *array_new();
void array_delete(array_t *array);
int va_array_add(array_t *aa, va_list ap);
int array_add(array_t *aa, ...);
int array_dup(array_t *dst, array_t *src);

#endif
