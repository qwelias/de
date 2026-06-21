static const double client_indicators_width = 0.30;
static const double client_indicators_spacing = 0.7;
static const unsigned int client_indicators_active_offset = 1;
static const unsigned int client_indicators_size = 1;
static const unsigned int floating_indicator_h = 2;

static void
draw_client_indicators(Bar *bar) {
	Client *c = bar->mon->clients;
	unsigned int cn = 0;
	unsigned int cliw = bar->bw*client_indicators_width;
	unsigned int start = bar->bw/2 - cliw/2;
	unsigned int active = 0;
	unsigned int x = 0;
	unsigned int y = bar->by;
	unsigned int w = 0;
	unsigned int h = 0;

	while (c) {
		if (ISVISIBLE(c)) cn++;
		c = c->next;
	}
	if (!cn || cn > 16) return;

	cliw = (cliw - (cn - 1)*client_indicators_spacing*fonth) / cn;
	c = bar->mon->clients;
	cn = 0;
	while (c) {
		if (ISVISIBLE(c)) {
			active = bar->mon->sel == c;
			x = start + cliw*cn + client_indicators_spacing*cn*fonth;
			w = cliw;
			h = client_indicators_size;
			if (active) {
				x -= client_indicators_active_offset;
				w += client_indicators_active_offset*2;
				h += client_indicators_active_offset;
			}

			drw_rect(drw, x, y, w, h, 1, 0);
			cn++;
		}
		c = c->next;
	} 
}

static void
draw_floating_indicator(Bar *bar) {
	int w = bar->bw*client_indicators_width/2;
	int x = bar->bw/2 - w/2;
	int y = bar->by + bar->bh - floating_indicator_h;

	drw_rect(
		drw,
		x, y,
		w, floating_indicator_h,
		1, 0
	);
}

int
width_wintitle(Bar *bar, BarArg *a)
{
	return a->w;
}

int
draw_wintitle(Bar *bar, BarArg *a)
{
	Monitor *m = bar->mon;
	Client *c = m->sel;

	if (!c) {
		drw_setscheme(drw, scheme[SchemeTitleNorm]);
		drw_rect(drw, a->x, a->y, a->w, a->h, 1, 1);
		return 0;
	}

	drw_setscheme(drw, scheme[m == selmon ? SchemeTitleSel : SchemeTitleNorm]);

	XSetForeground(drw->dpy, drw->gc, drw->scheme[ColBg].pixel);
	XFillRectangle(drw->dpy, drw->drawable, drw->gc, a->x, a->y, a->w, a->h);

	drw_text(drw, a->x + fonth, a->y, a->w - fonth*2, a->h, 0, c->name, 0, False);

	if (c->isfloating) {
		draw_floating_indicator(bar);
	}
	draw_client_indicators(bar);
	return 1;
}

int
click_wintitle(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkWinTitle;
}

