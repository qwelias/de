#include "bar_time.h"
#include <time.h>

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

int width_time(Bar *bar, BarArg *a)
{
    return time_width*fonth;
}

int draw_time(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(0)]);

    int lpad = (time_width*fonth - drw_fontset_getwidth(drw, time_txt, 0)) / 2;
    return drw_text(drw, a->x, a->y, a->w, a->h, lpad, time_txt, 0, 1);
}

int click_time(Bar *bar, Arg *arg, BarArg *a)
{
    if (killpopups()) return -1;

    if (arg->i == Button1) spawn(&(Arg){ .v = calendar });
    if (arg->i == Button3) spawn(&(Arg){ .v = times });
    return -1;
}

void update_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(time_txt, sizeof(time_txt), "  %a %d %b %H:%M  ", tm);
}