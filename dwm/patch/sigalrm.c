#include <stdio.h>
#include <unistd.h>

#include "sigalrm.h"
#include "../util.h"
static size_t alrmcounter;

void
sigalrm(int unused)
{
	alrmcounter++;
	fprintf(stderr, "sigalrm: %zu\n", alrmcounter);

	const AlrmSet *as;
	for (size_t i = 0; i < LENGTH(alrms); i++) {
		
		if (alrmcounter % alrms[i]->interval == 0) {
			fprintf(stderr, "sigalrm: %s\n", alrms[i]->name);
			alrms[i]->run();
		}
	}

	alarm(1);
}

