#ifndef SIGALRM_H
#define SIGALRM_H
#include <stddef.h>

typedef struct AlrmSet {
	const size_t interval;
	const void (*run)();
	const char *name;
} AlrmSet;

extern const AlrmSet alrms[];

static void sigalrm(int unused);
#endif