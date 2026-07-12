#ifndef BAR_AUDIO_H
#define BAR_AUDIO_H

static int audio_dirty = 1;
static int width_audio(Bar *bar, BarArg *a);
static int draw_audio(Bar *bar, BarArg *a);
static int click_audio(Bar *bar, Arg *arg, BarArg *a);
static void audio_update(void);
static void audio_change(const Arg *arg);
static void audio_toggle_sink(const Arg *arg);
static void audio_toggle_source(const Arg *arg);
#endif
