#ifndef SIGALRM_H
#define SIGALRM_H
#include <stddef.h>

typedef struct BarUpdateSet {
	const size_t interval;
	void (*run)(void);
	const char *name;
} BarUpdateSet;

static void* init_bar_loop(void* dpy);
#endif