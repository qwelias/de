#include "bar_time.h"

#include <time.h>

static char time_txt[21] = "  WEK DD MON HH:MM";

int width_time(Bar *bar, BarArg *a)
{
    return TEXTW(time_txt);
}

int draw_time(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(0)]);
    return drw_text(drw, a->x, a->y, a->w, a->h, 0, time_txt, 0, 1);
}

int click_time(Bar *bar, Arg *arg, BarArg *a)
{
    return -1;
}

void update_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    strftime(time_txt, sizeof(time_txt), "  %a %d %b %H:%M", tm);
}
