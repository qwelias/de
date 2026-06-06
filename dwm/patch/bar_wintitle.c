static const unsigned int client_indicators_width = 600;
static const unsigned int client_indicators_spacing = 5;
static const unsigned int client_indicators_height_active = 2;
static const unsigned int client_indicators_height_other = 1;

static void
draw_client_indicators(Bar *bar) {
	Client *c = bar->mon->clients;
	unsigned int cn = 0;
	unsigned int cliw = 0;
	unsigned int clis = bar->bw/2 - client_indicators_width/2;
	unsigned int active = 0;

	while (c) {
		if (ISVISIBLE(c)) cn++;
		c = c->next;
	}
	if (!cn || cn > 50) return;

	cliw = (client_indicators_width - cn*client_indicators_spacing) / cn;
	c = bar->mon->clients;
	cn = 0;
	while (c) {
		active = bar->mon->sel == c;
		if (ISVISIBLE(c)) {
			drw_rect(
				drw,
				clis + cliw*cn + client_indicators_spacing*cn, bar->by,
				cliw, active ? client_indicators_height_active : client_indicators_height_other,
				1, 0
			);
			cn++;
		}
		c = c->next;
	} 
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

	drawstateindicator(m, c, 1, a->x + fonth/2, a->y, a->w, a->h, 0, 0, c->isfixed);
	draw_client_indicators(bar);
	return 1;
}

int
click_wintitle(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkWinTitle;
}

