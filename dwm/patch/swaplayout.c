void
swaplayout(const Arg *arg)
{
	if (!arg || !arg->v) return;

	const Layout *const *ls = (const Layout *const *)arg->v;
	if (!ls[0] || !ls[1]) return;

	if (selmon->lt[selmon->sellt] != ls[0]) {
		setlayout(&(const Arg){ .v=ls[0] });
	} else {
		setlayout(&(const Arg){ .v=ls[1] });
	}
}
