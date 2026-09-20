#ifndef BAR_SYSTRAY_H
#define BAR_SYSTRAY_H

#include "./dwm.h"

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY      0
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_FOCUS_IN             4
#define XEMBED_MODALITY_ON         10

#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_WINDOW_ACTIVATE      1
#define XEMBED_WINDOW_DEACTIVATE    2

#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

typedef struct Systray Systray;
struct Systray {
	Window win;
	Client *icons;
	Bar *bar;
	int h;
};

static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
static int refresh_systray_icons = 0;

static int
width_systray(Bar *bar, BarArg *a)
{
	unsigned int w = 0;
	Client *i;
	if (!systray)
		return 1;
	for (i = systray->icons; i; w += i->w + fonth/2, i = i->next);
	if (!w)
		XMoveWindow(dpy, systray->win, -systray->h, bar->by);
	return w ? w + fonth : 0;
}

static int
draw_systray(Bar *bar, BarArg *a)
{
	XSetWindowAttributes wa;
	XWindowChanges wc;
	Client *i;
	unsigned int w;

	if (!systray) {
		/* init systray */
		if (!(systray = (Systray *)calloc(1, sizeof(Systray))))
			die("fatal: could not malloc() %u bytes\n", sizeof(Systray));

		wa.override_redirect = True;
		wa.event_mask = ButtonPressMask|ExposureMask;
		wa.border_pixel = 0;
		systray->h = MIN(a->h, drw->fonts->h);
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		systray->win = XCreateSimpleWindow(dpy, root, bar->bx + a->x + fonth / 2, -systray->h, MIN(a->w, 1), systray->h, 0, 0, scheme[SchemeNorm][ColBg].pixel);
		XChangeWindowAttributes(dpy, systray->win, CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &wa);

		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *)&systrayorientation, 1);
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *)&netatom[NetWMWindowTypeDock], 1);
		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		} else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return 0;
		}
	}

	systray->bar = bar;

	wc.stack_mode = Above;
	wc.sibling = bar->win;
	XConfigureWindow(dpy, systray->win, CWSibling|CWStackMode, &wc);

	drw_setscheme(drw, scheme[SchemeNorm]);
	for (w = 0, i = systray->icons; i; i = i->next) {
		wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		if (refresh_systray_icons)
			XClearArea(dpy, i->win, 0, 0, 0, 0, True);
		w += i->w;
		if (i->next)
			w += fonth/2;
		if (i->mon != bar->mon)
			i->mon = bar->mon;
	}
	refresh_systray_icons = 0;

	wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
	XChangeWindowAttributes(dpy, systray->win, CWBackPixel, &wa);
	XClearWindow(dpy, systray->win);

	XMoveResizeWindow(dpy, systray->win, bar->bx + a->x + fonth / 2, (w ? bar->by + a->y + (a->h - systray->h) / 2: -systray->h), MAX(w, 1), systray->h);
	return w;
}

static int
click_systray(Bar *bar, Arg *arg, BarArg *a)
{
	return -1;
}

static void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!i) return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	refresh_systray_icons = 1;
	XReparentWindow(dpy, i->win, root, 0, 0);
	free(i);
	drawbarwin(systray->bar);
}

static Client *
wintosystrayicon(Window w)
{
	if (!systray)
		return NULL;
	Client *i = NULL;
	if (!w) return i;
	for (i = systray->icons; i && i->win != w; i = i->next);
	return i;
}

static void
updatesystrayicongeom(Client *i, int w, int h)
{
	if (!systray)
		return;

	int icon_height = systray->h;
	if (i) {
		i->h = icon_height;
		if (w == h)
			i->w = icon_height;
		else if (h == icon_height)
			i->w = w;
		else
			i->w = (int) ((float)icon_height * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > icon_height) {
			if (i->w == i->h)
				i->w = icon_height;
			else
				i->w = (int) ((float)icon_height * ((float)i->w / (float)i->h));
			i->h = icon_height;
		}
		if (i->w > 2 * icon_height)
			i->w = icon_height;
	}
}

static void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		drawbarwin(systray->bar);
	}
}

static void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!systray || !i || ev->atom != xatom[XembedInfo] ||
			!(flags = getatomprop(i, xatom[XembedInfo], xatom[XembedInfo])))
		return;

	if (flags & XEMBED_MAPPED && !i->tags) {
		i->tags = 1;
		code = XEMBED_WINDOW_ACTIVATE;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	}
	else if (!(flags & XEMBED_MAPPED) && i->tags) {
		i->tags = 0;
		code = XEMBED_WINDOW_DEACTIVATE;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	}
	else
		return;
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}

#endif //BAR_SYSTRAY_H