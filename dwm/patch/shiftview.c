void
shiftview(const Arg *arg)
{
	Client* c;
	Arg shifted = shift(arg, 0);
	Arg next = shifted;
	int i = arg->i;
	int incr = i > 0 ? 1 : -1;
	fprintf(stderr, "shiftview: i %d \n", i);

	do {
		unsigned int ctags = next.ui & TAGMASK;
		fprintf(stderr, "shiftview: ctags %d \n", ctags);
		for (c = selmon->clients; c; c = c->next) {
			if (c->tags & ctags) {
				view(&next);
				return;
			}
		}
		i += incr;
		fprintf(stderr, "shiftview: i %d \n", i);
		next = shift(&(Arg){ .i = i }, 0);
	} while (next.ui != shifted.ui);
	view(&shifted);
}
