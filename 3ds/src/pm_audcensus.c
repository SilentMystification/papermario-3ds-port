#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "pm_port.h"
#include "pm_dspaud.h"

/* AUDDROP census (switch audiostat). Counts every event where a sound is cut, dropped or late on its way to the speaker: render side
 * (pm_dspaud.c, dsp_voice.c), control side (sfx_player.c, engine.c) and the block pace. Event paths only, cold, no heap. The counters
 * are plain: a race between the game, audio and render threads costs at most one count. One line per stats window (pm_dspa_stats_log),
 * and the first 48 events of the run as single lines. */

#define AC_IDS 32
#define AC_FIRST 48
#define AC_TOP 12

typedef struct { unsigned id; unsigned reason, n; } AcId;

extern unsigned pm_frame_n; /* pm_main.c: the logic step counter */

static const char* const ac_name[AC_N] = { "unheard", "squeezed", "early", "nochan", "giveup", "loopcap", "nonadpcm", "qovf", "merged", "noplayer", "stolen" };
static unsigned ac_n[AC_N];
static AcId ac_ids[AC_IDS];
static unsigned ac_first, ac_win;
static unsigned ac_catch, ac_run, ac_run_max, ac_late_max, ac_zero;

__attribute__((cold, noinline)) void pm_aud_ev(unsigned reason, unsigned id) {
    if (reason >= AC_N || !PMDL("audiostat")) return;
    ac_n[reason]++;
    int free_i = -1;
    for (int i = 0; i < AC_IDS; i++) {
        if (ac_ids[i].n && ac_ids[i].id == id && ac_ids[i].reason == reason) { ac_ids[i].n++; goto logged; }
        if (!ac_ids[i].n && free_i < 0) free_i = i;
    }
    if (free_i >= 0) { ac_ids[free_i].id = id; ac_ids[free_i].reason = reason; ac_ids[free_i].n = 1; }
logged:
    if (ac_first < AC_FIRST) {
        ac_first++;
        pm_log_d("audio drop ev %u: frame %u map %s id %08x reason %s", ac_first, pm_frame_n, pm_map_now(), id, ac_name[reason]);
    }
}

__attribute__((cold, noinline)) void pm_aud_catchup(unsigned late_us) {
    ac_catch++;
    ac_run++;
    if (ac_run > ac_run_max) ac_run_max = ac_run;
    if (late_us > ac_late_max) ac_late_max = late_us;
}

__attribute__((cold, noinline)) void pm_aud_run_end(void) { ac_run = 0; }

__attribute__((cold, noinline)) void pm_aud_zero_step(void) { ac_zero++; }

__attribute__((cold, noinline)) void pm_aud_window_log(unsigned decode_max_us) {
    char top[400] = "";
    unsigned len = 0;
    AcId cp[AC_IDS];
    memcpy(cp, ac_ids, sizeof cp);
    for (int k = 0; k < AC_TOP; k++) { /* the AC_TOP largest counts, in order */
        int best = -1;
        for (int i = 0; i < AC_IDS; i++)
            if (cp[i].n && (best < 0 || cp[i].n > cp[best].n)) best = i;
        if (best < 0) break;
        int w = snprintf(top + len, sizeof top - len, "%s%08x %s %u", k ? " " : "", cp[best].id, ac_name[cp[best].reason], cp[best].n);
        if (w < 0 || (unsigned)w >= sizeof top - len) break;
        len += (unsigned)w;
        cp[best].n = 0;
    }
    ac_win++;
    pm_log_d("audio drops w%u: unheard %u squeezed %u early %u nochan %u giveup %u loopcap %u nonadpcm %u qovf %u merged %u | engine noplayer %u stolen %u | catchup blocks %u maxrun %u late_us_max %u decode_max_us %u zero_tick_steps %u | ids: %s",
             ac_win, ac_n[AC_UNHEARD], ac_n[AC_SQUEEZED], ac_n[AC_EARLYEND], ac_n[AC_NOCHAN], ac_n[AC_GIVEUP], ac_n[AC_LOOPCAP], ac_n[AC_NONADPCM],
             ac_n[AC_QOVF], ac_n[AC_MERGED], ac_n[AC_NOPLAYER], ac_n[AC_STOLEN], ac_catch, ac_run_max, ac_late_max, decode_max_us, ac_zero, top);
    memset(ac_n, 0, sizeof ac_n);
    memset(ac_ids, 0, sizeof ac_ids);
    ac_catch = ac_run_max = ac_late_max = ac_zero = 0;
}
