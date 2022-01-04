/* See LICENSE file for copyright and license details. */
#include "common.h"


void *
erealloc(void *ptr, size_t n)
{
	void *ret = realloc(ptr, n);
	if (!ret)
		eprintf("realloc %zu:", n);
	return ret;
}


void *
ecalloc(size_t n, size_t m)
{
	void *ret = calloc(n, m);
	if (!ret)
		eprintf("calloc %zu %zu:", n, m);
	return ret;
}


void *
emalloc(size_t n)
{
	void *ret = malloc(n);
	if (!ret)
		eprintf("malloc %zu:", n);
	return ret;
}


void *
ememdup(const void *ptr, size_t n)
{
	void *ret = emalloc(n);
	memcpy(ret, ptr, n);
	return ret;
}


void
eprintf(const char *fmt, ...)
{
	va_list ap;
	int err = errno;
	char end = *fmt ? strchr(fmt, '\0')[-1] : '\0';
	va_start(ap, fmt);
	fprintf(stderr, "%s: ", argv0);
	vfprintf(stderr, fmt, ap);
	if (end == '\0')
		fprintf(stderr, "%s\n", strerror(err));
	else if (end == ':')
		fprintf(stderr, " %s\n", strerror(err));
	else if (end != '\n')
		fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_ERROR);
}
