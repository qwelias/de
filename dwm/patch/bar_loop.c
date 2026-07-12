#include <X11/Xlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bar_loop.h"
#include "../util.h"
static size_t sleepcounter = 0;

static int
runupdates(int force)
{
	int redraw = 0;
	const BarUpdateSet *bu;
	for (size_t i = 0; i < LENGTH(barupdates); i++) {
		bu = &barupdates[i];
		if (
			sleepcounter % bu->interval == 0
			|| (bu->dirty && *bu->dirty)
			|| force
		) {
			if (bu->dirty) *bu->dirty = 0;
			bu->run();
			redraw = 1;
		}
	}
	return redraw;
}

void
triggerstatusbar(void)
{
	XEvent ev;
	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;
	ev.xclient.window = root;
	ev.xclient.message_type = wmatom[WMStatusRedraw];
	ev.xclient.format = 32;

	XSendEvent(dpy, root, False, StructureNotifyMask, &ev);
	XFlush(dpy);
}

void *
init_bar_loop(void *arg)
{
	runupdates(1);

	while (1) {
		sleep(1);
		sleepcounter++;

		if (runupdates(0)) {
			triggerstatusbar();
		}
	}
	return NULL;
}
