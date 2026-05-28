#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char audio_txt[16] = "  X X 000  ";
static unsigned int audio_color = 1;

int width_audio(Bar *bar, BarArg *a)
{
    return TEXTW(audio_txt) - lrpad;
}

int draw_audio(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(audio_color-1)]);
    return drw_text(drw, a->x, a->y, a->w, a->h, 0, audio_txt, 0, 1);
}

int click_audio(Bar *bar, Arg *arg, BarArg *a)
{
    if (arg->i == Button1) spawn(&(Arg){ .v = mute_mic });
    else if (arg->i == Button3) spawn(&(Arg){ .v = mute_vol });
    else if (arg->i == Button4) spawn(&(Arg){ .v = up_vol });
    else if (arg->i == Button5) spawn(&(Arg){ .v = down_vol });
    return -1;
}

static const char* wpctlsink[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL };
static const char* wpctlsource[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SOURCE@", NULL };
static const char* icons[] = { " "," ","󰕾 ","󰝟 " };

void audio_update(void)
{
    char sink[16] = {0};
    char source[16] = {0};
    double sinkvol = 0;
    int sinkm = 0;
    int sourcem = 0;
    char* strp;

    spawn_capture(&(Arg){ .v = wpctlsink }, sink, sizeof(sink));
    sink[12] = '\n';
    sinkvol = strtod(sink+8, &strp);
    if (!strncmp(sink+13, "[MU", 3)) sinkm = 1;

    spawn_capture(&(Arg){ .v = wpctlsource }, source, sizeof(source));
    if (!strncmp(source+13, "[MU", 3)) sourcem = 1;

    snprintf(audio_txt, sizeof(audio_txt), 
        "  %s%s%03d  ",
        icons[sourcem], icons[sinkm+2], (int)(sinkvol * 100)
    );
}