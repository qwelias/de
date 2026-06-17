static double lts_width = 3;

int
width_ltsymbol(Bar *bar, BarArg *a)
{
	return lts_width*fonth;
}

int
draw_ltsymbol(Bar *bar, BarArg *a)
{
	int lpad = (lts_width*fonth - drw_fontset_getwidth(drw, bar->mon->ltsymbol, 0)) / 2;
	if (bar->mon->lt[bar->mon->sellt] == &speciallt) {
		drw_setscheme(drw, scheme[SchemeSel]);
	}
	return drw_text(drw, a->x, a->y, a->w, a->h, lpad, bar->mon->ltsymbol, 0, 0);
}

int
click_ltsymbol(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkLtSymbol;
}
