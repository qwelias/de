#include "bar_time.h"
#include <time.h>

static char time_txt[21] = "  WEK DD MON HH:MM  ";

int width_time(Bar *bar, BarArg *a)
{
    return TEXTW(time_txt) - lrpad;
}

int draw_time(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(0)]);
    return drw_text(drw, a->x, a->y, a->w, a->h, 0, time_txt, 0, 1);
}

static const char* killsleep[] = { "bash", "-c", "pkill sleep && echo 0 || echo 1", NULL };
static const char* calendar[] = { "ghostty", "--window-padding-x=20,0", "--window-padding-y=20,0", "--title=CALENDAR", "--confirm-close-surface=false", "--cursor-opacity=0", "-e", "bash", "-c", "cal -3c1; exec sleep 100", NULL };
static const char* times[] = { "ghostty", "--window-padding-x=20,0", "--window-padding-y=17,0", "--title=TIMES", "--confirm-close-surface=false", "--cursor-opacity=0", "-e", "bash", "-c", "~/script/times.sh; exec sleep 100", NULL };

int click_time(Bar *bar, Arg *arg, BarArg *a)
{
    char buf[2] = {0};
    spawn_capture(&(Arg){ .v = killsleep }, buf, 1);
    if (buf[0] == '0') return -1;

    if (arg->i == Button1) spawn(&(Arg){ .v = calendar });
    if (arg->i == Button3) spawn(&(Arg){ .v = times });
    return -1;
}

void update_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(time_txt, sizeof(time_txt), "  %a %d %b %H:%M  ", tm);
}
