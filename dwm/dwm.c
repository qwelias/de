/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <poll.h>
#include <inttypes.h>
#include <math.h>
#include <errno.h>
#include <X11/X.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/XF86keysym.h>

#pragma region macros
#ifdef _DEBUG
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define BAR_HEADERS(name)                              \
    static int bar_##name##_width(Bar *bar, BarArg *a); \
    static int bar_##name##_draw(Bar *bar, BarArg *a);  \
    static int bar_##name##_click(Bar *bar, Arg *arg, BarArg *a);

#define MAX_BARRULES                20
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define HIDDEN(C)               ((getstate(C->win) == IconicState))

#define MAX(A, B)               ((A) > (B) ? (A) : (B))
#define MIN(A, B)               ((A) < (B) ? (A) : (B))
#define CLAMP(A, B, C)               (MIN((C), (MAX((A), (B)))))
#define LENGTH(X)               (sizeof (X) / sizeof (X)[0])
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#pragma endregion macros

#pragma region enums
enum {
	CurResizeBR,
	CurResizeBL,
	CurResizeTR,
	CurResizeTL,
	CurResizeHorzArrow,
	CurResizeVertArrow,
	CurIronCross,
	CurNormal,
	CurResize,
	CurMove,
	CurLast
}; /* cursor */

enum {
	SchemeNorm,
	SchemeSel,
	SchemeTitleNorm,
	SchemeTitleSel,
	SchemeTagsNorm,
	SchemeTagsSel,
	SchemeHidNorm,
	SchemeHidSel,
	SchemeUrg,
	SchemeSelUrg,
}; /* color schemes */

enum {
	NetSupported, NetWMName, NetWMState, NetWMCheck,
	NetWMFullscreen, NetActiveWindow, NetWMWindowType,
	NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation,
	NetSystemTrayVisual, NetWMWindowTypeDock, NetSystemTrayOrientationHorz,
	NetClientList,
	NetLast
}; /* EWMH atoms */

enum {
	WMProtocols,
	WMDelete,
	WMState,
	WMTakeFocus,
	WMStatusRedraw,
	WMLast
}; /* default atoms */

enum {
	ClientFields,
	ClientTags,
	ClientLast
}; /* dwm client atoms */

enum {
	ClkTagBar,
	ClkLtSymbol,
	ClkStatusText,
	ClkWinTitle,
	ClkClientWin,
	ClkRootWin,
	ClkLast
}; /* clicks */

enum {
	BAR_ALIGN_LEFT,
	BAR_ALIGN_CENTER,
	BAR_ALIGN_RIGHT,
	BAR_ALIGN_LEFT_LEFT,
	BAR_ALIGN_LEFT_RIGHT,
	BAR_ALIGN_LEFT_CENTER,
	BAR_ALIGN_NONE,
	BAR_ALIGN_RIGHT_LEFT,
	BAR_ALIGN_RIGHT_RIGHT,
	BAR_ALIGN_RIGHT_CENTER,
	BAR_ALIGN_LAST
}; /* bar alignment */

enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */


enum {
	INDICATOR_NONE,
	INDICATOR_TOP_LEFT_SQUARE,
	INDICATOR_TOP_LEFT_LARGER_SQUARE,
	INDICATOR_TOP_BAR,
	INDICATOR_TOP_BAR_SLIM,
	INDICATOR_BOTTOM_BAR,
	INDICATOR_BOTTOM_BAR_SLIM,
	INDICATOR_BOX,
	INDICATOR_BOX_WIDER,
	INDICATOR_BOX_FULL,
	INDICATOR_CLIENT_DOTS,
	INDICATOR_RIGHT_TAGS,
	INDICATOR_PLUS,
	INDICATOR_PLUS_AND_SQUARE,
	INDICATOR_PLUS_AND_LARGER_SQUARE,
}; /* indicators */
#pragma endregion enums

#pragma region structs
typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct Monitor Monitor;
typedef struct Bar Bar;
struct Bar {
	Window win;
	Monitor *mon;
	Bar *next;
	int idx;
	int showbar;
	int topbar;
	int external;
	int borderpx;
	int borderscheme;
	int bx, by, bw, bh; /* bar geometry */
	int w[MAX_BARRULES]; // width, array length == barrules, then use r index for lookup purposes
	int x[MAX_BARRULES]; // x position, array length == ^
};

typedef struct {
	int x;
	int y;
	int h;
	int w;
} BarArg;

typedef struct {
	int monitor;
	int bar;
	int alignment; // see bar alignment enum
	int (*widthfunc)(Bar *bar, BarArg *a);
	int (*drawfunc)(Bar *bar, BarArg *a);
	int (*clickfunc)(Bar *bar, Arg *arg, BarArg *a);
	int (*hoverfunc)(Bar *bar, BarArg *a, XMotionEvent *ev);
	char *name; // for debugging
	int x, w; // position, width for internal use
} BarRule;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	const Layout *layout;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	float cfact;
	int x, y, w, h;
	unsigned int idx;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	unsigned int tags;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	int iscentered;
	int beingmoved;
	int iskilling;
	Client *next;
	Client *snext;
	Monitor *mon;
	Window win;
};

typedef struct {
	int ev;
	unsigned int mod;
	KeySym keysym;
	const Layout *layout;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Bar *bar;
	const Layout *lt[2];
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	const char *wintype;
	unsigned int tags;
	int iscentered;
	int isfloating;
	const char *floatpos;
	int monitor;
} Rule;

typedef struct {
	Cursor cursor;
} Cur;

typedef struct Fnt {
	Display *dpy;
	unsigned int h;
	XftFont *xfont;
	FcPattern *pattern;
	struct Fnt *next;
} Fnt;

enum { ColFg, ColBg, ColBorder, ColFloat, ColCount }; /* Clr scheme index */
typedef XftColor Clr;

typedef struct {
	unsigned int w, h;
	Display *dpy;
	int screen;
	Window root;
	Drawable drawable;
	GC gc;
	Clr *scheme;
	Fnt *fonts;
} Drw;

typedef struct BarUpdateSet {
	const size_t interval;
	void (*run)(void);
	int* dirty;
	const char *name;
} BarUpdateSet;

typedef struct Systray Systray;
struct Systray {
	Window win;
	Client *icons;
	Bar *bar;
	int h;
};
#pragma endregion structs

#pragma region headers
BAR_HEADERS(ltsym)
BAR_HEADERS(tags)
BAR_HEADERS(audio)
static void bar_audio_change(const Arg *arg);
static void bar_audio_toggle_source(const Arg *arg);
static void bar_audio_toggle_sink(const Arg *arg);
BAR_HEADERS(bat)
BAR_HEADERS(util)
BAR_HEADERS(time)
BAR_HEADERS(wintitle)
BAR_HEADERS(systray)

static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachx(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void drawbarwin(Bar *bar);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop, Atom req);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static int noborder(Client *c);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizerequest(XEvent *e);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void spawn_capture(const Arg *arg, char* output, ssize_t output_size);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus, Client *nextfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatetitle(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);
static int streqeq(const char* a, const char* b);

static void tile(Monitor *m);
static void monocle(Monitor *m);
static void gaplessgrid(Monitor *m);
static void shifttag(const Arg *arg);
static void shiftview(const Arg *arg);
static void swaplayout(const Arg *arg);
static void floatpos(const Arg *arg);
static void moveorplace(const Arg *arg);
static void dragmfact(const Arg *arg);
static void dragcfact(const Arg *arg);
#pragma endregion headers

#pragma region vars
static const char* tags[] = { "1", "2", "3", "4", "5" };
static const int bartextvoffset = 0;
static const unsigned int barhextra      = 10;   /* extra bar height to font height */
static const unsigned int borderpx       = 1;   /* border pixel of windows */
static const unsigned int snap           = 32;  /* snap pixel */
static const unsigned int gappih         = 35;  /* height inner gap between windows */
static const unsigned int gappiw         = 35;  /* width inner gap between windows */
static const unsigned int gappow         = 35;  /* width outer gap between windows and screen edge */
static const unsigned int gapptop         = 35;  /* top outer gap between windows and screen edge */
static const unsigned int gappbot         = 35;  /* bot outer gap between windows and screen edge */
static const int showbar                 = 1;   /* 0 means no bar */
static const int topbar                  = 1;   /* 0 means bottom bar */
static int floatposgrid_x                = 5;  /* float grid columns */
static int floatposgrid_y                = 5;  /* float grid rows */
static const int showsystray             = 1;   /* 0 means no systray */

static const char *fonts[]               = { "Ubuntu:size=11:style=Bold", "Noto Sans:size=11", "Symbols Nerd Font:size=15", "Noto Sans CJK SC:size=11", "Noto Color Emoji:pixelsize=11:antialias=true:autohint=true" };
static const char dmenufont[]            = "Ubuntu Mono:size=18";

static char c000000[]                    = "#000000"; // placeholder value

static char normfgcolor[]                = "#f8f8f2";
static char normbgcolor[]                = "#272822";
static char normbordercolor[]            = "#272822";
static char normfloatcolor[]             = "#272822";

static char selfgcolor[]                 = "#1d1e19";
static char selbgcolor[]                 = "#e6db74";
static char selbordercolor[]             = "#e6db74";
static char selfloatcolor[]              = "#e6db74";

static char titlenormfgcolor[]           = "#f8f8f2";
static char titlenormbgcolor[]           = "#272822";
static char titlenormbordercolor[]       = "#272822";
static char titlenormfloatcolor[]        = "#272822";

static char titleselfgcolor[]            = "#f8f8f2";
static char titleselbgcolor[]            = "#272822";
static char titleselbordercolor[]        = "#272822";
static char titleselfloatcolor[]         = "#272822";

static char tagsnormfgcolor[]            = "#f8f8f2";
static char tagsnormbgcolor[]            = "#272822";
static char tagsnormbordercolor[]        = "#e6db74";
static char tagsnormfloatcolor[]         = "#ae81ff";

static char tagsselfgcolor[]             = "#e6db74";
static char tagsselbgcolor[]             = "#3e3f37";
static char tagsselbordercolor[]         = "#272822";
static char tagsselfloatcolor[]          = "#ae81ff";

static char hidnormfgcolor[]             = "#f8f8f2";
static char hidselfgcolor[]              = "#f8f8f2";
static char hidnormbgcolor[]             = "#222222";
static char hidselbgcolor[]              = "#222222";

static char urgfgcolor[]                 = "#f8f8f2";
static char urgbgcolor[]                 = "#f92672";
static char urgbordercolor[]             = "#f92672";
static char urgfloatcolor[]              = "#f92672";

static char selurgfgcolor[]              = "#f92672";
static char selurgbgcolor[]              = "#3e3f37";
static char selurgbordercolor[]          = "#e6db74";
static char selurgfloatcolor[]           = "#e6db74";

static char *colors[][ColCount] = {
	/*                       fg                bg                border                float */
	[SchemeNorm]         = { normfgcolor,      normbgcolor,      normbordercolor,      normfloatcolor },
	[SchemeSel]          = { selfgcolor,       selbgcolor,       selbordercolor,       selfloatcolor },
	[SchemeTitleNorm]    = { titlenormfgcolor, titlenormbgcolor, titlenormbordercolor, titlenormfloatcolor },
	[SchemeTitleSel]     = { titleselfgcolor,  titleselbgcolor,  titleselbordercolor,  titleselfloatcolor },
	[SchemeTagsNorm]     = { tagsnormfgcolor,  tagsnormbgcolor,  tagsnormbordercolor,  tagsnormfloatcolor },
	[SchemeTagsSel]      = { tagsselfgcolor,   tagsselbgcolor,   tagsselbordercolor,   tagsselfloatcolor },
	[SchemeHidNorm]      = { hidnormfgcolor,   hidnormbgcolor,   c000000,              c000000 },
	[SchemeHidSel]       = { hidselfgcolor,    hidselbgcolor,    c000000,              c000000 },
	[SchemeUrg]          = { urgfgcolor,       urgbgcolor,       urgbordercolor,       urgfloatcolor },
	[SchemeSelUrg]       = { selurgfgcolor,    selurgbgcolor,    selurgbordercolor,    selurgfloatcolor },
};

/* There are two options when it comes to per-client rules:
 *  - a typical struct table or
 *  - using the RULE macro
 *
 * A traditional struct table looks like this:
 *    // class      instance  title  wintype  tags mask  isfloating  monitor
 *    { "Gimp",     NULL,     NULL,  NULL,    1 << 4,    0,          -1 },
 *    { "Firefox",  NULL,     NULL,  NULL,    1 << 7,    0,          -1 },
 *
 * The RULE macro has the default values set for each field allowing you to only
 * specify the values that are relevant for your rule, e.g.
 *
 *    RULE(.class = "Gimp", .tags = 1 << 4)
 *    RULE(.class = "Firefox", .tags = 1 << 7)
 *
 * Refer to the Rule struct definition for the list of available fields depending on
 * the patches you enable.
 */
static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 *	WM_WINDOW_ROLE(STRING) = role
	 *	_NET_WM_WINDOW_TYPE(ATOM) = wintype
	 */
	{ .monitor = -1, .title = "WIREMIX", .isfloating = 1, .floatpos = "100%   0% 800W 300H"},
	{ .monitor = -1, .title = "CALENDAR", .isfloating = 1, .floatpos = "88%   0% 255W 550H"},
	{ .monitor = -1, .title = "TIMES", .isfloating = 1, .floatpos = "88%   0% 365W 210H"},
	{ .monitor = -1, .title = "AmneziaVPN", .isfloating = 1},
	{ .monitor = -1, .title = "Delta Chat", .isfloating = 1},
	{ .monitor = -1, .title = "Karing", .isfloating = 1},
	{ .monitor = -1, .wintype = "_NET_WM_WINDOW_TYPE_DIALOG", .isfloating = 1},
	{ .monitor = -1, .wintype = "_NET_WM_WINDOW_TYPE_UTILITY", .isfloating = 1},
	{ .monitor = -1, .wintype = "_NET_WM_WINDOW_TYPE_TOOLBAR", .isfloating = 1},
	{ .monitor = -1, .wintype = "_NET_WM_WINDOW_TYPE_SPLASH", .isfloating = 1}
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int refreshrate_resize = 6;  /* refresh rate (per second) for client resize */
static const int refreshrate_dragmfact = 6; /* refresh rate (per second) for dragmfact */
static const int refreshrate_dragcfact = 6; /* refresh rate (per second) for dragcfact */

static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
	"dmenu_run",
	"-m", dmenumon,
	"-fn", dmenufont,
	"-nb", normbgcolor,
	"-nf", normfgcolor,
	"-sb", selbgcolor,
	"-sf", selfgcolor,
	NULL
};
static const char *termcmd[]  = { "ghostty", NULL };

static const char *brighter[] = { "brightnessctl", "set", "10%+", NULL };
static const char *dimmer[]   = { "brightnessctl", "set", "10%-", NULL };
static const char *kbd_up[]   = { "brightnessctl", "--device=tpacpi::kbd_backlight", "set", "10%+", NULL };
static const char *kbd_down[] = { "brightnessctl", "--device=tpacpi::kbd_backlight", "set", "10%-", NULL };
static const char *lock[]     = { "xset", "dpms", "force", "off", NULL };
static const char *maimss[]   = { "maimpick.sh", "ss", NULL };
static const char *maimocr[]  = { "maimpick.sh", "ocr", NULL };
static const char *switchsink[]  = { "switch_sink.sh", NULL };
static const char *suspend[]  = { "systemctl", "suspend", NULL };
static const char *clicks[]  = { "xdotool", "click", "--repeat", "100", "--delay", "30", "1", NULL };

static const Layout speciallt = { ": : :",      gaplessgrid };
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[ ]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[    ]",      monocle },
	speciallt,
};
static const Layout *layouts_swap[] = { &layouts[2], &layouts[0] };

/* Bar rules allow you to configure what is shown where on the bar, as well as
 * introducing your own bar modules.
 *
 *    monitor:
 *      -1  show on all monitors
 *       0  show on monitor 0
 *      'A' show on active monitor (i.e. focused / selected) (or just -1 for active?)
 *    bar - bar index, 0 is default, 1 is extrabar
 *    alignment - how the module is aligned compared to other modules
 *    widthfunc, drawfunc, clickfunc - providing bar module width, draw and click functions
 *    name - does nothing, intended for visual clue and for logging / debugging
 */
static const BarRule barrules[] = {
	/* monitor   bar    alignment         widthfunc                 drawfunc                clickfunc                hoverfunc                name */
	{ -1,        0,     BAR_ALIGN_LEFT,   bar_ltsym_width,          bar_ltsym_draw,         bar_ltsym_click,         NULL,                    "layout" },
	{ -1,        0,     BAR_ALIGN_LEFT,   bar_tags_width,           bar_tags_draw,          bar_tags_click,          NULL,                    "tags" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  bar_bat_width,            bar_bat_draw,           bar_bat_click,           NULL,                    "bat" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  bar_util_width,           bar_util_draw,          bar_util_click,          NULL,                    "bar_system_stats" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  bar_audio_width,          bar_audio_draw,         bar_audio_click,         NULL,                    "bar_audio" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  bar_time_width,           bar_time_draw,          bar_time_click,          NULL,                    "bar_time" },
	{  0,        0,     BAR_ALIGN_RIGHT,  bar_systray_width,        bar_systray_draw,       bar_systray_click,       NULL,                    "systray" },
	{ -1,        0,     BAR_ALIGN_NONE,   bar_wintitle_width,       bar_wintitle_draw,      bar_wintitle_click,      NULL,                    "wintitle" },
};

#define TAGKEYS(KEY,TAG) \
	{ KeyPress,    0,                              KEY,   &speciallt,   view,           {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask,                       KEY,   NULL,          view,           {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ControlMask,           KEY,   NULL,          toggleview,     {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ShiftMask,             KEY,   NULL,          tag,            {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ControlMask|ShiftMask, KEY,   NULL,          toggletag,      {.ui = 1 << TAG} },

static const Key keys[] = {
	/* event,      modifier                      key                         layout        function                argument */
	// fn keys
	{ KeyPress,    0,                            XF86XK_AudioRaiseVolume,    NULL,         bar_audio_change,           {.i = (int)'+' } },
	{ KeyPress,    0,                            XF86XK_AudioLowerVolume,    NULL,         bar_audio_change,           {.i = (int)'-' } },
	{ KeyPress,    0,                            XF86XK_AudioMute,           NULL,         bar_audio_toggle_sink,      {0} },
	{ KeyPress,    0,                            XF86XK_AudioMicMute,        NULL,         bar_audio_toggle_source,    {0} },
	{ KeyPress,    0,                            XF86XK_MonBrightnessUp,     NULL,         spawn,                  {.v = brighter } },
	{ KeyPress,    0,                            XF86XK_MonBrightnessDown,   NULL,         spawn,                  {.v = dimmer } },
	{ KeyPress,    0,                            XF86XK_KbdBrightnessUp,     NULL,         spawn,                  {.v = kbd_up } },
	{ KeyPress,    0,                            XF86XK_KbdBrightnessDown,   NULL,         spawn,                  {.v = kbd_down } },
	// spawn
	{ KeyPress,    Mod4Mask,                       XK_r,                       NULL,         spawn,                  {.v = dmenucmd } },
	{ KeyPress,    Mod4Mask,                       XK_t,                       NULL,         spawn,                  {.v = termcmd } },
	{ KeyPress,    Mod4Mask,                       XK_z,                       NULL,         spawn,                  {.v = lock } },
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_z,                       NULL,         spawn,                  {.v = suspend } },
	{ KeyPress,    0,                              XK_Print,                   NULL,         spawn,                  {.v = maimss } },
	{ KeyPress,    ShiftMask,                      XK_Print,                   NULL,         spawn,                  {.v = maimocr } },
	{ KeyPress,    Mod4Mask,                       XK_p,                       NULL,         spawn,                  {.v = maimss } },
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_p,                       NULL,         spawn,                  {.v = maimocr } },
	{ KeyPress,    ControlMask|Mod1Mask,           XK_x,                       NULL,         spawn,                  {.v = switchsink } },
	// nav clients
	{ KeyPress,    Mod4Mask,                       XK_b,                       NULL,         togglebar,              {0} },
	{ KeyPress,    Mod4Mask,                       XK_d,                       NULL,         focusstack,             {.i = +1 } },
	{ KeyPress,    Mod4Mask,                       XK_a,                       NULL,         focusstack,             {.i = -1 } },
	{ KeyPress,    0,                              XK_d,                       &speciallt,  focusstack,             {.i = +1 } },
	{ KeyPress,    0,                              XK_a,                       &speciallt,  focusstack,             {.i = -1 } },
	{ KeyPress,    Mod4Mask,                       XK_Tab,                     NULL,         zoom,                   {0} },
	{ KeyPress,    0,                              XK_Tab,                     &speciallt,  zoom,                   {0} },
	// nav tags
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_q,                       NULL,         shifttag,               { .i = -1 } }, // note keybinding conflict with focusadjacenttag tagtoleft
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_e,                       NULL,         shifttag,               { .i = +1 } }, // note keybinding conflict with focusadjacenttag tagtoright
	{ KeyPress,    Mod4Mask,                       XK_q,                       NULL,         shiftview,              { .i = -1 } },
	{ KeyPress,    Mod4Mask,                       XK_e,                       NULL,         shiftview,              { .i = +1 } },
	{ KeyPress,    0,                              XK_q,                       &speciallt,   shiftview,              { .i = -1 } },
	{ KeyPress,    0,                              XK_e,                       &speciallt,   shiftview,              { .i = +1 } },
	TAGKEYS(                        XK_1,                                  0)
	TAGKEYS(                        XK_2,                                  1)
	TAGKEYS(                        XK_3,                                  2)
	TAGKEYS(                        XK_4,                                  3)
	TAGKEYS(                        XK_5,                                  4)
	// kills
	{ KeyPress,    Mod4Mask,                       XK_c,                       NULL,         killclient,             {0} },
	{ KeyPress,    0,                              XK_c,                       &speciallt,  killclient,             {0} },
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_c,                       NULL,         quit,                   {0} },
	// layouts
	{ KeyPress,    Mod4Mask,                       XK_x,                       NULL,         swaplayout,             { .v = &layouts_swap } },
	{ KeyRelease,  0,                              XK_Super_L,                 NULL,         setlayout,              { .v = &speciallt } },
	// mons
	{ KeyPress,    Mod4Mask,                       XK_comma,                   NULL,         focusmon,               {.i = -1 } },
	{ KeyPress,    Mod4Mask,                       XK_period,                  NULL,         focusmon,               {.i = +1 } },
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_comma,                   NULL,         tagmon,                 {.i = -1 } },
	{ KeyPress,    Mod4Mask|ShiftMask,             XK_period,                  NULL,         tagmon,                 {.i = +1 } },
	// floating
	{ KeyPress,    Mod4Mask,                       XK_Up,                      NULL,         togglefloating,         {0} },
	{ KeyPress,    Mod4Mask,                       XK_Left,                    NULL,         floatpos,               {.v = "0% 50% 50% 100%" } },
	{ KeyPress,    Mod4Mask,                       XK_Right,                   NULL,         floatpos,               {.v = "100% 50% 50% 100%" } },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask           button         layout           function        argument */
	{ ClkLtSymbol,          0,                     Button4,       NULL,            focusstack,     {.i = -1 } },
	{ ClkLtSymbol,          0,                     Button5,       NULL,            focusstack,     {.i = +1 } },
	{ ClkLtSymbol,          0,                     Button1,       NULL,            setlayout,      {.v = &speciallt } },
	{ ClkLtSymbol,          0,                     Button3,       NULL,            swaplayout,     {.v = &layouts_swap } },
	{ ClkWinTitle,          0,                     Button1,       NULL,            moveorplace,    {.i = 1} },
	{ ClkWinTitle,          0,                     Button2,       NULL,            killclient, {0} },
	{ ClkWinTitle,          0,                     Button3,       NULL,            resizemouse,    {0} },
	{ ClkWinTitle,          0,                     Button4,       NULL,            focusstack,     {.i = -1} },
	{ ClkWinTitle,          0,                     Button5,       NULL,            focusstack,     {.i = +1} },
	{ ClkClientWin,         Mod4Mask|Mod1Mask,     Button1,       NULL,            spawn,          { .v = clicks } },
	{ ClkClientWin,         Mod4Mask,              Button1,       NULL,            moveorplace,    {.i = 1} },
	{ ClkClientWin,         Mod4Mask,              Button2,       NULL,            togglefloating, {0} },
	{ ClkClientWin,         Mod4Mask,              Button3,       NULL,            resizemouse,    {0} },
	{ ClkClientWin,         0,                     Button1,       &speciallt,     setlayout,      { .v = &layouts[2] } },
	{ ClkClientWin,         0,                     Button2,       &speciallt,     killclient,     {0} },
	{ ClkClientWin,         0,                     Button3,       &speciallt,     setlayout,      { .v = &layouts[2] } },
	{ ClkClientWin,         Mod4Mask|ShiftMask,    Button3,       NULL,            dragcfact,      {0} },
	{ ClkClientWin,         Mod4Mask|ShiftMask,    Button1,       NULL,            dragmfact,      {0} },
	{ ClkTagBar,            0,                     Button1,       NULL,            view,           {0} },
	{ ClkTagBar,            0,                     Button3,       NULL,            toggleview,     {0} },
	{ ClkTagBar,            0,                     Button4,       NULL,            shiftview,       { .i = -1 } },
	{ ClkTagBar,            0,                     Button5,       NULL,            shiftview,       { .i = +1 } },
	{ ClkTagBar,            ShiftMask,             Button4,       NULL,            shifttag,       { .i = -1 } },
	{ ClkTagBar,            ShiftMask,             Button5,       NULL,            shifttag,       { .i = +1 } },
	{ ClkTagBar,            Mod4Mask,              Button1,       NULL,            tag,            {0} },
	{ ClkTagBar,            Mod4Mask,              Button3,       NULL,            toggletag,      {0} },
};

// dwm
static const char broken[] = "broken";
static pthread_t barloopth;

static int keypressed;
static int btnpressed;

static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar geometry */
static int fonth;            /* sum of left and right padding for text */
/* Some clients (e.g. alacritty) helpfully send configure requests with a new size or position
 * when they detect that they have been moved to another monitor. This can cause visual glitches
 * when moving (or resizing) client windows from one monitor to another. This variable is used
 * internally to ignore such configure requests while movemouse or resizemouse are being used. */
static int ignoreconfigurerequests = 0;
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static Atom wmatom[WMLast], netatom[NetLast];
static Atom xatom[XLast];
static Atom clientatom[ClientLast];
static volatile sig_atomic_t running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
#pragma endregion vars

#pragma region util
void
die(const char *fmt, ...)
{
	va_list ap;
	int saved_errno;

	saved_errno = errno;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (fmt[0] && fmt[strlen(fmt)-1] == ':')
		fprintf(stderr, " %s", strerror(saved_errno));
	fputc('\n', stderr);

	exit(1);
}

void *
ecalloc(size_t nmemb, size_t size)
{
	void *p;

	if (!(p = calloc(nmemb, size)))
		die("calloc:");
	return p;
}
#pragma endregion util

#pragma region drw
#define UTF_INVALID 0xFFFD

// circular
static unsigned int drw_fontset_getwidth(Drw *drw, const char *text, Bool markup);
static int drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert, Bool markup);

int
utf8decode(const char *s_in, long *u, int *err)
{
	static const unsigned char lens[] = {
		/* 0XXXX */ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
		/* 10XXX */ 0, 0, 0, 0, 0, 0, 0, 0,  /* invalid */
		/* 110XX */ 2, 2, 2, 2,
		/* 1110X */ 3, 3,
		/* 11110 */ 4,
		/* 11111 */ 0,  /* invalid */
	};
	static const unsigned char leading_mask[] = { 0x7F, 0x1F, 0x0F, 0x07 };
	static const unsigned int overlong[] = { 0x0, 0x80, 0x0800, 0x10000 };

	const unsigned char *s = (const unsigned char *)s_in;
	int len = lens[*s >> 3];
	*u = UTF_INVALID;
	*err = 1;
	if (len == 0)
		return 1;

	long cp = s[0] & leading_mask[len - 1];
	for (int i = 1; i < len; ++i) {
		if (s[i] == '\0' || (s[i] & 0xC0) != 0x80)
			return i;
		cp = (cp << 6) | (s[i] & 0x3F);
	}
	/* out of range, surrogate, overlong encoding */
	if (cp > 0x10FFFF || (cp >> 11) == 0x1B || cp < overlong[len - 1])
		return len;

	*err = 0;
	*u = cp;
	return len;
}

Drw *
drw_create(Display *dpy, int screen, Window root, unsigned int w, unsigned int h)
{
	Drw *drw = ecalloc(1, sizeof(Drw));

	drw->dpy = dpy;
	drw->screen = screen;
	drw->root = root;
	drw->w = w;
	drw->h = h;

	drw->drawable = XCreatePixmap(dpy, root, w, h, DefaultDepth(dpy, screen));
	drw->gc = XCreateGC(dpy, root, 0, NULL);
	XSetLineAttributes(dpy, drw->gc, 1, LineSolid, CapButt, JoinMiter);

	return drw;
}

void
drw_resize(Drw *drw, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	drw->w = w;
	drw->h = h;
	if (drw->drawable)
		XFreePixmap(drw->dpy, drw->drawable);
	drw->drawable = XCreatePixmap(drw->dpy, drw->root, w, h, DefaultDepth(drw->dpy, drw->screen));
}

static void
xfont_free(Fnt *font)
{
	if (!font)
		return;
	if (font->pattern)
		FcPatternDestroy(font->pattern);
	XftFontClose(font->dpy, font->xfont);
	free(font);
}

void
drw_fontset_free(Fnt *font)
{
	if (font) {
		drw_fontset_free(font->next);
		xfont_free(font);
	}
}

void
drw_free(Drw *drw)
{
	XFreePixmap(drw->dpy, drw->drawable);
	XFreeGC(drw->dpy, drw->gc);
	drw_fontset_free(drw->fonts);
	free(drw);
}

/* This function is an implementation detail. Library users should use
 * drw_fontset_create instead.
 */
static Fnt *
xfont_create(Drw *drw, const char *fontname, FcPattern *fontpattern)
{
	Fnt *font;
	XftFont *xfont = NULL;
	FcPattern *pattern = NULL;

	if (fontname) {
		/* Using the pattern found at font->xfont->pattern does not yield the
		 * same substitution results as using the pattern returned by
		 * FcNameParse; using the latter results in the desired fallback
		 * behaviour whereas the former just results in missing-character
		 * rectangles being drawn, at least with some fonts. */
		if (!(xfont = XftFontOpenName(drw->dpy, drw->screen, fontname))) {
			fprintf(stderr, "error, cannot load font from name: '%s'\n", fontname);
			return NULL;
		}
		if (!(pattern = FcNameParse((FcChar8 *) fontname))) {
			fprintf(stderr, "error, cannot parse font name to pattern: '%s'\n", fontname);
			XftFontClose(drw->dpy, xfont);
			return NULL;
		}
	} else if (fontpattern) {
		if (!(xfont = XftFontOpenPattern(drw->dpy, fontpattern))) {
			fprintf(stderr, "error, cannot load font from pattern.\n");
			return NULL;
		}
	} else {
		die("no font specified.");
	}

	font = ecalloc(1, sizeof(Fnt));
	font->xfont = xfont;
	font->pattern = pattern;
	font->h = xfont->ascent + xfont->descent;
	font->dpy = drw->dpy;

	return font;
}

Fnt*
drw_fontset_create(Drw* drw, const char *fonts[], size_t fontcount)
{
	Fnt *cur, *ret = NULL;
	size_t i;

	if (!drw || !fonts)
		return NULL;

	for (i = 1; i <= fontcount; i++) {
		if ((cur = xfont_create(drw, fonts[fontcount - i], NULL))) {
			cur->next = ret;
			ret = cur;
		}
	}
	return (drw->fonts = ret);
}

void
drw_clr_create(
	Drw *drw,
	Clr *dest,
	const char *clrname
) {
	if (!drw || !dest || !clrname)
		return;

	if (!XftColorAllocName(drw->dpy, DefaultVisual(drw->dpy, drw->screen),
	                       DefaultColormap(drw->dpy, drw->screen),
	                       clrname, dest))
		die("error, cannot allocate color '%s'", clrname);

}

/* Wrapper to create color schemes. The caller has to call free(3) on the
 * returned color scheme when done using it. */
Clr *
drw_scm_create(
	Drw *drw,
	char *clrnames[],
	size_t clrcount
) {
	size_t i;
	Clr *ret;

	/* need at least two colors for a scheme */
	if (!drw || !clrnames || clrcount < 2 || !(ret = ecalloc(clrcount, sizeof(XftColor))))
		return NULL;

	for (i = 0; i < clrcount; i++)
		drw_clr_create(drw, &ret[i], clrnames[i]);
	return ret;
}

void
drw_setfontset(Drw *drw, Fnt *set)
{
	if (drw)
		drw->fonts = set;
}

void
drw_setscheme(Drw *drw, Clr *scm)
{
	if (drw)
		drw->scheme = scm;
}

void
drw_rect(Drw *drw, int x, int y, unsigned int w, unsigned int h, int filled, int invert)
{
	if (!drw || !drw->scheme)
		return;
	XSetForeground(drw->dpy, drw->gc, invert ? drw->scheme[ColBg].pixel : drw->scheme[ColFg].pixel);
	if (filled)
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
	else
		XDrawRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w - 1, h - 1);
}

void
drw_font_getexts(Fnt *font, const char *text, unsigned int len, unsigned int *w, unsigned int *h)
{
	XGlyphInfo ext;

	if (!font || !text)
		return;

	XftTextExtentsUtf8(font->dpy, font->xfont, (XftChar8 *)text, len, &ext);
	if (w)
		*w = ext.xOff;
	if (h)
		*h = font->h;
}

unsigned int
drw_fontset_getwidth(Drw *drw, const char *text, Bool markup)
{
	if (!drw || !drw->fonts || !text)
		return 0;
	return drw_text(drw, 0, 0, 0, 0, 0, text, 0, markup);
}

int
drw_text(Drw *drw, int x, int y, unsigned int w, unsigned int h, unsigned int lpad, const char *text, int invert, Bool markup)
{
	int ty, ellipsis_x = 0;
	unsigned int tmpw, ew, ellipsis_w = 0, ellipsis_len, hash, h0, h1;
	XftDraw *d = NULL;
	Fnt *usedfont, *curfont, *nextfont;
	int utf8strlen, utf8charlen, utf8err, render = x || y || w || h;
	long utf8codepoint = 0;
	const char *utf8str;
	FcCharSet *fccharset;
	FcPattern *fcpattern;
	FcPattern *match;
	XftResult result;
	int charexists = 0, overflow = 0;
	/* keep track of a couple codepoints for which we have no match. */
	static unsigned int nomatches[128], ellipsis_width, invalid_width;
	static const char invalid[] = "�";

	if (!drw || (render && (!drw->scheme || !w)) || !text || !drw->fonts)
		return 0;

	if (!render) {
		w = invert ? invert : ~invert;
	} else {
		XSetForeground(drw->dpy, drw->gc, drw->scheme[invert ? ColFg : ColBg].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, y, w, h);
		if (w < lpad)
			return x + w;
		d = XftDrawCreate(drw->dpy, drw->drawable,
		                  DefaultVisual(drw->dpy, drw->screen),
		                  DefaultColormap(drw->dpy, drw->screen));
		x += lpad;
		w -= lpad;
	}

	usedfont = drw->fonts;
	if (!ellipsis_width && render)
		ellipsis_width = drw_fontset_getwidth(drw, "...", markup);
	if (!invalid_width && render)
		invalid_width = drw_fontset_getwidth(drw, invalid, markup);
	while (1) {
		ew = ellipsis_len = utf8err = utf8charlen = utf8strlen = 0;
		utf8str = text;
		nextfont = NULL;
		while (*text) {
			utf8charlen = utf8decode(text, &utf8codepoint, &utf8err);
			for (curfont = drw->fonts; curfont; curfont = curfont->next) {
				charexists = charexists || XftCharExists(drw->dpy, curfont->xfont, utf8codepoint);
				if (charexists) {
					drw_font_getexts(curfont, text, utf8charlen, &tmpw, NULL);
					if (ew + ellipsis_width <= w) {
						/* keep track where the ellipsis still fits */
						ellipsis_x = x + ew;
						ellipsis_w = w - ew;
						ellipsis_len = utf8strlen;
					}

					if (ew + tmpw > w) {
						overflow = 1;
						/* called from drw_fontset_getwidth_clamp():
						 * it wants the width AFTER the overflow
						 */
						if (!render)
							x += tmpw;
						else
							utf8strlen = ellipsis_len;
					} else if (curfont == usedfont) {
						text += utf8charlen;
						utf8strlen += utf8err ? 0 : utf8charlen;
						ew += utf8err ? 0 : tmpw;
					} else {
						nextfont = curfont;
					}
					break;
				}
			}

			if (overflow || !charexists || nextfont || utf8err)
				break;
			else
				charexists = 0;
		}

		if (utf8strlen) {
			if (render) {
				ty = y + (h - usedfont->h) / 2 + usedfont->xfont->ascent + bartextvoffset;
				XftDrawStringUtf8(d, &drw->scheme[invert ? ColBg : ColFg],
				                  usedfont->xfont, x, ty, (XftChar8 *)utf8str, utf8strlen);
			}
			x += ew;
			w -= ew;
		}
		if (utf8err && (!render || invalid_width < w)) {
			if (render)
				drw_text(drw, x, y, w, h, 0, invalid, invert, markup);
			x += invalid_width;
			w -= invalid_width;
		}
		if (render && overflow)
			drw_text(drw, ellipsis_x, y, ellipsis_w, h, 0, "...", invert, markup);

		if (!*text || overflow) {
			break;
		} else if (nextfont) {
			charexists = 0;
			usedfont = nextfont;
		} else {
			/* Regardless of whether or not a fallback font is found, the
			 * character must be drawn. */
			charexists = 1;

			hash = (unsigned int)utf8codepoint;
			hash = ((hash >> 16) ^ hash) * 0x21F0AAAD;
			hash = ((hash >> 15) ^ hash) * 0xD35A2D97;
			h0 = ((hash >> 15) ^ hash) % LENGTH(nomatches);
			h1 = (hash >> 17) % LENGTH(nomatches);
			/* avoid expensive XftFontMatch call when we know we won't find a match */
			if (nomatches[h0] == utf8codepoint || nomatches[h1] == utf8codepoint)
				goto no_match;

			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, utf8codepoint);

			if (!drw->fonts->pattern) {
				/* Refer to the comment in xfont_create for more information. */
				die("the first font in the cache must be loaded from a font string.");
			}

			fcpattern = FcPatternDuplicate(drw->fonts->pattern);
			FcPatternAddCharSet(fcpattern, FC_CHARSET, fccharset);
			FcPatternAddBool(fcpattern, FC_SCALABLE, FcTrue);

			FcConfigSubstitute(NULL, fcpattern, FcMatchPattern);
			FcDefaultSubstitute(fcpattern);
			match = XftFontMatch(drw->dpy, drw->screen, fcpattern, &result);

			FcCharSetDestroy(fccharset);
			FcPatternDestroy(fcpattern);

			if (match) {
				usedfont = xfont_create(drw, NULL, match);
				if (usedfont && XftCharExists(drw->dpy, usedfont->xfont, utf8codepoint)) {
					for (curfont = drw->fonts; curfont->next; curfont = curfont->next)
						; /* NOP */
					curfont->next = usedfont;
				} else {
					xfont_free(usedfont);
					nomatches[nomatches[h0] ? h1 : h0] = utf8codepoint;
no_match:
					usedfont = drw->fonts;
				}
			}
		}
	}
	if (d)
		XftDrawDestroy(d);

	return x + (render ? w : 0);
}

void
drw_map(Drw *drw, Window win, int x, int y, unsigned int w, unsigned int h)
{
	if (!drw)
		return;

	XCopyArea(drw->dpy, drw->drawable, win, drw->gc, x, y, w, h, x, y);
	XSync(drw->dpy, False);
}

Cur *
drw_cur_create(Drw *drw, int shape)
{
	Cur *cur;

	if (!drw || !(cur = ecalloc(1, sizeof(Cur))))
		return NULL;

	cur->cursor = XCreateFontCursor(drw->dpy, shape);

	return cur;
}

void
drw_cur_free(Drw *drw, Cur *cursor)
{
	if (!cursor)
		return;

	XFreeCursor(drw->dpy, cursor->cursor);
	free(cursor);
}

#pragma endregion

#pragma region audio
static int audio_dirty = 1;
static double audio_width = 5;
static char audio_txt[15] = " X X 000  ";
static unsigned int audio_color = 1;
static const char* wiremix[] = { "ghostty", "--title=WIREMIX", "--confirm-close-surface=false", "-e", "wiremix", NULL };
static const char* wpctlsink[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL };
static const char* wpctlsinkm[] = { "wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", NULL };
static const char* wpctlsinki[] = { "wpctl", "inspect", "@DEFAULT_AUDIO_SINK@", NULL };
static const char* wpctlsource[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SOURCE@", NULL };
static const char* wpctlsourcei[] = { "wpctl", "inspect", "@DEFAULT_AUDIO_SOURCE@", NULL };
static const char* wpctlsourcem[] = { "wpctl", "set-mute", "@DEFAULT_AUDIO_SOURCE@", "toggle", NULL };

int
killwiremix(void) {
    Client* c = selmon->clients;
    while (c) {
        if (strstr(c->name, "WIREMIX")) {
            killclient(&(Arg){ .v = c });
            return 1;
        }
        c = c->next;
    }

    return 0;
}

int bar_audio_width(Bar *bar, BarArg *a)
{
    return audio_width*fonth;
}

int bar_audio_draw(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(audio_color-1)]);

    int lpad = (audio_width*fonth - drw_fontset_getwidth(drw, audio_txt, 0)) / 2;
    return drw_text(drw, a->x, a->y, a->w, a->h, lpad, audio_txt, 0, 1);
}

void
readvolmute(char* buf, double* vol, int* mute) {
    // fprintf(stderr, "readvolmute: %2s\n", buf+13);
    if (vol) *vol = strtod(buf+8, NULL);
    if (mute) *mute = !strncmp(buf+13, "[M", 2);
}

void
audio_update(void) {
    // fprintf(stderr, "audio_update\n");
    char buf[16] = {0};
    double sinkvol = 0;
    int sinkm = 0;
    int sourcem = 0;

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, sizeof(buf) - 1);
    readvolmute(buf, &sinkvol, &sinkm);
    memset(buf, 0, sizeof(buf));
    // fprintf(stderr, "audio_update: sink %d %s\n", sinkm, buf);

    spawn_capture(&(Arg){ .v = wpctlsource }, buf, sizeof(buf) - 1);
    readvolmute(buf, NULL, &sourcem);
    // fprintf(stderr, "audio_update: source %d %s\n", sourcem, buf);

    snprintf(audio_txt, sizeof(audio_txt), 
        "%s%s%03d",
        sourcem ? " " : " ",
        sinkm ? "󰝟 " : "󰕾 ",
        (int)(sinkvol * 100)
    );
}

void
parse_name(char* out, char* name, unsigned int len) {
    char* left = NULL;
    char* right = NULL;

    left = strstr(out, "device.profile.description = ");
    if (left) left += 30;
    else {
        left = strstr(out, "node.description = ");
        if (left) left += 20;
    }

    right = left ? strstr(left, "\"") : NULL;
    if (!left || !right) {
        strcpy(name, "???");
    } else {
        strncpy(name, left, right - left);
        name[right - left + 1] = '\0';
    }
}

void bar_audio_change(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    double vol = 0;

    // fprintf(stderr, "audio_change\n");
    sprintf(buf, 
        "5%%%c",
        (char)arg->i
    );
    spawn_capture(&(Arg){ .v = (char*[]){ "wpctl", "set-volume","-l", "1.5", "@DEFAULT_AUDIO_SINK@", buf, NULL } }, buf, 1);

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, 15);
    readvolmute(buf, &vol, NULL);

    spawn_capture(&(Arg){ .v = wpctlsinki }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    vol = vol * 100 / 3 * 2;
    sprintf(buf, 
        "INT:value:%d",
        (int)vol
    );
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        "-h", buf,
        (char)arg->i == '+' ? "󰕾 +++++                  |" : "󰕾 -----                  |",
        name, NULL
    } });
}

void bar_audio_toggle_sink(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    double vol = 0;
    int mute = 0;

    spawn_capture(&(Arg){ .v = wpctlsinkm }, buf, 1);
    spawn_capture(&(Arg){ .v = wpctlsinki }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, 15);
    readvolmute(buf, &vol, &mute);
    // fprintf(stderr, "bar_audio_toggle_sink: %d %s\n", mute, buf);
    if (mute) {
        spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
            "󰝟 XXXXX", name, NULL
        } });
        return;
    }

    vol = vol * 100 / 3 * 2;
    sprintf(buf, 
        "INT:value:%d",
        (int)vol
    );
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        "-h", buf,
        "󰕾 )))))                  |",
        name, NULL
    } });
}

void bar_audio_toggle_source(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    int mute = 0;

    spawn_capture(&(Arg){ .v = wpctlsourcem }, buf, 1);

    spawn_capture(&(Arg){ .v = wpctlsourcei }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    spawn_capture(&(Arg){ .v = wpctlsource }, buf, 15);
    readvolmute(buf, NULL, &mute);
    // fprintf(stderr, "bar_audio_toggle_source: %d %s\n", mute, buf);
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        mute ? " XXXXX" : " (((((",
        name, NULL
    } });
}

int bar_audio_click(Bar *bar, Arg *arg, BarArg *a)
{
    audio_dirty = 1;
    if (arg->i == Button1) bar_audio_toggle_source(NULL);
    else if (arg->i == Button2) {
        if (killwiremix()) return -1;
        spawn(&(Arg){ .v = wiremix });
    }
    else if (arg->i == Button3) bar_audio_toggle_sink(NULL);
    else if (arg->i == Button4) bar_audio_change(&(Arg){ .i = (int)'+' });
    else if (arg->i == Button5) bar_audio_change(&(Arg){ .i = (int)'-' });
    return -1;
}
#pragma endregion audio

#pragma region bat
static int bat_dirty = 1;
static double bat_width = 2.3;
static char bat_txt[8] = "XX";
static unsigned int bat_color = 1;
static unsigned int bat_perfi = 0;
static char bat_wasdischarging = 0;

static void
setpower(char* mode, char* notif) {
	spawn(&(Arg){ .v = (char*[]){ "powerprofilesctl", "set", mode, NULL } });
	spawn(&(Arg){ .v = (char*[]){ "notify-send", "-h", "STRING:x-dunst-stack-tag:power", "-u", "low", notif, NULL } });
}

int bar_bat_width(Bar *bar, BarArg *a)
{
	if (!bat_txt[0]) return 0;
	return bat_width*fonth;
}

int bar_bat_draw(Bar *bar, BarArg *a)
{
	if (!bat_txt[0]) return 0;
	drw_setscheme(drw, scheme[(unsigned int)(bat_color-1)]);

	int lpad = (bat_width*fonth - drw_fontset_getwidth(drw, bat_txt, 0)) / 2;
	int res = drw_text(drw, a->x, a->y, a->w, a->h, lpad, bat_txt, 0, 1);

	unsigned int w = a->w * ((double)(bat_perfi + 1) / 3);
	unsigned int x = a->x + (a->w - w) / 2;
	unsigned int h = 1 + bat_perfi;
	drw_rect(
		drw,
		x, a->y + a->h - h,
		w, h,
		1, 0
	);
	return res;
}

int bar_bat_click(Bar *bar, Arg *arg, BarArg *a)
{
	bat_dirty = 1;
	if (arg->i == Button1) setpower("balanced", "󱐋 balanced");
	else if (arg->i == Button2) setpower("performance", "󱐋 performance");
	else if (arg->i == Button3) setpower("power-saver", "󱐋 power-saver");
	return -1;
}

void bat_update(void) {
	if (!bat_txt[0]) return;

	FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
	if (!f) {
		fprintf(stderr, "cannot fopen(/sys/class/power_supply/BAT0/capacity)\n");
		bat_txt[0] = 0;
		return;
	}
	uint64_t cap = 0;
	fscanf(
		f,
		"%" SCNu64,
		&cap
	);
	fclose(f);
	cap = CLAMP(0, cap, 99);

	f = fopen("/sys/class/power_supply/BAT0/status", "r");
	if (!f) {
		fprintf(stderr, "cannot fopen(/sys/class/power_supply/BAT0/status)\n");
		snprintf(bat_txt, 5, "ES");
		return;
	}
	char status[3] = "xx";
	fgets(
		status,
		sizeof(status),
		f
	);
	fclose(f);

	f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference", "r");
	if (!f) {
		fprintf(stderr, "cannot fopen(/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference)\n");
		snprintf(bat_txt, 5, "EP");
		return;
	}
	char perf[3] = "xx";
	fgets(
		perf,
		sizeof(perf),
		f
	);
	fclose(f);

	bat_color = 1;
	const int ischarging = !strncmp(status, "C", 1);
	const int isdischarging = !strncmp(status, "D", 1);
	if (ischarging) bat_color = 6;
	else if (cap < 10) {
		bat_color = 9;
		char notif[32] = {0};
		snprintf(notif, 32, "󱃍 %d%% charge left !!!", (int)cap);
		spawn(&(Arg){ .v = (char*[]){ "notify-send", "-t", "2000", "-u", "critical", notif, NULL } });
	}
	else if (cap < 20) bat_color = 9;
	else if (cap < 30) bat_color = 2;
	else if (isdischarging) bat_color = 8;

	if (bat_wasdischarging != isdischarging) {
		if (isdischarging) setpower("power-saver", "󱐋 power-saver");
		else setpower("balanced", "󱐋 balanced");
		bat_wasdischarging = isdischarging;
		bat_dirty = 1;
	}

	bat_perfi = 0;
	if (!strncmp(perf, "b", 1)) bat_perfi = 1;
	else if (!strncmp(perf, "pe", 2)) bat_perfi = 2;

	snprintf(bat_txt, sizeof(bat_txt),
		"%02d",
		(int)cap
	);
}
#pragma endregion bat

#pragma region floatpos
void
getfloatpos(int pos, char pCh, int size, char sCh, int min_p, int max_s, int cp, int cs, int cbw, int defgrid, int *out_p, int *out_s)
{
	int abs_p, abs_s, i, delta, rest;

	abs_p = pCh == 'A' || pCh == 'a';
	abs_s = sCh == 'A' || sCh == 'a';

	cs += 2*cbw;

	switch(pCh) {
	case 'A': // absolute position
		cp = pos;
		break;
	case 'a': // absolute relative position
		cp += pos;
		break;
	case 'y':
	case 'x': // client relative position
		cp = MIN(cp + pos, min_p + max_s);
		break;
	case 'Y':
	case 'X': // client position relative to monitor
		cp = min_p + MIN(pos, max_s);
		break;
	case 'S': // fixed client position (sticky)
	case 'C': // fixed client position (center)
	case 'Z': // fixed client right-hand position (position + size)
		if (pos == -1)
			break;
		pos = MAX(MIN(pos, max_s), 0);
		if (pCh == 'Z')
			cs = abs((cp + cs) - (min_p + pos));
		else if (pCh == 'C')
			cs = abs((cp + cs / 2) - (min_p + pos));
		else
			cs = abs(cp - (min_p + pos));
		cp = min_p + pos;
		sCh = 0; // size determined by position, override defined size
		break;
	case 'G': // grid
		if (pos <= 0)
			pos = defgrid; // default configurable
		if (size == 0 || pos < 2 || (sCh != 'p' && sCh != 'P'))
			break;
		delta = (max_s - cs) / (pos - 1);
		rest = max_s - cs - delta * (pos - 1);
		if (sCh == 'P') {
			if (size < 1 || size > pos)
				break;
			cp = min_p + delta * (size - 1);
		} else {
			for (i = 0; i < pos && cp >= min_p + delta * i + (i > pos - rest ? i + rest - pos + 1 : 0); i++);
			cp = min_p + delta * (MAX(MIN(i + size, pos), 1) - 1) + (i > pos - rest ? i + rest - pos + 1 : 0);
		}
		break;
	}

	switch(sCh) {
	case 'A': // absolute size
		cs = size;
		break;
	case 'a': // absolute relative size
		cs = MAX(1, cs + size);
		break;
	case '%': // client size percentage in relation to monitor window area size
		if (size <= 0)
			break;
		size = max_s * MIN(size, 100) / 100;
		/* falls through */
	case 'h':
	case 'w': // size relative to client
		if (sCh == 'w' || sCh == 'h') {
			if (size == 0)
				break;
			size += cs;
		}
		/* falls through */
	case 'H':
	case 'W': // normal size, position takes precedence
		if (pCh == 'S' && cp + size > min_p + max_s)
			size = min_p + max_s - cp;
		else if (size > max_s)
			size = max_s;

		if (pCh == 'C') { // fixed client center, expand or contract client
			delta = size - cs;
			if (delta < 0 || (cp - delta / 2 + size <= min_p + max_s))
				cp -= delta / 2;
			else if (cp - delta / 2 < min_p)
				cp = min_p;
			else if (delta)
				cp = min_p + max_s;
		} else if (pCh == 'Z')
			cp -= size - cs;

		cs = size;
		break;
	}

	if (pCh == '%') // client mid-point position in relation to monitor window area size
		cp = min_p + max_s * MAX(MIN(pos, 100), 0) / 100 - (cs) / 2;
	if (pCh == 'm' || pCh == 'M')
		cp = pos - cs / 2;

	if (!abs_p && cp < min_p)
		cp = min_p;
	if (cp + cs > min_p + max_s && !(abs_p && abs_s)) {
		if (abs_p || cp == min_p)
			cs = min_p + max_s - cp;
		else
			cp = min_p + max_s - cs;
	}

	*out_p = cp;
	*out_s = MAX(cs - 2*cbw, 1);
}

void
setfloatpos(Client *c, const char *floatpos)
{
	char xCh, yCh, wCh, hCh;
	int x, y, w, h, wx, ww, wy, wh;

	if (!c || !floatpos)
		return;
	if (selmon->lt[selmon->sellt]->arrange && !c->isfloating)
		return;

	switch(sscanf(floatpos, "%d%c %d%c %d%c %d%c", &x, &xCh, &y, &yCh, &w, &wCh, &h, &hCh)) {
		case 4:
			if (xCh == 'w' || xCh == 'W') {
				w = x; wCh = xCh;
				h = y; hCh = yCh;
				x = -1; xCh = 'C';
				y = -1; yCh = 'C';
			} else if (xCh == 'p' || xCh == 'P') {
				w = x; wCh = xCh;
				h = y; hCh = yCh;
				x = 0; xCh = 'G';
				y = 0; yCh = 'G';
			} else if (xCh == 'm' || xCh == 'M') {
				getrootptr(&x, &y);
			} else {
				w = 0; wCh = 0;
				h = 0; hCh = 0;
			}
			break;
		case 8:
			if (xCh == 'm' || xCh == 'M')
				getrootptr(&x, &y);
			break;
		default:
			return;
	}

	wx = c->mon->wx;
	wy = c->mon->wy;
	ww = c->mon->ww;
	wh = c->mon->wh;

	getfloatpos(x, xCh, w, wCh, wx, ww, c->x, c->w, c->bw, floatposgrid_x, &c->x, &c->w);
	getfloatpos(y, yCh, h, hCh, wy, wh, c->y, c->h, c->bw, floatposgrid_y, &c->y, &c->h);
}

void
floatpos(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!c || (selmon->lt[selmon->sellt]->arrange && !c->isfloating))
		togglefloating(NULL);

	setfloatpos(c, (char *)arg->v);
	resizeclient(c, c->x, c->y, c->w, c->h);

	XRaiseWindow(dpy, c->win);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
}
#pragma endregion floatpos

#pragma region systray
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

static Systray *systray = NULL;
static unsigned long systrayorientation = _NET_SYSTEM_TRAY_ORIENTATION_HORZ;
static int refresh_systray_icons = 0;

static int
bar_systray_width(Bar *bar, BarArg *a)
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
bar_systray_draw(Bar *bar, BarArg *a)
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
bar_systray_click(Bar *bar, Arg *arg, BarArg *a)
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
#pragma endregion systray

#pragma region indicators
#define TAGSPX 5        // # pixels for tag grid boxes
#define TAGSROWS 3      // # rows in tag grid (9 tags, e.g. 3x3)

static void
drawindicator(Monitor *m, Client *c, unsigned int occ, int x, int y, int w, int h, unsigned int tag, int filled, int invert, int type)
{
	int i, boxw, boxs, indn = 0;
	if (!(occ & 1 << tag) || type == INDICATOR_NONE)
		return;

	boxs = drw->fonts->h / 8;
	boxw = drw->fonts->h / 5 + 2;

	switch (type) {
	default:
	case INDICATOR_TOP_LEFT_SQUARE:
		drw_rect(drw, x + boxs, y + boxs, boxw, boxw, filled, invert);
		break;
	case INDICATOR_TOP_LEFT_LARGER_SQUARE:
		drw_rect(drw, x + boxs + 2, y + boxs+1, boxw+1, boxw+1, filled, invert);
		break;
	case INDICATOR_TOP_BAR:
		drw_rect(drw, x + boxw, y, w - ( 2 * boxw + 1), boxw/2, filled, invert);
		break;
	case INDICATOR_TOP_BAR_SLIM:
		drw_rect(drw, x + boxw, y, w - ( 2 * boxw + 1), 1, 0, invert);
		break;
	case INDICATOR_BOTTOM_BAR:
		drw_rect(drw, x + boxw, y + h - boxw/2 -1, w - ( 2 * boxw + 1), boxw/2 +1, filled, invert);
		break;
	case INDICATOR_BOTTOM_BAR_SLIM:
		drw_rect(drw, x + boxw, y + h - 1, w - ( 2 * boxw + 1), 1, 0, invert);
		break;
	case INDICATOR_BOX:
		drw_rect(drw, x + boxw, y, w - 2 * boxw, h, 0, invert);
		break;
	case INDICATOR_BOX_WIDER:
		drw_rect(drw, x + boxw/2, y, w - boxw, h, 0, invert);
		break;
	case INDICATOR_BOX_FULL:
		drw_rect(drw, x, y, w, h, 0, invert);
		break;
	case INDICATOR_CLIENT_DOTS:
		for (c = m->clients; c; c = c->next) {
			if (c->tags & (1 << tag)) {
				drw_rect(drw, x, 1 + (indn * 2), m->sel == c ? 6 : 1, 1, 1, invert);
				indn++;
			}
			if (h <= 1 + (indn * 2)) {
				indn = 0;
				x += 2;
			}
		}
		break;
	case INDICATOR_RIGHT_TAGS:
		if (!c)
			break;
		for (i = 0; i < LENGTH(tags); i++) {
			drw_rect(drw,
				( x + w - 2 - ((LENGTH(tags) / TAGSROWS) * TAGSPX)
					- (i % (LENGTH(tags)/TAGSROWS)) + ((i % (LENGTH(tags) / TAGSROWS)) * TAGSPX)
				),
				( y + 2 + ((i / (LENGTH(tags)/TAGSROWS)) * TAGSPX)
					- ((i / (LENGTH(tags)/TAGSROWS)))
				),
				TAGSPX, TAGSPX, (c->tags >> i) & 1, 0
			);
		}
		break;
	case INDICATOR_PLUS_AND_LARGER_SQUARE:
		boxs += 2;
		boxw += 2;
		/* falls through */
	case INDICATOR_PLUS_AND_SQUARE:
		drw_rect(drw, x + boxs, y + boxs, boxw % 2 ? boxw : boxw + 1, boxw % 2 ? boxw : boxw + 1, filled, invert);
		/* falls through */
	case INDICATOR_PLUS:
		if (!(boxw % 2))
			boxw += 1;
		drw_rect(drw, x + boxs + boxw / 2, y + boxs, 1, boxw, filled, invert); // |
		drw_rect(drw, x + boxs, y + boxs + boxw / 2, boxw + 1, 1, filled, invert); // ‒
		break;
	}
}
#pragma endregion indicators

#pragma region bar_util
typedef struct CpuStat {
    uint64_t total;
    uint64_t idle;
} CpuStat;

static int system_stats_width = 5;
static CpuStat precpustat = {0};
static CpuStat curcpustat = {0};
static char system_stats_txt[12] = "  00 ~ 00  ";
static unsigned int system_stats_color = 1;

int bar_util_width(Bar *bar, BarArg *a)
{
    return system_stats_width*fonth;
}

int bar_util_draw(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(system_stats_color-1)]);

    int lpad = (system_stats_width*fonth - drw_fontset_getwidth(drw, system_stats_txt, 0)) / 2;
    return drw_text(drw, a->x, a->y, a->w, a->h, lpad, system_stats_txt, 0, 1);
}

int bar_util_click(Bar *bar, Arg *arg, BarArg *a)
{
    return -1;
}

static int bar_util_read_cpu_stat(CpuStat *out) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        fprintf(stderr, "cannot fopen(/proc/stat)");
        return 0;
    }

    uint64_t v[10] = {0};
    fscanf(
        f,
        "cpu  %" SCNu64 " %" SCNu64 " %" SCNu64
        " %" SCNu64 " %" SCNu64 " %" SCNu64
        " %" SCNu64 " %" SCNu64 " %" SCNu64
        " %" SCNu64,
        &v[0], &v[1], &v[2], &v[3], &v[4],
        &v[5], &v[6], &v[7], &v[8], &v[9]
    );
    fclose(f);

    uint64_t total = 0;
    for (size_t i = 0; i < 10; ++i) {
        total += v[i];
    }

    // idle + iowait
    uint64_t idle = v[3] + v[4];
    out->total = total;
    out->idle = idle;

    return 0;
}

static int bar_util_calculate_cpu_usage_percent(
    const CpuStat *prev,
    const CpuStat *current
) {
    uint64_t delta_total =
        current->total - prev->total;

    uint64_t delta_idle =
        current->idle - prev->idle;

    if (delta_total == 0) {
        return 0;
    }

    double usage = 100.0 * (1.0 - ((double)delta_idle / (double)delta_total));

    return CLAMP(0, (int)ceil(usage), 99);
}

static int bar_util_read_mem_usage_percent(void) {
    FILE *f = fopen("/proc/meminfo", "r");

    if (!f) {
        fprintf(stderr, "cannot fopen(/proc/meminfo)");
        return 0;
    }

    char key[64];
    uint64_t value;
    uint64_t mem_total = 0;
    uint64_t mem_avail = 0;
    while (
        fscanf(
            f,
            "%63s %" SCNu64 " kB",
            key,
            &value
        ) == 2
    ) {
        if (strcmp(key, "MemTotal:") == 0) {
            mem_total = value;
        } else if (
            strcmp(key, "MemAvailable:") == 0
        ) {
            mem_avail = value;
        }

        if (mem_total && mem_avail) {
            break;
        }
    }
    fclose(f);

    if (mem_total == 0) {
        fprintf(stderr, "MemTotal not found\n");
        return 0;
    }

    double mem_used = 100.0 * (mem_total - mem_avail) / mem_total;

    return CLAMP(0, (int)ceil(mem_used), 99);
}

void bar_util_update(void) {
    bar_util_read_cpu_stat(&curcpustat);
    const int cpu = bar_util_calculate_cpu_usage_percent(&precpustat, &curcpustat);
    precpustat = curcpustat;
    
    const int mem = bar_util_read_mem_usage_percent();
    system_stats_color = 1;
    if (mem > 90) system_stats_color = 9;
    else if (mem > 85) system_stats_color = 2;

    snprintf(system_stats_txt, sizeof(system_stats_txt),
        "  %02d ~ %02d  ",
        cpu, mem
    );
}
#pragma endregion bar_util

#pragma region bar_time
static double time_width = 9;
static char time_txt[21] = "  WEK DD MON HH:MM  ";
static const char* calendar[] = { "ghostty", "--window-padding-x=20,0", "--window-padding-y=20,0", "--title=CALENDAR", "--confirm-close-surface=false", "--cursor-opacity=0", "-e", "bash", "-c", "cal -3c1; exec sleep 100", NULL };
static const char* times[] = { "ghostty", "--window-padding-x=20,0", "--window-padding-y=17,0", "--title=TIMES", "--confirm-close-surface=false", "--cursor-opacity=0", "-e", "bash", "-c", "~/script/times.sh; exec sleep 100", NULL };

static int
killpopups(void) {
    Client* c = selmon->clients;
    while (c) {
        if (strstr(c->name, "CALENDAR") || strstr(c->name, "TIMES")) {
            killclient(&(Arg){ .v = c });
            return 1;
        }
        c = c->next;
    }

    return 0;
}

static int bar_time_width(Bar *bar, BarArg *a)
{
    return time_width*fonth;
}

static int bar_time_draw(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(0)]);

    int lpad = (time_width*fonth - drw_fontset_getwidth(drw, time_txt, 0)) / 2;
    return drw_text(drw, a->x, a->y, a->w, a->h, lpad, time_txt, 0, 1);
}

static int bar_time_click(Bar *bar, Arg *arg, BarArg *a)
{
    if (killpopups()) return -1;

    if (arg->i == Button1) spawn(&(Arg){ .v = calendar });
    if (arg->i == Button3) spawn(&(Arg){ .v = times });
    return -1;
}

static void bar_time_update(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(time_txt, sizeof(time_txt), "  %a %d %b %H:%M  ", tm);
}
#pragma endregion bar_time

#pragma region loop
static size_t sleepcounter = 0;
static const BarUpdateSet barupdates[] = {
	{ 3, bar_util_update, NULL, "system_stats_update" },
	{ 3, bar_time_update, NULL, "bar_time_update" },
	{ 3, bat_update, &bat_dirty, "bat_update" },
	{ 10, audio_update, &audio_dirty, "audio_update" },
};

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
#pragma endregion loop

#pragma region bar_ltsym
static double bar_ltsym_width_fonth = 3;

int
bar_ltsym_width(Bar *bar, BarArg *a)
{
	return bar_ltsym_width_fonth*fonth;
}

int
bar_ltsym_draw(Bar *bar, BarArg *a)
{
	int lpad = (bar_ltsym_width_fonth*fonth - drw_fontset_getwidth(drw, bar->mon->ltsymbol, 0)) / 2;
	if (bar->mon->lt[bar->mon->sellt] == &speciallt) {
		drw_setscheme(drw, scheme[SchemeSel]);
	}
	return drw_text(drw, a->x, a->y, a->w, a->h, lpad, bar->mon->ltsymbol, 0, 0);
}

int
bar_ltsym_click(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkLtSymbol;
}
#pragma endregion bar_ltsym

#pragma region bar_tags
static double bar_tags_width_fonth = 1.8;

static int
bar_tags_width(Bar *bar, BarArg *a)
{
	return fonth*(LENGTH(tags))*bar_tags_width_fonth;
}

static int
bar_tags_draw(Bar *bar, BarArg *a)
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
		w = bar_tags_width_fonth*fonth;
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

static int
bar_tags_click(Bar *bar, Arg *arg, BarArg *a)
{
	int i = 0, x = 0;

	do {
		x += bar_tags_width_fonth*fonth;
	} while (a->x >= x && ++i < LENGTH(tags));
	if (i < LENGTH(tags)) {
		arg->ui = 1 << i;
	}
	return ClkTagBar;
}
#pragma endregion bar_tags

#pragma region bar_wintitle
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

static int
bar_wintitle_width(Bar *bar, BarArg *a)
{
	return a->w;
}

static int
bar_wintitle_draw(Bar *bar, BarArg *a)
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

static int
bar_wintitle_click(Bar *bar, Arg *arg, BarArg *a)
{
	return ClkWinTitle;
}
#pragma endregion bar_wintitle

#pragma region restart
static int restart = 0;

static void
sighup(int unused)
{
	Arg a = {.i = 1};
	quit(&a);
}

static void
sigterm(int unused)
{
	Arg a = {.i = 0};
	quit(&a);
}

int
getlayoutindex(const Layout *layout)
{
	int i;

	for (i = 0; i < LENGTH(layouts) && &layouts[i] != layout; i++);
	if (i == LENGTH(layouts))
		i = 0;
	return i;
}

void
setmonitorfields(Monitor *m)
{
	char atom[22] = {0};
	Atom monitor_fields;

	sprintf(atom, "_DWM_MONITOR_FIELDS_%u", m->num);
	monitor_fields = XInternAtom(dpy, atom, False);

	/* Perists workspace information in 32 bits laid out like this:
	 *
	 * |0|0000|0|0000|0000|0000|0000|0000|000|000
	 * | |    | |    |    |    |    |    |   |-- nmaster
	 * | |    | |    |    |    |    |    |-- nstack
	 * | |    | |    |    |    |    |-- layout
	 * | |    | |    |    |    |-- flextile LAYOUT (split)
	 * | |    | |    |    |-- flextile MASTER
	 * | |    | |    |-- flextile STACK1
	 * | |    | |-- flextile STACK2
	 * | |    |-- flextile mirror layout (indicated by negative layout)
	 * | |
	 * | |-- reserved
	 * |-- showbar
	 */
	uint32_t data[] = {
		(m->nmaster & 0x7) |
		(getlayoutindex(m->lt[m->sellt]) & 0xF) << 6 |
		m->showbar << 31
	};

	XChangeProperty(dpy, root, monitor_fields, XA_CARDINAL, 32, PropModeReplace,
		(unsigned char *)data, 1);
}

void
setmonitortags(Monitor *m)
{
	char atom[22] = {0};
	Atom monitor_tags;

	sprintf(atom, "_DWM_MONITOR_TAGS_%u", m->num);
	monitor_tags = XInternAtom(dpy, atom, False);

	uint32_t data[] = { m->tagset[m->seltags] };
	XChangeProperty(dpy, root, monitor_tags, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setclienttags(Client *c)
{
	uint32_t data[] = { c->tags };
	XChangeProperty(dpy, c->win, clientatom[ClientTags], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

void
setclientfields(Client *c)
{
	/* Perists client information in 32 bits laid out like this:
	 *
	 * |00000000|00000|0|0|0|0|0|0|0|0|00000000|000
	 * |        |     | | | | | | | | |        |-- monitor index
	 * |        |     | | | | | | | | |-- client index
	 * |        |     | | | | | | | |-- isfloating
	 * |        |     | | | | | | |-- ispermanent
	 * |        |     | | | | | |-- isterminal
	 * |        |     | | | | |-- noswallow
	 * |        |     | | | |-- issteam
	 * |        |     | | |-- issticky
	 * |        |     | |-- fakefullscreen
	 * |        |     |-- isfreesize
	 * |        |
	 * |        |-- reserved
	 * |-- scratchkey (for scratchpads)
	 */
	uint32_t data[] = {
		(c->mon->num & 0x7)
		| (c->idx & 0xFF) << 3
		| (c->isfloating & 0x1) << 11
	};
	XChangeProperty(dpy, c->win, clientatom[ClientFields], XA_CARDINAL, 32, PropModeReplace, (unsigned char *)data, 1);
}

static void
persistmonitorstate(Monitor *m)
{
	Client *c;
	unsigned int i;

	setmonitortags(m);
	setmonitorfields(m);

	/* Set client atoms */
	for (i = 1, c = m->clients; c; c = c->next, ++i) {
		c->idx = i;
		setclienttags(c);
		setclientfields(c);
	}
}

int
getmonitortags(Monitor *m)
{
	int di;
	unsigned long dl, nitems;
	unsigned char *p = NULL;
	char atom[22] = {0};
	Atom da, monitor_tags = None, tagsat;

	sprintf(atom, "_DWM_MONITOR_TAGS_%u", m->num);
	monitor_tags = XInternAtom(dpy, atom, False);

	if (!(XGetWindowProperty(dpy, root, monitor_tags, 0L, sizeof dl,
			False, AnyPropertyType, &da, &di, &nitems, &dl, &p) == Success && p)) {
		return 0;
	}

	if (nitems) {
		tagsat = *(Atom *)p;
		m->tagset[m->seltags] = tagsat & TAGMASK;
	}

	XFree(p);
	return 1;
}

int
getmonitorfields(Monitor *m)
{
	int di, layout_index;
	unsigned long dl, nitems;
	unsigned char *p = NULL;
	char atom[22] = {0};
	Atom da, state = None;

	sprintf(atom, "_DWM_MONITOR_FIELDS_%u", m->num);
	Atom dwm_monitor = XInternAtom(dpy, atom, False);
	if (!dwm_monitor)
		return 0;

	if (!(XGetWindowProperty(dpy, root, dwm_monitor, 0L, sizeof dl,
			False, AnyPropertyType, &da, &di, &nitems, &dl, &p) == Success && p)) {
		return 0;
	}

	if (nitems) {
		state = *(Atom *)p;

		/* See bit layout in the persistmonitorstate function */
		m->nmaster = state & 0x7;
		layout_index = (state >> 6) & 0xF;
		if (layout_index < LENGTH(layouts))
			m->lt[m->sellt] = &layouts[layout_index];
		m->showbar = (state >> 31) & 0x1;
	}

	XFree(p);
	return 1;
}

static int
restoremonitorstate(Monitor *m)
{
	return getmonitortags(m) | getmonitorfields(m);
}

int
getclienttags(Client *c)
{
	Atom tagsat = getatomprop(c, clientatom[ClientTags], AnyPropertyType);
	if (tagsat == None)
		return 0;

	c->tags = tagsat & TAGMASK;
	return 1;
}

int
getclientfields(Client *c)
{
	Monitor *m;
	Atom fields = getatomprop(c, clientatom[ClientFields], AnyPropertyType);
	if (fields == None)
		return 0;

	/* See bit layout in the setclientfields function */
	for (m = mons; m; m = m->next)
		if (m->num == (fields & 0x7)) {
			c->mon = m;
			break;
		}
	c->idx = (fields >> 3) & 0xFF;
	c->isfloating = (fields >> 11) & 0x1;
	return 1;
}

static int
restoreclientstate(Client *c)
{
	int restored = getclientfields(c);
	getclienttags(c);
	return restored;
}
#pragma endregion restart

#pragma region dwm
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[KeyRelease] = keyrelease,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[ResizeRequest] = resizerequest,
	[UnmapNotify] = unmapnotify
};

int
streqeq(const char* a, const char* b) {
	int i = 0;
	while (a[i] && a[i] == b[i]) i++;
	return a[i] == b[i];
}

void
applyrules(Client *c)
{
	const char *class, *instance;
	Atom wintype;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;
	wintype  = getatomprop(c, netatom[NetWMWindowType], XA_ATOM);

	// fprintf(stderr, "applyrules: %s : %s : %s\n", instance, class, c->name);
	if (streqeq(instance, "steamwebhelper") && streqeq(class, "steam") && !streqeq(c->name, "Steam")) {
		c->isfloating = 1;
	}

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if (
			(!r->title || strstr(c->name, r->title))
			&& (!r->class || strstr(class, r->class))
			&& (!r->instance || strstr(instance, r->instance))
			&& (!r->wintype || wintype == XInternAtom(dpy, r->wintype, False))
		) {
			c->iscentered = r->iscentered;
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
			if (c->isfloating && r->floatpos) {
				c->iscentered = 0;
				setfloatpos(c, r->floatpos);
			}

		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : c->mon->tagset[c->mon->seltags];
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

static Arg
shift(const Arg *arg, int clients)
{
	Arg shifted;
	Client *c;
	unsigned int tagmask = 0;

	shifted.ui = selmon->tagset[selmon->seltags];

	for (c = selmon->clients; c && clients; c = c->next) {
		if (c == selmon->sel)
			continue;
		tagmask |= c->tags;
	}

	do {
		if (arg->i > 0) // left circular shift
			shifted.ui = (shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i));
		else // right circular shift
			shifted.ui = (shifted.ui >> -arg->i) | (shifted.ui << (LENGTH(tags) + arg->i));
	} while (tagmask && !(shifted.ui & tagmask));

	return shifted;
}

void
shifttag(const Arg *arg)
{
	Arg shifted = shift(arg, 0);
	tag(&shifted);
}

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

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachx(Client *c)
{
	Client *at;

	if (c->idx > 0) { /* then the client has a designated position in the client list */
		for (at = c->mon->clients; at; at = at->next) {
			if (c->idx < at->idx) {
				c->next = at;
				c->mon->clients = c;
				return;
			} else if (at->idx <= c->idx && (!at->next || c->idx <= at->next->idx)) {
				c->next = at->next;
				at->next = c;
				return;
			}
		}
	}

	attach(c); // master (default)
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
	int click, i, r;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	Bar *bar;
	XButtonPressedEvent *ev = &e->xbutton;
	const BarRule *br;
	BarArg carg = { 0, 0, 0, 0 };
	click = ClkRootWin;
	arg.i = ev->button;

	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon
	) {
		unfocus(selmon->sel, 1, NULL);
		selmon = m;
		focus(NULL);
	}

	for (bar = selmon->bar; bar; bar = bar->next) {
		if (ev->window == bar->win) {
			for (r = 0; r < LENGTH(barrules); r++) {
				br = &barrules[r];
				if (br->bar != bar->idx || (br->monitor == 'A' && m != selmon) || br->clickfunc == NULL)
					continue;
				if (br->monitor != 'A' && br->monitor != -1 && br->monitor != bar->mon->num)
					continue;
				if (bar->x[r] <= ev->x && ev->x <= bar->x[r] + bar->w[r]) {
					carg.x = ev->x - bar->x[r];
					carg.y = ev->y - bar->borderpx;
					carg.w = bar->w[r];
					carg.h = bar->bh - 2 * bar->borderpx;
					click = br->clickfunc(bar, &arg, &carg);
					if (click < 0)
						return;
					break;
				}
			}
			break;
		}
	}

	if (click == ClkRootWin && (c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}

	btnpressed = ev->button;
	for (i = 0; i < LENGTH(buttons); i++) {
		if (
			click == buttons[i].click &&
			buttons[i].func &&
			buttons[i].button == ev->button &&
			CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state) &&
			(!buttons[i].layout || buttons[i].layout == selmon->lt[selmon->sellt])
		) {
			buttons[i].func(
				(
					click == ClkTagBar
				) && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg
			);
		}
	}

}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Monitor *m;
	Layout foo = { "", NULL };
	size_t i;

	for (m = mons; m; m = m->next)
		persistmonitorstate(m);

	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	if (showsystray && systray) {
		while (systray->icons)
			removesystrayicon(systray->icons);
		if (systray->win) {
			XUnmapWindow(dpy, systray->win);
			XDestroyWindow(dpy, systray->win);
		}
		free(systray);
	}
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);

}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;
	Bar *bar;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	for (bar = mon->bar; bar; bar = mon->bar) {
		if (!bar->external) {
			XUnmapWindow(dpy, bar->win);
			XDestroyWindow(dpy, bar->win);
		}
		mon->bar = bar->next;
		if (systray && bar == systray->bar)
			systray->bar = NULL;
		free(bar);
	}
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XWindowAttributes wa;
	XSetWindowAttributes swa;
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (cme->message_type == wmatom[WMStatusRedraw]) {
		drawbars();
		return;
	}

	if (showsystray && systray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		if (cme->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK) {
			if (!(c = (Client *)calloc(1, sizeof(Client))))
				die("fatal: could not malloc() %u bytes\n", sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}

			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			XGetWindowAttributes(dpy, c->win, &wa);
			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			c->isfloating = True;
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			XClassHint ch = {"dwmsystray", "dwmsystray"};
			XSetClassHint(dpy, c->win, &ch);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			/* use parents background color */
			swa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			sendevent(c->win, xatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_EMBEDDED_NOTIFY, 0 , systray->win, XEMBED_EMBEDDED_VERSION);
			XSync(dpy, False);
			setclientstate(c, NormalState);
		}
		return;
	}

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen]) {
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */
				&& !c->isfullscreen
			)));
		}
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	// fprintf(stderr, "configure: %s\n", c->name);
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	// fprintf(stderr, "configure: %d : %d : %d : %d %s\n", c->x, c->y, c->w, c->h, c->name);

	if (noborder(c)) {
		ce.width += c->bw * 2;
		ce.height += c->bw * 2;
		ce.border_width = 0;
	}

	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Bar *bar;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;
	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, sh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				for (bar = m->bar; bar; bar = bar->next)
					XMoveResizeWindow(dpy, bar->win, bar->bx, bar->by, bar->bw, bar->bh);
			}
			arrange(NULL);
			focus(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if (ignoreconfigurerequests)
		return;

	if ((c = wintoclient(ev->window))) {
		// fprintf(stderr, "configurerequest: %d : %s\n", c->isfloating, c->name);
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if (c->isfloating) {
				if (!c->isfullscreen) {
					c->x = m->wx + (m->ww / 2 - WIDTH(c) / 2);  /* center in x direction */
					c->y = m->wy; /* top in y direction */
				} else {
					c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2);  /* center in x direction */
					c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
				}
			}
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m, *mon;
	int i, n, mi, max_bars = 2, istopbar = topbar;

	const BarRule *br;
	Bar *bar;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	for (mi = 0, mon = mons; mon; mon = mon->next, mi++); // monitor index
	m->num = mi;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);

	/* Derive the number of bars for this monitor based on bar rules */
	for (n = -1, i = 0; i < LENGTH(barrules); i++) {
		br = &barrules[i];
		if (br->monitor == 'A' || br->monitor == -1 || br->monitor == m->num)
			n = MAX(br->bar, n);
	}

	m->bar = NULL;
	for (i = 0; i <= n && i < max_bars; i++) {
		bar = ecalloc(1, sizeof(Bar));
		bar->mon = m;
		bar->idx = i;
		bar->next = m->bar;
		bar->topbar = istopbar;
		m->bar = bar;
		istopbar = !istopbar;
		bar->showbar = 1;
		bar->external = 0;
		bar->borderpx = 0;
		bar->bh = bh + bar->borderpx * 2;
		bar->borderscheme = SchemeNorm;
	}

	restoremonitorstate(m);

	return m;
}

void
destroynotify(XEvent *e)
{
	// fprintf(stderr, "destroynotify\n");
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if (showsystray && (c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		drawbarwin(systray->bar);
	}
}

void
detach(Client *c)
{
	Client **tc;
	c->idx = 0;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
	c->next = NULL;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
	c->snext = NULL;
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	Bar *bar;
	
	if (m->showbar)
		for (bar = m->bar; bar; bar = bar->next)
			drawbarwin(bar);
}

void
drawbars(void)
{
	Monitor *m;
	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
drawbarwin(Bar *bar)
{
	if (!bar || !bar->win || bar->external)
		return;
	int r, w, total_drawn = 0;
	int rx, lx, rw, lw; // bar size, split between left and right if a center module is added
	const BarRule *br;

	if (bar->borderpx) {
		XSetForeground(drw->dpy, drw->gc, scheme[bar->borderscheme][ColBorder].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, 0, 0, bar->bw, bar->bh);
	}

	BarArg warg = { 0 };
	BarArg darg  = { 0 };
	warg.h = bar->bh - 2 * bar->borderpx;

	rw = lw = bar->bw - 2 * bar->borderpx;
	rx = lx = bar->borderpx;

	drw_setscheme(drw, scheme[SchemeNorm]);
	drw_rect(drw, lx, bar->borderpx, lw, bar->bh - 2 * bar->borderpx, 1, 1);
	for (r = 0; r < LENGTH(barrules); r++) {
		br = &barrules[r];
		if (br->bar != bar->idx || !br->widthfunc || (br->monitor == 'A' && bar->mon != selmon))
			continue;
		if (br->monitor != 'A' && br->monitor != -1 && br->monitor != bar->mon->num)
			continue;
		drw_setscheme(drw, scheme[SchemeNorm]);
		warg.w = (br->alignment < BAR_ALIGN_RIGHT_LEFT ? lw : rw);

		w = br->widthfunc(bar, &warg);
		w = MIN(warg.w, w);

		if (lw <= 0) { // if left is exhausted then switch to right side, and vice versa
			lw = rw;
			lx = rx;
		} else if (rw <= 0) {
			rw = lw;
			rx = lx;
		}

		switch(br->alignment) {
		default:
		case BAR_ALIGN_NONE:
		case BAR_ALIGN_LEFT_LEFT:
		case BAR_ALIGN_LEFT:
			bar->x[r] = lx;
			if (lx == rx) {
				rx += w;
				rw -= w;
			}
			lx += w;
			lw -= w;
			break;
		case BAR_ALIGN_LEFT_RIGHT:
		case BAR_ALIGN_RIGHT:
			bar->x[r] = lx + lw - w;
			if (lx == rx)
				rw -= w;
			lw -= w;
			break;
		case BAR_ALIGN_LEFT_CENTER:
		case BAR_ALIGN_CENTER:
			bar->x[r] = lx + lw / 2 - w / 2;
			if (lx == rx) {
				rw = rx + rw - bar->x[r] - w;
				rx = bar->x[r] + w;
			}
			lw = bar->x[r] - lx;
			break;
		case BAR_ALIGN_RIGHT_LEFT:
			bar->x[r] = rx;
			if (lx == rx) {
				lx += w;
				lw -= w;
			}
			rx += w;
			rw -= w;
			break;
		case BAR_ALIGN_RIGHT_RIGHT:
			bar->x[r] = rx + rw - w;
			if (lx == rx)
				lw -= w;
			rw -= w;
			break;
		case BAR_ALIGN_RIGHT_CENTER:
			bar->x[r] = rx + rw / 2 - w / 2;
			if (lx == rx) {
				lw = lx + lw - bar->x[r] + w;
				lx = bar->x[r] + w;
			}
			rw = bar->x[r] - rx;
			break;
		}
		bar->w[r] = w;
		darg.x = bar->x[r];
		darg.y = bar->borderpx;
		darg.h = bar->bh - 2 * bar->borderpx;
		darg.w = bar->w[r];
		if (br->drawfunc)
			total_drawn += br->drawfunc(bar, &darg);
	}

	if (total_drawn == 0 && bar->showbar) {
		bar->showbar = 0;
		updatebarpos(bar->mon);
		XMoveResizeWindow(dpy, bar->win, bar->bx, bar->by, bar->bw, bar->bh);
		arrange(bar->mon);
	}
	else if (total_drawn > 0 && !bar->showbar) {
		bar->showbar = 1;
		updatebarpos(bar->mon);
		XMoveResizeWindow(dpy, bar->win, bar->bx, bar->by, bar->bw, bar->bh);
		drw_map(drw, bar->win, 0, 0, bar->bw, bar->bh);
		arrange(bar->mon);
	} else
		drw_map(drw, bar->win, 0, 0, bar->bw, bar->bh);
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1, c);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	// fprintf(stderr, "expose\n");
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
	}
}

void
focus(Client *c)
{
	// fprintf(stderr, "focus\n");
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0, c);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		if (c->isfloating)
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColFloat].pixel);
		else
			XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();

}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0, NULL);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
		if (!c)
			for (c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i) && !i->isfloating)
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i) && !i->isfloating)
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}

Atom
getatomprop(Client *c, Atom prop, Atom req)
{
	int format;
	unsigned long nitems, dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (prop == xatom[XembedInfo])
		req = xatom[XembedInfo];

	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
		&da, &format, &nitems, &dl, &p) == Success && p) {
		if (nitems > 0 && format == 32)
			atom = *(long *)p;
		if (da == xatom[XembedInfo] && dl == 2)
			atom = ((long *)p)[1];
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, &p) != Success)
		return -1;
	if (n != 0 && format == 32)
		result = *(long *)p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (selmon->lt[selmon->sellt] == &speciallt) {
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeAsync, GrabModeSync, None, None);
			return;
		}

		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin && (!buttons[i].layout || buttons[i].layout == selmon->lt[selmon->sellt]))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		if (selmon->lt[selmon->sellt] == &speciallt) {
			XGrabKey(dpy, AnyKey,
				AnyModifier,
				root, True,
				GrabModeAsync, GrabModeAsync);
			return;
		}

		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (!syms) return;

		for (k = start; k <= end; k++) {
			for (i = 0; i < LENGTH(keys); i++) {
				/* skip modifier codes, we do that ourselves */
				if (
					keys[i].keysym == syms[(k - start) * skip] &&
					(!keys[i].layout || keys[i].layout == selmon->lt[selmon->sellt])
				) {
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							 keys[i].mod | modifiers[j],
							 root, True,
							 GrabModeAsync, GrabModeAsync);
				}
			}
		}
		XFree(syms);
	}
}

void
incnmaster(const Arg *arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

void
keypress(XEvent *e)
{
	unsigned int i;
	int keysyms_return;
	KeySym* keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XGetKeyboardMapping(dpy, (KeyCode)ev->keycode, 1, &keysyms_return);
	keypressed = *keysym;
	btnpressed = 0;
	for (i = 0; i < LENGTH(keys); i++) {
		if (*keysym == keys[i].keysym
				&& keys[i].ev == KeyPress
				&& (CLEANMASK(keys[i].mod) == CLEANMASK(ev->state))
				&& (!keys[i].layout || keys[i].layout == selmon->lt[selmon->sellt])
				&& keys[i].func) {
					keys[i].func(&(keys[i].arg));
				}
	}
	XFree(keysym);
}

void
keyrelease(XEvent *e)
{
	unsigned int i;
	int keysyms_return;
	KeySym* keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XGetKeyboardMapping(dpy, (KeyCode)ev->keycode, 1, &keysyms_return);
	for (i = 0; i < LENGTH(keys); i++) {
		if (
			*keysym == keys[i].keysym
			&& keys[i].ev == KeyRelease
			&& keys[i].func
			&& !btnpressed
		) {
			if (keypressed && keypressed != keys[i].keysym) {
				if (keypressed == XK_a || keypressed == XK_d) pop(selmon->sel);
				keypressed = 0;
			} else {
				keys[i].func(&(keys[i].arg));
			}
		}
	}
	XFree(keysym);
}

void
killclient(const Arg *arg)
{
	Client* c = arg->v ? (Client*)arg->v : selmon->sel;
	if (!c)
		return;

	if (c->iskilling || !sendevent(c->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0, 0, 0))
	{
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, c->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	} else {
		c->iskilling = 1;
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	int settings_restored;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;
	settings_restored = restoreclientstate(c);
	updatetitle(c);

	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		c->bw = borderpx;
		c->iscentered = 1;
	} else {
		if (!settings_restored || c->mon == NULL) {
			c->mon = selmon;
			settings_restored = 0;
		}
		if (c->x == c->mon->wx && c->y == c->mon->wy)
			c->iscentered = 1;
		c->bw = borderpx;
		if (!settings_restored)
			applyrules(c);
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	if (c->isfloating)
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColFloat].pixel);
	else
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatesizehints(c);
	updatewmhints(c);

	c->iscentered = c->iscentered || c->isfloating;
	if (c->iscentered) {
		c->x = c->mon->wx + (c->mon->ww - WIDTH(c)) / 2;
		c->y = c->mon->wy;
	}

	if (getatomprop(c, netatom[NetWMState], XA_ATOM) == netatom[NetWMFullscreen])
		setfullscreen(c, 1);

	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);

	c->isfloating = c->isfloating || (c->oldstate = trans != None);
	if (c->isfloating) {
		XRaiseWindow(dpy, c->win);
		XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColFloat].pixel);
	}
	attachx(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */

	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0, c);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);

}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	// fprintf(stderr, "maprequest\n");
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	Client *i;
	if (showsystray && systray && (i = wintosystrayicon(ev->window))) {
		sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, XEMBED_WINDOW_ACTIVATE, 0, systray->win, XEMBED_EMBEDDED_VERSION);
		drawbarwin(systray->bar);
	}

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
barhover(XEvent *e, Bar *bar)
{
	const BarRule *br;
	Monitor *m = bar->mon;
	XMotionEvent *ev = &e->xmotion;
	BarArg barg = { 0, 0, 0, 0 };
	int r;

	for (r = 0; r < LENGTH(barrules); r++) {
		br = &barrules[r];
		if (br->bar != bar->idx || (br->monitor == 'A' && m != selmon) || br->hoverfunc == NULL)
			continue;
		if (br->monitor != 'A' && br->monitor != -1 && br->monitor != bar->mon->num)
			continue;
		if (bar->x[r] > ev->x || ev->x > bar->x[r] + bar->w[r])
			continue;

		barg.x = ev->x - bar->x[r];
		barg.y = ev->y - bar->borderpx;
		barg.w = bar->w[r];
		barg.h = bar->bh - 2 * bar->borderpx;

		br->hoverfunc(bar, &barg, ev);
		break;
	}
}

Bar *
wintobar(Window win)
{
	Monitor *m;
	Bar *bar;
	for (m = mons; m; m = m->next)
		for (bar = m->bar; bar; bar = bar->next)
			if (bar->win == win)
				return bar;
	return NULL;
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	Bar *bar;
	XMotionEvent *ev = &e->xmotion;

	if ((bar = wintobar(ev->window))) {
		barhover(e, bar);
		return;
	}

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1, NULL);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	nx = ocx = c->x;
	ny = ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	ignoreconfigurerequests = 1;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap)) {
				togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating) {
				resize(c, nx, ny, c->w, c->h, 1);
			}
			break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
	ignoreconfigurerequests = 0;

	if (!lasttime) {
		togglefloating(NULL);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

int
noborder(Client *c)
{
	int monocle_layout = 0;

	if (&monocle == c->mon->lt[c->mon->sellt]->arrange)
		monocle_layout = 1;

	if (!monocle_layout && (nexttiled(c->mon->clients) != c || nexttiled(c->next)))
		return 0;

	if (c->isfloating)
		return 0;

	if (!c->mon->lt[c->mon->sellt]->arrange)
		return 0;

	if (c->isfullscreen)
		return 0;

	return 1;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if (showsystray && (c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		}
		else
			updatesystrayiconstate(c, ev);
		drawbarwin(systray->bar);
	}

	if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
		drawbars();
	} else if (ev->state == PropertyDelete) {
		return; /* ignore */
	} else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			if (c->isurgent)
				drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
	}
}

void
quit(const Arg *arg)
{
	restart = arg->i;
	running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	if (noborder(c)) {
		wc.width += c->bw * 2;
		wc.height += c->bw * 2;
		wc.border_width = 0;
	}
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh, nx, ny;
	int opx, opy;
	unsigned int dui;
	Window dummy;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	nx = ocx = c->x;
	ny = ocy = c->y;
	nh = c->h;
	nw = c->w;
	if (!XQueryPointer(dpy, c->win, &dummy, &dummy, &opx, &opy, &nx, &ny, &dui))
		return;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResizeBR]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer (dpy, None, c->win, 0, 0, 0, 0,
			(c->w + c->bw - 1),
			(c->h + c->bw - 1));
	ignoreconfigurerequests = 1;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate_resize))
				continue;
			lasttime = ev.xmotion.time;

			nx = c->x;
			ny = c->y;
			nw = ev.xmotion.x - ocx - 2 * c->bw + 1;
			nh = ev.xmotion.y - ocy - 2 * c->bw + 1;
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap)) {
					togglefloating(NULL);
				}
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating) {
				resize(c, nx, ny, nw, nh, 1);
			}
			break;
		}
	} while (ev.type != ButtonRelease);

	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
			(c->w + c->bw - 1),
			(c->h + c->bw - 1));
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
	ignoreconfigurerequests = 0;
}

void
restack(Monitor *m)
{
	// fprintf(stderr, "restack\n");
	Client *c, *f = NULL;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);

	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		if (m->bar) {
			wc.sibling = m->bar->win;
		} else {
			for (f = m->stack; f && (f->isfloating || !ISVISIBLE(f)); f = f->snext); // find first tiled stack client
			if (f)
				wc.sibling = f->win;
		}
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c) && c != f) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
	XEvent ev;
	XSync(dpy, False);
	/* main event loop */
	while (running) {
		struct pollfd pfd = {
			.fd = ConnectionNumber(dpy),
			.events = POLLIN,
		};
		int pending = XPending(dpy) > 0 || poll(&pfd, 1, -1) > 0;

		if (!running)
			break;
		if (!pending)
			continue;

		XNextEvent(dpy, &ev);
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
	}
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1, NULL);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	if (c->isfullscreen)
		resizeclient(c, m->mx, m->my, m->mw, m->mh);
	arrange(NULL);
	focus(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
{
	int n;
	Atom *protocols;
	Atom mt;
	int exists = 0;
	XEvent ev;

	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
			while (!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	} else {
		exists = True;
		mt = proto;
	}

	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
	}
	XChangeProperty(dpy, root, netatom[NetActiveWindow], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &(c->win), 1);

	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldbw = c->bw;
		c->oldstate = c->isfloating;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->bw = c->oldbw;
		c->isfloating = c->oldstate;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
setlayout(const Arg *arg)
{
	// fprintf(stderr, "setlayout\n");
	if (selmon->sel && selmon->sel->isfullscreen) return;

	if (arg && arg->v && arg->v != selmon->lt[selmon->sellt]) {
		selmon->sellt ^= 1;
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	} else if (!arg || !arg->v || arg->v == selmon->lt[selmon->sellt]) {
		selmon->sellt ^= 1;
	}

	grabkeys();
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel) {
		pop(selmon->sel);
		// arrange(selmon->sel->mon);
	} else {
		drawbar(selmon);
	}
}

static void
dragmfact(const Arg *arg)
{
	unsigned int n;
	int py, px; // pointer coordinates
	int ax, ay, aw, ah; // area position, width and height
	int center = 0, horizontal = 0, mirror = 0, fixed = 0; // layout configuration
	double fact;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	m = selmon;

	Client *c;
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);

	ax = m->wx;
	ay = m->wy;
	ah = m->wh;
	aw = m->ww;

	if (!n)
		return;

	/* do not allow mfact to be modified under certain conditions */
	if (!m->lt[m->sellt]->arrange                            // floating layout
		|| (!fixed && m->nmaster && n <= m->nmaster) // no master
		|| m->lt[m->sellt]->arrange == &monocle
		|| m->lt[m->sellt]->arrange == &gaplessgrid
	)
		return;

	if (center) {
		if (horizontal) {
			px = ax + aw / 2;
			py = ay + ah / 2 + ah * m->mfact / 2.0;
		} else { // vertical split
			px = ax + aw / 2 + aw * m->mfact / 2.0;
			py = ay + ah / 2;
		}
	} else if (horizontal) {
		px = ax + aw / 2;
		if (mirror)
			py = ay + (ah * (1.0 - m->mfact));
		else
			py = ay + (ah * m->mfact);
	} else { // vertical split
		if (mirror)
			px = ax + (aw * m->mfact);
		else
			px = ax + (aw * m->mfact);
		py = ay + ah / 2;
	}

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[horizontal ? CurResizeVertArrow : CurResizeHorzArrow]->cursor, CurrentTime) != GrabSuccess)
		return;

	XWarpPointer(dpy, None, root, 0, 0, 0, 0, px, py);

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate_dragmfact))
				continue;
			if (lasttime != 0) {
				px = ev.xmotion.x;
				py = ev.xmotion.y;
			}
			lasttime = ev.xmotion.time;
			if (center)
				if (horizontal)
					if (py - ay > ah / 2)
						fact = (double) 1.0 - (ay + ah - py) * 2 / (double) ah;
					else
						fact = (double) 1.0 - (py - ay) * 2 / (double) ah;
				else
					if (px - ax > aw / 2)
						fact = (double) 1.0 - (ax + aw - px) * 2 / (double) aw;
					else
						fact = (double) 1.0 - (px - ax) * 2 / (double) aw;
			else
				if (horizontal)
					fact = (double) (py - ay) / (double) ah;
				else
					fact = (double) (px - ax) / (double) aw;

			if (!center && mirror)
				fact = 1.0 - fact;

			setmfact(&((Arg) { .f = 1.0 + fact }));
			px = ev.xmotion.x;
			py = ev.xmotion.y;
			break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = f;
	arrange(selmon);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	/* clean up any zombies (inherited from .xinitrc etc) immediately */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	signal(SIGHUP, sighup);
	signal(SIGTERM, sigterm);

	/* the one line of bloat that would have saved a lot of time for a lot of people */
	putenv("_JAVA_AWT_WM_NONREPARENTING=1");

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	fonth = drw->fonts->h;
	bh = drw->fonts->h + barhextra;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wmatom[WMStatusRedraw] = XInternAtom(dpy, "WM_STATUS_REDRAW", False);
	clientatom[ClientFields] = XInternAtom(dpy, "_DWM_CLIENT_FIELDS", False);
	clientatom[ClientTags] = XInternAtom(dpy, "_DWM_CLIENT_TAGS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetSystemTray] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
	netatom[NetSystemTrayVisual] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
	netatom[NetWMWindowTypeDock] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	xatom[Manager] = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed] = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo] = XInternAtom(dpy, "_XEMBED_INFO", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurResizeBR] = drw_cur_create(drw, XC_bottom_right_corner);
	cursor[CurResizeBL] = drw_cur_create(drw, XC_bottom_left_corner);
	cursor[CurResizeTR] = drw_cur_create(drw, XC_top_right_corner);
	cursor[CurResizeTL] = drw_cur_create(drw, XC_top_left_corner);
	cursor[CurResizeHorzArrow] = drw_cur_create(drw, XC_sb_h_double_arrow);
	cursor[CurResizeVertArrow] = drw_cur_create(drw, XC_sb_v_double_arrow);
	cursor[CurIronCross] = drw_cur_create(drw, XC_iron_cross);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], ColCount);

	updatebars();
	drawbars();

	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);

	if (pthread_create(&barloopth, NULL, init_bar_loop, NULL) != 0) {
		die("dwm: Could not create barloopth\n");
	}
	pthread_detach(barloopth);

	grabkeys();
	focus(NULL);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating)
			&& !c->isfullscreen
			)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
spawn(const Arg *arg)
{
	struct sigaction sa;

	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;

	if (fork() == 0)
	{
		if (dpy)
			close(ConnectionNumber(dpy));

		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

void
spawn_capture(const Arg *arg, char *output, ssize_t output_size)
{
	int pipefd[2];
	pid_t pid;
	ssize_t readbytes = 0;
	struct sigaction sa;

	if (pipe(pipefd) == -1) {
		perror("pipe");
		return;
	}

	pid = fork();
	if (pid == -1) {
		perror("fork");
		close(pipefd[0]);
		close(pipefd[1]);
		return;
	}

	if (pid == 0)
	{
		if (dpy)
			close(ConnectionNumber(dpy));

		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		close(pipefd[0]);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);

		execvp(((char **)arg->v)[0], (char **)arg->v);
		return;
	} 
	else
	{
		close(pipefd[1]);
		while (output_size - readbytes > 0) {
			ssize_t bytes = read(pipefd[0], output + readbytes, output_size - readbytes);
			if (bytes <= 0) break;

			readbytes += bytes;
		}

		close(pipefd[0]);
	}

	return;
}

void
tag(const Arg *arg)
{

	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		arrange(selmon);
		focus(NULL);
	}
}

void
tagmon(const Arg *arg)
{
	Client *c = selmon->sel;
	Monitor *dest;
	if (!c || !mons->next)
		return;
	dest = dirtomon(arg->i);
	sendmon(c, dest);
}

void
togglebar(const Arg *arg)
{
	Bar *bar;
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	for (bar = selmon->bar; bar; bar = bar->next)
		XMoveResizeWindow(dpy, bar->win, bar->bx, bar->by, bar->bw, bar->bh);
	if (!selmon->showbar && systray)
		XMoveWindow(dpy, systray->win, -32000, -32000);
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	Client *c = selmon->sel;
	if (arg && arg->v)
		c = (Client*)arg->v;
	if (!c)
		return;
	if (c->isfullscreen) /* no support for fullscreen windows */
		return;
	c->isfloating = !c->isfloating || c->isfixed;
	resizeclient(c, c->x, c->y, c->w, c->h); // hack to make sure borders update correctly
	if (c->isfloating)
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColFloat].pixel);
	else
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
	if (c->isfloating) {
		resize(c, c->x, c->y, c->w, c->h, 0);
	}
	arrange(c->mon);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		arrange(selmon);
		focus(NULL);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);;

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;

		arrange(selmon);
		focus(NULL);
	}
}

void
unfocus(Client *c, int setfocus, Client *nextfocus)
{
	if (!c)
		return;

	grabbuttons(c, 0);
	if (c->isfloating)
		XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColFloat].pixel);
	else
		XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m;
	XWindowChanges wc;

	m = c->mon;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!HIDDEN(c))
			setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}

	free(c);
	arrange(m);
	focus(NULL);
	updateclientlist();
}

void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		drawbarwin(systray->bar);
	}
}

void
unmapnotify(XEvent *e)
{
	// fprintf(stderr, "unmapnotify\n");
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	} else if (showsystray && (c = wintosystrayicon(ev->window))) {
		/* KLUDGE! sometimes icons occasionally unmap their windows, but do
		 * _not_ destroy them. We map those windows back */
		XMapRaised(dpy, c->win);
		removesystrayicon(c);
		drawbarwin(systray->bar);
	}
}

void
updatebars(void)
{
	Bar *bar;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		for (bar = m->bar; bar; bar = bar->next) {
			if (bar->external)
				continue;
			if (!bar->win) {
				bar->win = XCreateWindow(dpy, root, bar->bx, bar->by, bar->bw, bar->bh, 0, DefaultDepth(dpy, screen),
						CopyFromParent, DefaultVisual(dpy, screen),
						CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
				XDefineCursor(dpy, bar->win, cursor[CurNormal]->cursor);
				XMapRaised(dpy, bar->win);
				XSetClassHint(dpy, bar->win, &ch);
			}
		}
	}
}

void
updatebarpos(Monitor *m)
{

	m->wx = m->mx;
	m->wy = m->my;
	m->ww = m->mw;
	m->wh = m->mh;
	Bar *bar;
	int y_pad = 0;
	int x_pad = 0;

	for (bar = m->bar; bar; bar = bar->next) {
		bar->bx = m->wx + x_pad;
		bar->bw = m->ww - 2 * x_pad;
	}

	for (bar = m->bar; bar; bar = bar->next)
		if (!m->showbar || !bar->showbar)
			bar->by = -bar->bh - y_pad;

	if (!m->showbar)
		return;
	for (bar = m->bar; bar; bar = bar->next) {
		if (!bar->showbar)
			continue;
		if (bar->topbar)
			m->wy = m->wy + bar->bh + y_pad;
		m->wh -= y_pad + bar->bh;
		bar->by = (bar->topbar ? m->wy - bar->bh : m->wy + m->wh);
	}
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);

}

int
updategeom(void)
{
	int dirty = 0;
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->isfloating = c->isfloating || c->isfixed;
	c->hintsvalid = 1;
}

void
updatetitle(Client *c)
{

	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);

}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (c->isurgent) {
			if (c->isfloating)
				XSetWindowBorder(dpy, c->win, scheme[SchemeUrg][ColFloat].pixel);
			else
				XSetWindowBorder(dpy, c->win, scheme[SchemeUrg][ColBorder].pixel);
		}
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
view(const Arg *arg)
{
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
	{
		return;
	}
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	arrange(selmon);
	focus(NULL);
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;
	Bar *bar;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		for (bar = m->bar; bar; bar = bar->next)
			if (w == bar->win)
				return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;
	if (arg && arg->v) c = (Client*)arg->v;

	if (!c) return;
	if (!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) return;

	// if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next))) return;
	pop(c);
}

void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;
	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww;

	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i);
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
			if (ty + HEIGHT(c) < m->wh)
					ty += HEIGHT(c);
		}
}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0 && n < 100) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[ %d ]", n);

	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

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
#pragma endregion dwm

#pragma region placemouse
#define INTERSECTC(X,Y,W,H,Z)   (MAX(0, MIN((X)+(W),(Z)->x+(Z)->w) - MAX((X),(Z)->x)) \
                               * MAX(0, MIN((Y)+(H),(Z)->y+(Z)->h) - MAX((Y),(Z)->y)))

Client *
recttoclient(int x, int y, int w, int h, int include_floating)
{
	Client *c, *r = NULL;
	int a, area = 1;

	for (c = selmon->stack; c; c = c->snext) {
		if (!ISVISIBLE(c) || (c->isfloating && !include_floating))
			continue;
		if ((a = INTERSECTC(x, y, w, h, c)) >= area) {
			area = a;
			r = c;
		}
	}
	return r;
}

static void
placemouse(const Arg *arg)
{
	int x, y, px, py, ocx, ocy, nx = -9999, ny = -9999, freemove = 0;
	Client *c, *r = NULL, *at, *prevr;
	Monitor *m;
	XEvent ev;
	XWindowAttributes wa;
	Time lasttime = 0;
	int attachmode, prevattachmode;
	attachmode = prevattachmode = -1;

	if (!(c = selmon->sel) || !c->mon->lt[c->mon->sellt]->arrange) /* no support for placemouse when floating layout is used */
		return;
	if (c->isfullscreen) /* no support placing fullscreen windows by mouse */
		return;
	restack(selmon);
	prevr = c;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;

	c->isfloating = 0;
	c->beingmoved = 1;

	XGetWindowAttributes(dpy, c->win, &wa);
	ocx = wa.x;
	ocy = wa.y;

	if (arg->i == 2) // warp cursor to client center
		XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, WIDTH(c) / 2, HEIGHT(c) / 2);

	if (!getrootptr(&x, &y))
		return;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);

			if (!freemove && (abs(nx - ocx) > snap || abs(ny - ocy) > snap))
				freemove = 1;

			if (freemove)
				XMoveWindow(dpy, c->win, nx, ny);

			if ((m = recttomon(ev.xmotion.x, ev.xmotion.y, 1, 1)) && m != selmon)
				selmon = m;

			if (arg->i == 1) { // tiled position is relative to the client window center point
				px = nx + wa.width / 2;
				py = ny + wa.height / 2;
			} else { // tiled position is relative to the mouse cursor
				px = ev.xmotion.x;
				py = ev.xmotion.y;
			}

			r = recttoclient(px, py, 1, 1, 0);

			if (!r || r == c)
				break;

			if ((((float)(r->y + r->h - py) / r->h) > ((float)(r->x + r->w - px) / r->w)
			    	&& (abs(r->y - py) < r->h / 2)) || (abs(r->x - px) < r->w / 2))
				attachmode = 1; // above
			else
				attachmode = 0; // below

			if ((r && r != prevr) || (attachmode != prevattachmode)) {
				detachstack(c);
				detach(c);
				if (c->mon != r->mon) {
					arrangemon(c->mon);
					c->tags = r->mon->tagset[r->mon->seltags];
				}

				c->mon = r->mon;
				r->mon->sel = r;

				if (attachmode) {
					if (r == r->mon->clients)
						attach(c);
					else {
						for (at = r->mon->clients; at->next != r; at = at->next);
						c->next = at->next;
						at->next = c;
					}
				} else {
					c->next = r->next;
					r->next = c;
				}

				attachstack(c);
				arrangemon(r->mon);
				prevr = r;
				prevattachmode = attachmode;
			}
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);

	if ((m = recttomon(ev.xmotion.x, ev.xmotion.y, 1, 1)) && m != c->mon) {
		detach(c);
		detachstack(c);
		arrangemon(c->mon);
		c->mon = m;
		c->tags = m->tagset[m->seltags];
		attach(c);
		attachstack(c);
		selmon = m;
	}

	focus(c);
	c->beingmoved = 0;

	if (nx != -9999)
		resize(c, nx, ny, c->w, c->h, 0);
	arrangemon(c->mon);

	if (!lasttime) {
		togglefloating(NULL);
	}
}

static void
moveorplace(const Arg *arg) {
	if ((!selmon->lt[selmon->sellt]->arrange || (selmon->sel && selmon->sel->isfloating)))
		movemouse(arg);
	else
		placemouse(arg);
}

#pragma endregion placemouse

#pragma region dragfact
static void
setcfact(const Arg *arg)
{
	float f;
	Client *c;

	c = selmon->sel;

	if (!arg || !c || !selmon->lt[selmon->sellt]->arrange)
		return;
	if (!arg->f)
		f = 1.0;
	else if (arg->f > 4.0) // set fact absolutely
		f = arg->f - 4.0;
	else
		f = arg->f + c->cfact;
	if (f < 0.25)
		f = 0.25;
	else if (f > 4.0)
		f = 4.0;
	c->cfact = f;
	arrange(selmon);
}

static void
dragcfact(const Arg *arg)
{
	int prev_x, prev_y, dist_x, dist_y;
	float fact;
	Client *c;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfloating) {
		resizemouse(arg);
		return;
	}
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);

	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurIronCross]->cursor, CurrentTime) != GrabSuccess)
		return;

	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);

	prev_x = prev_y = -999999;

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate_dragcfact))
				continue;
			lasttime = ev.xmotion.time;
			if (prev_x == -999999) {
				prev_x = ev.xmotion.x_root;
				prev_y = ev.xmotion.y_root;
			}

			dist_x = ev.xmotion.x - prev_x;
			dist_y = ev.xmotion.y - prev_y;

			if (abs(dist_x) > abs(dist_y)) {
				fact = (float) 4.0 * dist_x / c->mon->ww;
			} else {
				fact = (float) -4.0 * dist_y / c->mon->wh;
			}

			if (fact)
				setcfact(&((Arg) { .f = fact }));

			prev_x = ev.xmotion.x;
			prev_y = ev.xmotion.y;
			break;
		}
	} while (ev.type != ButtonRelease);

	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);

	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}
#pragma endregion dragfact

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	if (restart)
		execvp(argv[0], argv);
	return EXIT_SUCCESS;
}

