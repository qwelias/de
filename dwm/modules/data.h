#ifndef DATA_H
#define DATA_H

#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <stddef.h>

/* macros */
#define Button6                 6
#define Button7                 7
#define Button8                 8
#define Button9                 9
#define BARRULES                20
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define HIDDEN(C)               ((getstate(C->win) == IconicState))

/* enums */
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

enum { Manager, Xembed, XembedInfo, XLast };
/* Xembed atoms */

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
	int w[BARRULES]; // width, array length == barrules, then use r index for lookup purposes
	int x[BARRULES]; // x position, array length == ^
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

static const char* tags[] = { "1", "2", "3", "4", "5" };

/* appearance */
static const unsigned int barhextra      = 10;   /* extra bar height to font height */
static const unsigned int borderpx       = 1;   /* border pixel of windows */
static const unsigned int snap           = 32;  /* snap pixel */
static const unsigned int gappih         = 35;  /* height inner gap between windows */
static const unsigned int gappiw         = 35;  /* width inner gap between windows */
static const unsigned int gappow         = 35;  /* width outer gap between windows and screen edge */
static const unsigned int gapptop         = 35;  /* top outer gap between windows and screen edge */
static const unsigned int gappbot         = 35;  /* bot outer gap between windows and screen edge */
static const int smartgaps_fact          = 1;   /* gap factor when there is only one client; 0 = no gaps, 3 = 3x outer gaps */
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

// defined later to avoid circ deps
extern const BarRule barrules[];
extern const size_t num_barrules;
extern const BarUpdateSet barupdates[];
extern const Layout speciallt;
extern const Layout layouts[];
extern const size_t num_layouts;
extern const Layout* layouts_swap[];
extern const Key keys[];
extern const Button buttons[];

#endif //DATA_H