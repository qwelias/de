void
gaplessgrid(Monitor *m)
{
	unsigned int i, n;
	int x, y, cols, rows, ch, cw, cn, rn, rrest, crest; // counters
	int ot = gapptop;
	int ob = gappbot;
	int ow = gappow;
	int ih = gappih;
	int iw = gappiw;

	Client *c;
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);

	if (n == 0)
		return;

	/* grid dimensions */
	for (cols = 0; cols <= n/2; cols++)
		if (cols*cols >= n)
			break;
	if (n == 5) /* set layout against the general calculation: not 1:2:2, but 2:3 */
		cols = 2;
	rows = n/cols;
	cn = rn = 0; // reset column no, row no, client count

	ch = (m->wh - ot - ob - ih * (rows - 1)) / rows;
	cw = (m->ww - 2*ow - iw * (cols - 1)) / cols;
	rrest = (m->wh - ot - ob - ih * (rows - 1)) - ch * rows;
	crest = (m->ww - 2*ow - iw * (cols - 1)) - cw * cols;
	x = m->wx + ow;
	y = m->wy + ot;

	for (i = 0, c = nexttiled(m->clients); c; i++, c = nexttiled(c->next)) {
		if (i/rows + 1 > cols - n%cols) {
			rows = n/cols + 1;
			ch = (m->wh - ot - ob - ih * (rows - 1)) / rows;
			rrest = (m->wh - ot - ob - ih * (rows - 1)) - ch * rows;
		}
		resize(c,
			x,
			y + rn*(ch + ih) + MIN(rn, rrest),
			cw + (cn < crest ? 1 : 0) - 2*c->bw,
			ch + (rn < rrest ? 1 : 0) - 2*c->bw,
			0);
		rn++;
		if (rn >= rows) {
			rn = 0;
			x += cw + ih + (cn < crest ? 1 : 0);
			cn++;
		}
	}
}

