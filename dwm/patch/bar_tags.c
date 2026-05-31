int
width_tags(Bar *bar, BarArg *a)
{
	int w, i;

	for (w = 0, i = 0; i < LENGTH(tags); i++) {
		w += TEXTW(tags[i]);
	}
	return w;
}

int
draw_tags(Bar *bar, BarArg *a)
{
	int invert;
	int w, x = a->x;
	unsigned int i, occ = 0, urg = 0;
	const char *icon;
	Client *c;
	Monitor *m = bar->mon;
	int is_tag_selected;

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	for (i = 0; i < LENGTH(tags); i++) {
		is_tag_selected = m->tagset[m->seltags] & 1 << i;
		icon = tags[i];
		invert = 0;
		w = TEXTW(icon);
		drw_setscheme(drw, scheme[
			is_tag_selected
			? SchemeTagsSel
			: urg & 1 << i
			? SchemeUrg
			: SchemeTagsNorm
		]);
		drw_text(drw, x, a->y, w, a->h, lrpad / 2, icon, invert, False);
		drawindicator(m, NULL, occ, x, a->y, w, a->h, i, is_tag_selected, invert, tagindicatortype);
		if (is_tag_selected) drawindicator(m, NULL, 1, x, a->y, w, a->h, 0, 1, invert, INDICATOR_BOTTOM_BAR);
		else drawindicator(m, NULL, 1, x, a->y, w, a->h, 0, 1, invert, INDICATOR_BOTTOM_BAR_SLIM);
		x += w;
	}

	return 1;
}

int
click_tags(Bar *bar, Arg *arg, BarArg *a)
{
	int i = 0, x = 0;

	do {
		x += TEXTW(tags[i]);
	} while (a->x >= x && ++i < LENGTH(tags));
	if (i < LENGTH(tags)) {
		arg->ui = 1 << i;
	}
	return ClkTagBar;
}

int
hover_tags(Bar *bar, BarArg *a, XMotionEvent *ev)
{

	return 1;
}
