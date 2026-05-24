#include <stdio.h>
#include <unistd.h>

#include "sigalrm.h"
#include "../util.h"
static size_t alrmcounter;

void
sigalrm(int sig)
{
	alrmcounter++;

	int redraw = 0;
	const AlrmSet *as;
	for (size_t i = 0; i < LENGTH(alrms); i++) {
		as = &alrms[i];
		if (!sig || alrmcounter % as->interval == 0) {
			as->run();
			redraw = 1;
		}
	}
	if (redraw && sig) drawbars();

	alarm(1);
}

