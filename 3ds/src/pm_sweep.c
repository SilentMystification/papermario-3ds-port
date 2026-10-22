/* SWEEP (class E, port-side test tool, no game logic): the in-game part of the full sweep (docs/plans/sweep_notes.md).
 * Two modes, both log the same lines:
 *  - single item: switch sweep=<name> sweepcap=<steps> (default 1200) sweepjump=<x> sweepuntil=thankyou. The tool 3ds/tools/sweep.py starts one item per Azahar start and sets the
 *    start state with the normal switches (warp, story, autoa, shots). This file logs markers and ends the item.
 *  - chain: switch sweeplist=@sweep/list.txt. The file has one item per line (3ds/sweep/sweep.txt format with numbers only: warp=<map>:<entry> story=<n>
 *    cap=<steps> jump=<x> until=thankyou). The items run one after the other in ONE boot: story set and a real map change (pm_route_goto). For the console (one push, pulled log).
 * Boss fights are NOT automated (user decision): no battle code is in this file.
 * Log lines (3ds/tools/sweep_report.py reads them; g = game step count since boot, f = steps since the item start):
 *   sweep: <name> start g<n> kind <k> cap <n>
 *   sweep: <name> mode <m> map <area>:<map> f<n>               (at every change)
 *   sweep: <name> marker thankyou f<n> alpha <a>               (credits: THE END of kmr_30 shows, TheEndModelAlpha > 0)
 *   sweep: <name> end <cap|marker> f<n> g<n> pturn 0 eturn 0 pmove 0 ehp -1 hp <Mario HP> maps <n> (the pturn fields stay for the report format)
 *   sweep: chain end items <n>
 * The sweep changes no game state except TheEndModelAlpha (reset to 0 at the start of a credits item) and the camera x of a credits scene (jump). */
#include "common.h"
#include "functions.h"
#include "game_modes.h"
#include "map.h"
#include "pm_port.h"
#include <stdio.h>
#include <string.h>

int atoi(const char*);
long strtol(const char*, char**, int);
extern int pm_debug_value(const char* key, char* out, unsigned n);
extern unsigned pm_frame_count(void);
void pm_route_goto(const char* name, int entry);

typedef struct {
    char name[56];
    int cap, entry, has_story, jump, until_ty;
    s32 story;
    char map[16];
} Item;

static Item cur;
static int ev_done, last_mode, last_map, nmaps;
static unsigned g_start;
/* credits parade of end_00 (src/world/area_end/end_00/end_00.h PARADE_PHASE_*): the game starts each phase when the camera x passes the value. The marker only reads it. */
static const short parade_x[] = { -2845, -2550, -2150, -2090, -1500, -1355, -960, -760, -310, -53, 750, 1200, 1860, 2580 };
/* kmr_30 the_end.c: the THE END model fades in through this global (set by the game script). It is the thank-you screen. */
extern s32 TheEndModelAlpha;
static int parade_k, jumped, ty_f;
static unsigned f_base;

static void item_begin(unsigned f) {
    parade_k = 0; jumped = 0; ty_f = -1; if (cur.until_ty) TheEndModelAlpha = 0; f_base = 0; ev_done = 0; last_mode = -99; last_map = -99; nmaps = 0;
    g_start = pm_frame_count();
    pm_log("sweep: %s start g%u kind cut cap %d", cur.name, g_start, cur.cap);
}

static void item_end(const char* why, unsigned f) {
    if (ev_done) return;
    ev_done = 1;
    pm_log("sweep: %s end %s f%u g%u pturn 0 eturn 0 pmove 0 ehp -1 hp %d maps %d", cur.name, why, f, pm_frame_count(), (int)gPlayerData.curHP, nmaps);
}

/* One drawn frame of a running item. f = frames since the item start. */
static void item_step(unsigned f) {
    int mode, map, bs;
    if (ev_done) return;
    mode = (int)get_game_mode();
    map = (int)gGameStatusPtr->areaID * 1000 + (int)gGameStatusPtr->mapID;
    if (mode != last_mode || map != last_map) {
        if (map != last_map) nmaps++;
        pm_log("sweep: %s mode %d map %d:%d f%u", cur.name, mode, (int)gGameStatusPtr->areaID, (int)gGameStatusPtr->mapID, f);
        last_mode = mode;
        last_map = map;
    }
    if (strncmp(cur.name, "credits", 7) == 0 && mode == GAME_MODE_WORLD && parade_k < (int)(sizeof parade_x / sizeof parade_x[0])
        && gCameras[CAM_DEFAULT].movePos.x > (float)parade_x[parade_k] && gCameras[CAM_DEFAULT].movePos.x < 3000.0f) {
        pm_log("sweep: %s phase %d camx %d f%u", cur.name, parade_k, (int)gCameras[CAM_DEFAULT].movePos.x, f);
        parade_k++;
    }
    /* jump=<x> (credits scenes): when the first parade phase has started (the Luigi part is over), the camera moves 14 units per step to 25 units before the phase threshold x.
     * The game starts each phase when the camera passes its value. The cap counts from the end of the move. */
    if (cur.jump && !jumped && parade_k >= 1) {
        float tx = (float)(cur.jump - 25);
        if (gCameras[CAM_DEFAULT].movePos.x < tx) gCameras[CAM_DEFAULT].movePos.x += 14.0f; /* 20 times the parade speed: the phases start one after the other, as in a run */
        else { jumped = 1; f_base = f; pm_log("sweep: %s jump camx %d f%u", cur.name, (int)gCameras[CAM_DEFAULT].movePos.x, f); }
    }
    if (cur.until_ty) { /* credits: stop 90 steps (3 s) after the THE END screen first shows */
        if (ty_f < 0 && mode == GAME_MODE_WORLD && TheEndModelAlpha > 0) { ty_f = (int)f; pm_log("sweep: %s marker thankyou f%u alpha %d", cur.name, f, (int)TheEndModelAlpha); }
        if (ty_f >= 0 && (int)f - ty_f >= 90) { item_end("marker", f); return; }
    }
    if ((int)(f - f_base) >= cur.cap && (!cur.jump || jumped)) item_end("cap", f);
}

/* ---- chain mode ---- */
static char list[40000];
static const char* list_next;
static int chain_st, chain_n, chain_on;
static unsigned chain_t, chain_f0;

static int parse_line(const char* p, Item* it) {
    char tok[96];
    int n;
    memset(it, 0, sizeof *it);
    it->cap = 1200;
    for (n = 0; *p && *p != '\n' && *p != '\r'; ) {
        unsigned i = 0;
        while (*p == ' ' || *p == '\t') p++;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && i + 1 < sizeof tok) tok[i++] = *p++;
        tok[i] = 0;
        if (!i) break;
        if (n++ == 0) { strncpy(it->name, tok, sizeof it->name - 1); continue; }
        if (!strncmp(tok, "cap=", 4)) it->cap = atoi(tok + 4);
        else if (!strncmp(tok, "until=", 6)) it->until_ty = strcmp(tok + 6, "thankyou") == 0;
        else if (!strncmp(tok, "jump=", 5)) it->jump = atoi(tok + 5);
        else if (!strncmp(tok, "story=", 6)) { it->story = (s32)strtol(tok + 6, NULL, 0); it->has_story = 1; }
        else if (!strncmp(tok, "warp=", 5)) {
            char* c = strchr(tok + 5, ':');
            if (c) { *c = 0; it->entry = atoi(c + 1); }
            strncpy(it->map, tok + 5, sizeof it->map - 1);
        }
    }
    return it->name[0] != 0;
}

static void chain_tick(unsigned f) {
    if (chain_st == 9) return;
    if (chain_st == 0) {
        if (get_game_mode() != GAME_MODE_WORLD) {
            if (f - chain_t > 900 && chain_n > 0) { /* the last item left no world: go to a known map */
                pm_log("sweep: recover goto mac_00:4 at f%u mode %d", f, (int)get_game_mode());
                pm_route_goto("mac_00", 4);
                chain_t = f;
            }
            return;
        }
        if (f < chain_t) return;
        while (*list_next == '\n' || *list_next == '\r' || *list_next == ' ' || *list_next == '\t' || *list_next == '#') {
            if (*list_next == '#') while (*list_next && *list_next != '\n') list_next++;
            else list_next++;
        }
        if (!*list_next) { pm_log("sweep: chain end items %d", chain_n); chain_st = 9; return; }
        parse_line(list_next, &cur);
        while (*list_next && *list_next != '\n') list_next++;
        chain_n++;
        if (cur.has_story) evt_set_variable(NULL, GB_StoryProgress, cur.story);
        chain_f0 = f;
        item_begin(f);
        if (cur.map[0]) {
            pm_route_goto(cur.map, cur.entry);
        }
        chain_st = 1;
    } else if (chain_st == 1) {
        item_step(f - chain_f0);
        if (ev_done) {
            chain_st = 0; chain_t = f + 10;
        }
    }
}

/* Called once per drawn frame from pm_warp_tick with the scene frame f (frames after the warp). */
void pm_sweep_tick(unsigned f) {
    static int init, on;
    char v[96];
    if (!init) {
        init = 1;
        if (pm_debug_value("sweeplist", v, sizeof v) && v[0] == '@') {
            FILE* fp = fopen(v + 1, "r");
            if (fp) {
                size_t n = fread(list, 1, sizeof list - 1, fp);
                fclose(fp);
                list[n] = 0;
                list_next = list;
                chain_on = on = 1;
                chain_t = 60;
                pm_log("sweep: chain list %s, %u bytes", v + 1, (unsigned)n);
            } else pm_log_w("sweep: cannot open %s", v + 1);
        } else if (pm_debug_value("sweep", cur.name, sizeof cur.name)) {
            on = 1;
            cur.cap = 1200;
            if (pm_debug_value("sweepcap", v, sizeof v)) cur.cap = atoi(v);
            if (pm_debug_value("sweepjump", v, sizeof v)) cur.jump = atoi(v);
            if (pm_debug_value("sweepuntil", v, sizeof v)) cur.until_ty = strcmp(v, "thankyou") == 0;
            item_begin(f);
        }
    }
    if (!on) return;
    if (chain_on) chain_tick(f); else item_step(f);
}
