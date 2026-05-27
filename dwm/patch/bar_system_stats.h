#ifndef BAR_SYSTEM_STATS_H
#define BAR_SYSTEM_STATS_H

#include <stdint.h>

typedef struct CpuStat {
    uint64_t total;
    uint64_t idle;
} CpuStat;

static int width_system_stats(Bar *bar, BarArg *a);
static int draw_system_stats(Bar *bar, BarArg *a);
static int click_system_stats(Bar *bar, Arg *arg, BarArg *a);
static void system_stats_update(void);

#endif