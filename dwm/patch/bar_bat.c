#include "../util.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

static double bat_width = 2.3;
static char bat_txt[8] = "XX";
static unsigned int bat_color = 1;
static unsigned int bat_perfi = 0;
static char wasdischarging = 0;

static void
setpower(char* mode, char* notif) {
    spawn(&(Arg){ .v = (char*[]){ "powerprofilesctl", "set", mode, NULL } });
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-h", "STRING:x-dunst-stack-tag:power", "-u", "low", notif, NULL } });
}

int width_bat(Bar *bar, BarArg *a)
{
    if (!bat_txt[0]) return 0;
    return bat_width*fonth;
}

int draw_bat(Bar *bar, BarArg *a)
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

int click_bat(Bar *bar, Arg *arg, BarArg *a)
{
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

    if (wasdischarging != isdischarging) {
        if (isdischarging) setpower("power-saver", "󱐋 power-saver");
        else setpower("balanced", "󱐋 balanced");
        wasdischarging = isdischarging;
    }

    bat_perfi = 0;
    if (!strncmp(perf, "b", 1)) bat_perfi = 1;
    else if (!strncmp(perf, "pe", 2)) bat_perfi = 2;

    snprintf(bat_txt, sizeof(bat_txt),
        "%02d",
        (int)cap
    );
}