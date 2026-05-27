#include "../util.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static char bat_txt[8] = " xx ";
static unsigned int bat_color = 1;

int width_bat(Bar *bar, BarArg *a)
{
    return TEXTW(bat_txt) - lrpad;
}

int draw_bat(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(bat_color-1)]);
    return drw_text(drw, a->x, a->y, a->w, a->h, 0, bat_txt, 0, 1);
}

int click_bat(Bar *bar, Arg *arg, BarArg *a)
{
    return -1;
}

void bat_update(void) {
    FILE *f = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!f) {
        fprintf(stderr, "cannot fopen(/sys/class/power_supply/BAT0/capacity)");
        snprintf(bat_txt, 5, " EC ");
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
        fprintf(stderr, "cannot fopen(/sys/class/power_supply/BAT0/status)");
        snprintf(bat_txt, 5, " ES ");
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
        fprintf(stderr, "cannot fopen(/sys/devices/system/cpu/cpu0/cpufreq/energy_performance_preference)");
        snprintf(bat_txt, 5, " E ");
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
    if (!strncmp(status, "C", 1)) bat_color = 6;
    else if (cap < 10) bat_color = 9 && fprintf(stderr, "bat_update: TODO WARN\n");
    else if (cap < 20) bat_color = 9;
    else if (cap < 30) bat_color = 2;
    else if (!strncmp(status, "D", 1)) bat_color = 8;

    char pi[4] = ".";
    if (!strncmp(perf, "b", 1)) snprintf(pi, 3, "·");
    else if (!strncmp(perf, "pe", 2)) snprintf(pi, 2, ":");

    snprintf(bat_txt, sizeof(bat_txt),
        " %02d%s",
        (int)cap, pi
    );
}