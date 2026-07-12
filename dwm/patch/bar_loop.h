#ifndef BAR_LOOP_H
#define BAR_LOOP_H
#include <stddef.h>

typedef struct BarUpdateSet {
	const size_t interval;
	void (*run)(void);
	int* dirty;
	const char *name;
} BarUpdateSet;

static void* init_bar_loop(void* dpy);
#endif