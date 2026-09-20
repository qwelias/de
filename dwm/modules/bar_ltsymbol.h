#ifndef BAR_LTSYMBOL_H
#define BAR_LTSYMBOL_H

#include "./dwm.h"

static double lts_width = 3;

static int
width_ltsymbol(Bar *bar, BarArg *a)
{
	return lts_width*fonth;
}

static int
draw_ltsymbol(Bar *bar, BarArg *a)
{
	int lpad = (lts_width*fonth - drw_fontset_getwidth(drw, bar->mon->ltsymbol, 0)) / 2;
	if (bar->mon->lt[bar->mon->sellt] == &speciallt) {
		drw_setscheme(drw, scheme[SchemeSel]);
	}
	return drw_text(drw, a->x, a->y, a->w, a->h, lpad, bar->mon->ltsymbol, 0, 0);
}

static int
click_ltsymbol(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkLtSymbol;
}

#endif //BAR_LTSYMBOL_H