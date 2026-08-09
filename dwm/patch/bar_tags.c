static double tags_width = 1.8;

int
width_tags(Bar *bar, BarArg *a)
{
	return fonth*LENGTH(tags)*tags_width;
}

int
draw_tags(Bar *bar, BarArg *a)
{
	int invert;
	int w, lpad, x = a->x;
	unsigned int i, occ = 0, urg = 0;
	const char *icon;
	Client *c;
	Monitor *m = bar->mon;
	int is_tag_selected, is_tag_urg;

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	for (i = 0; i < LENGTH(tags); i++) {
		is_tag_selected = m->tagset[m->seltags] & 1 << i;
		is_tag_urg = urg & 1 << i;
		icon = tags[i];
		invert = 0;
		w = tags_width*fonth;
		lpad = (w - drw_fontset_getwidth(drw, icon, 0)) / 2;
		drw_setscheme(drw, scheme[
			is_tag_selected ? is_tag_urg ? SchemeSelUrg : SchemeTagsSel
			: is_tag_urg ? SchemeUrg
			: SchemeTagsNorm
		]);
		drw_text(drw, x, a->y, w, a->h, lpad, icon, invert, False);
		drawindicator(m, NULL, occ, x, a->y, w, a->h, i, is_tag_selected, invert, INDICATOR_TOP_LEFT_SQUARE);
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
		x += tags_width*fonth;
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
