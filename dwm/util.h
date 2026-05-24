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

void die(const char *fmt, ...);
void *ecalloc(size_t nmemb, size_t size);
#endif
