#include "./bar_audio.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double audio_width = 5;
static char audio_txt[15] = " X X 000  ";
static unsigned int audio_color = 1;

static const char* wiremix[] = { "ghostty", "--title=WIREMIX", "--confirm-close-surface=false", "-e", "wiremix", NULL };
static const char* wpctlsink[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@", NULL };
static const char* wpctlsinkm[] = { "wpctl", "set-mute", "@DEFAULT_AUDIO_SINK@", "toggle", NULL };
static const char* wpctlsinki[] = { "wpctl", "inspect", "@DEFAULT_AUDIO_SINK@", NULL };
static const char* wpctlsource[] = { "wpctl", "get-volume", "@DEFAULT_AUDIO_SOURCE@", NULL };
static const char* wpctlsourcei[] = { "wpctl", "inspect", "@DEFAULT_AUDIO_SOURCE@", NULL };
static const char* wpctlsourcem[] = { "wpctl", "set-mute", "@DEFAULT_AUDIO_SOURCE@", "toggle", NULL };

static int
killwiremix(void) {
    Client* c = selmon->clients;
    while (c) {
        if (strstr(c->name, "WIREMIX")) {
            killclient(&(Arg){ .v = c });
            return 1;
        }
        c = c->next;
    }

    return 0;
}

int width_audio(Bar *bar, BarArg *a)
{
    return audio_width*fonth;
}

int draw_audio(Bar *bar, BarArg *a)
{
    drw_setscheme(drw, scheme[(unsigned int)(audio_color-1)]);

    int lpad = (audio_width*fonth - drw_fontset_getwidth(drw, audio_txt, 0)) / 2;
    return drw_text(drw, a->x, a->y, a->w, a->h, lpad, audio_txt, 0, 1);
}

static void readvolmute(char* buf, double* vol, int* mute) {
    // fprintf(stderr, "readvolmute: %2s\n", buf+13);
    if (vol) *vol = strtod(buf+8, NULL);
    if (mute) *mute = !strncmp(buf+13, "[M", 2);
}

void audio_update(void) {
    // fprintf(stderr, "audio_update\n");
    char buf[16] = {0};
    double sinkvol = 0;
    int sinkm = 0;
    int sourcem = 0;

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, sizeof(buf) - 1);
    readvolmute(buf, &sinkvol, &sinkm);
    memset(buf, 0, sizeof(buf));
    // fprintf(stderr, "audio_update: sink %d %s\n", sinkm, buf);

    spawn_capture(&(Arg){ .v = wpctlsource }, buf, sizeof(buf) - 1);
    readvolmute(buf, NULL, &sourcem);
    // fprintf(stderr, "audio_update: source %d %s\n", sourcem, buf);

    snprintf(audio_txt, sizeof(audio_txt), 
        "%s%s%03d",
        sourcem ? " " : " ",
        sinkm ? "󰝟 " : "󰕾 ",
        (int)(sinkvol * 100)
    );
}

static void parse_name(char* out, char* name, unsigned int len) {
    char* left = NULL;
    char* right = NULL;

    left = strstr(out, "device.profile.description = ");
    if (left) left += 30;
    else {
        left = strstr(out, "node.description = ");
        if (left) left += 20;
    }

    right = left ? strstr(left, "\"") : NULL;
    if (!left || !right) {
        strcpy(name, "???");
    } else {
        strncpy(name, left, right - left);
        name[right - left + 1] = '\0';
    }
}

void audio_change(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    double vol = 0;

    // fprintf(stderr, "audio_change\n");
    sprintf(buf, 
        "5%%%c",
        (char)arg->i
    );
    spawn_capture(&(Arg){ .v = (char*[]){ "wpctl", "set-volume","-l", "1.5", "@DEFAULT_AUDIO_SINK@", buf, NULL } }, buf, 1);

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, 15);
    readvolmute(buf, &vol, NULL);

    spawn_capture(&(Arg){ .v = wpctlsinki }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    vol = vol * 100 / 3 * 2;
    sprintf(buf, 
        "INT:value:%d",
        (int)vol
    );
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        "-h", buf,
        (char)arg->i == '+' ? "󰕾 +++++                  |" : "󰕾 -----                  |",
        name, NULL
    } });
}

void audio_toggle_sink(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    double vol = 0;
    int mute = 0;

    spawn_capture(&(Arg){ .v = wpctlsinkm }, buf, 1);
    spawn_capture(&(Arg){ .v = wpctlsinki }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    spawn_capture(&(Arg){ .v = wpctlsink }, buf, 15);
    readvolmute(buf, &vol, &mute);
    // fprintf(stderr, "audio_toggle_sink: %d %s\n", mute, buf);
    if (mute) {
        spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
            "󰝟 XXXXX", name, NULL
        } });
        return;
    }

    vol = vol * 100 / 3 * 2;
    sprintf(buf, 
        "INT:value:%d",
        (int)vol
    );
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        "-h", buf,
        "󰕾 )))))                  |",
        name, NULL
    } });
}

void audio_toggle_source(const Arg *arg) {
    audio_dirty = 1;
    char buf[4096] = {0};
    char name[64] = {0};
    int mute = 0;

    spawn_capture(&(Arg){ .v = wpctlsourcem }, buf, 1);

    spawn_capture(&(Arg){ .v = wpctlsourcei }, buf, sizeof(buf)-1);
    parse_name(buf, name, sizeof(name)-1);

    spawn_capture(&(Arg){ .v = wpctlsource }, buf, 15);
    readvolmute(buf, NULL, &mute);
    // fprintf(stderr, "audio_toggle_source: %d %s\n", mute, buf);
    spawn(&(Arg){ .v = (char*[]){ "notify-send", "-u","low", "-h", "STRING:x-dunst-stack-tag:volume",
        mute ? " XXXXX" : " (((((",
        name, NULL
    } });
}

int click_audio(Bar *bar, Arg *arg, BarArg *a)
{
    audio_dirty = 1;
    if (arg->i == Button1) audio_toggle_source(NULL);
    else if (arg->i == Button2) {
        if (killwiremix()) return -1;
        spawn(&(Arg){ .v = wiremix });
    }
    else if (arg->i == Button3) audio_toggle_sink(NULL);
    else if (arg->i == Button4) audio_change(&(Arg){ .i = (int)'+' });
    else if (arg->i == Button5) audio_change(&(Arg){ .i = (int)'-' });
    return -1;
}