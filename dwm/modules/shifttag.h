#ifndef SHIFTTAG_H
#define SHIFTTAG_H

#include "./dwm.h"
#include "./shift.h"

static
void
shifttag(const Arg *arg)
{
	Arg shifted = shift(arg, 0);
	tag(&shifted);
}

#endif //SHIFTTAG_H
