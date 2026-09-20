#ifndef RESTARTSIG_H
#define RESTARTSIG_H

#include "./dwm.h"

static int restart = 0;

static void
sighup(int unused)
{
	Arg a = {.i = 1};
	quit(&a);
}

static void
sigterm(int unused)
{
	Arg a = {.i = 0};
	quit(&a);
}


#endif //RESTARTSIG_H
