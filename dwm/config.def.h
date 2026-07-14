/* See LICENSE file for copyright and license details. */
#include "patch/bar_audio.h"
#include "patch/bar_bat.h"
#include "patch/bar_loop.h"
#include "patch/bar_system_stats.h"
#include "patch/bar_bat.h"
#include <X11/X.h>
#include <X11/XF86keysym.h>
#include <X11/Xutil.h>

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

static const char *tags[] = { "1", "2", "3", "4", "5" };

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
	{ -1,        0,     BAR_ALIGN_LEFT,   width_ltsymbol,           draw_ltsymbol,          click_ltsymbol,          NULL,                    "layout" },
	{ -1,        0,     BAR_ALIGN_LEFT,   width_tags,               draw_tags,              click_tags,              hover_tags,              "tags" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  width_bat,                draw_bat,               click_bat,               NULL,                    "bat" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  width_system_stats,       draw_system_stats,      click_system_stats,      NULL,                    "bar_system_stats" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  width_audio,              draw_audio,             click_audio,             NULL,                    "bar_audio" },
	{ 'A',       0,     BAR_ALIGN_RIGHT,  width_time,               draw_time,              click_time,              NULL,                    "bar_time" },
	{  0,        0,     BAR_ALIGN_RIGHT,  width_systray,            draw_systray,           click_systray,           NULL,                    "systray" },
	{ -1,        0,     BAR_ALIGN_NONE,   width_wintitle,           draw_wintitle,          click_wintitle,          NULL,                    "wintitle" },
};
static const BarUpdateSet barupdates[] = {
	{ 3, system_stats_update, NULL, "system_stats_update" },
	{ 3, update_time, NULL, "update_time" },
	{ 3, bat_update, &bat_dirty, "bat_update" },
	{ 10, audio_update, &audio_dirty, "audio_update" },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */
static const int refreshrate_resize = 6;  /* refresh rate (per second) for client resize */
static const int refreshrate_dragmfact = 6; /* refresh rate (per second) for dragmfact */
static const int refreshrate_dragcfact = 6; /* refresh rate (per second) for dragcfact */

static const Layout speciallt = { ": : :",      gaplessgrid };
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[ ]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[    ]",      monocle },
	speciallt,
};

/* key definitions */
#define TAGKEYS(KEY,TAG) \
	{ KeyPress,    0,                              KEY,   &speciallt,   view,           {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask,                       KEY,   NULL,          view,           {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ControlMask,           KEY,   NULL,          toggleview,     {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ShiftMask,             KEY,   NULL,          tag,            {.ui = 1 << TAG} }, \
	{ KeyPress,    Mod4Mask|ControlMask|ShiftMask, KEY,   NULL,          toggletag,      {.ui = 1 << TAG} },

/* commands */
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

static const Layout *layouts_swap[] = { &layouts[2], &layouts[0] };

static const char *brighter[] = { "brightnessctl", "set", "10%+", NULL };
static const char *dimmer[]   = { "brightnessctl", "set", "10%-", NULL };
static const char *kbd_up[]   = { "brightnessctl", "--device=tpacpi::kbd_backlight", "set", "10%+", NULL };
static const char *kbd_down[] = { "brightnessctl", "--device=tpacpi::kbd_backlight", "set", "10%-", NULL };
static const char *lock[]     = { "xset", "dpms", "force", "off", NULL };
static const char *maimss[]   = { "maimpick.sh", "ss", NULL };
static const char *maimocr[]  = { "maimpick.sh", "ocr", NULL };
static const char *switchsink[]  = { "switch_sink.sh", NULL };
static const char *suspend[]  = { "systemctl", "suspend", NULL };

static const Key keys[] = {
	/* event,      modifier                      key                         layout        function                argument */
	// fn keys
	{ KeyPress,    0,                            XF86XK_AudioRaiseVolume,    NULL,         audio_change,           {.i = (int)'+' } },
	{ KeyPress,    0,                            XF86XK_AudioLowerVolume,    NULL,         audio_change,           {.i = (int)'-' } },
	{ KeyPress,    0,                            XF86XK_AudioMute,           NULL,         audio_toggle_sink,      {0} },
	{ KeyPress,    0,                            XF86XK_AudioMicMute,        NULL,         audio_toggle_source,    {0} },
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
	{ KeyPress,    0,                              XK_q,                       &speciallt,  shiftview,              { .i = -1 } },
	{ KeyPress,    0,                              XK_e,                       &speciallt,  shiftview,              { .i = +1 } },
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

