#include "bar_system_stats.h"
#include "../util.h"

#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CpuStat precpustat = {0};
static CpuStat curcpustat = {0};
static char system_stats_txt[12] = "  00 ~ 00  ";
static unsigned int system_stats_color = 1;

int width_system_stats(Bar *bar, BarArg *a)
{
    return TEXTW(system_stats_txt) - lrpad;
}

int draw_system_stats(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(system_stats_color-1)]);
    return drw_text(drw, a->x, a->y, a->w, a->h, 0, system_stats_txt, 0, 1);
}

int click_system_stats(Bar *bar, Arg *arg, BarArg *a)
{
    return -1;
}

static int read_cpu_stat(CpuStat *out) {
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

static int calculate_cpu_usage_percent(
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

static int read_mem_usage_percent(void) {
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

void system_stats_update(void) {
    read_cpu_stat(&curcpustat);
    const int cpu = calculate_cpu_usage_percent(&precpustat, &curcpustat);
    precpustat = curcpustat;
    
    const int mem = read_mem_usage_percent();
    system_stats_color = 1;
    if (mem > 90) system_stats_color = 9;
    else if (mem > 85) system_stats_color = 2;

    snprintf(system_stats_txt, sizeof(system_stats_txt),
        "  %02d ~ %02d  ",
        cpu, mem
    );
}