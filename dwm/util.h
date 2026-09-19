/* See LICENSE file for copyright and license details. */

#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

#ifndef MAX
#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#endif
#ifndef MIN
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#endif
#ifndef CLAMP
#define CLAMP(A, B, C)               (MIN((C), (MAX((A), (B)))))
#endif
#define BETWEEN(X, A, B)        ((A) <= (X) && (X) <= (B))
#define LENGTH(X)               (sizeof (X) / sizeof (X)[0])

#ifdef _DEBUG
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno;

	saved_errno = errno;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}

#endif //UTIL_H
