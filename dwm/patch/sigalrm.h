#ifndef SIGALRM_H
#define SIGALRM_H
#include <stddef.h>

typedef struct AlrmSet {
	const size_t interval;
	void (*run)(void);
	const char *name;
} AlrmSet;

static void sigalrm(int unused);
#endif