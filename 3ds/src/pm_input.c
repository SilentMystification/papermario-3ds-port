#include <3ds.h>
#include "pm_port.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

unsigned pm_pad_scans;
static unsigned short held;
static unsigned short down;
static unsigned short press_seen; /* presses since the last read: a tap shorter than one game frame still counts as held once */
static unsigned tap_injected, tap_seen;
static signed char stick_x;
static signed char stick_y;
static int rand_on, rand_x, rand_y; /* padrand switch */
static unsigned select_scan = 0x80000000u; /* F11-3: scan of the last Select press. A port flag, never an N64 bit. */
static int select_pending; /* F11-6: a Select press that no game code took opens the port menu */
static unsigned ps_menu_edge; /* F11-6: padscript edge of this scan, drives the port menu in tests */
static unsigned tab_scan = 0x80000000u; /* UI1a: scan of the last L or R press (3DS keys), for the Options tabs */
static int tab_dir;
int pm_pad_take_tab(void) {
    int r = pm_pad_scans - tab_scan <= 4u ? tab_dir : 0;
    tab_scan = 0x80000000u;
    return r;
}
int pm_pad_take_select(void) {
    int r = pm_pad_scans - select_scan <= 4u; /* a press older than 2 game frames is stale */
    select_scan = 0x80000000u;
    select_pending = 0;
    return r;
}

/* padscript switch (slice W01): a scripted pad. `padscript=A,wait30,right*60,right+A*20` or `padscript=@file`
 * (file in the SD app folder, tokens split by comma, space or newline). A token is `wait<n>` (nothing pressed) or buttons joined
 * with +, then an optional *<n> scan count (default 1). Buttons: A B Z START L R dup ddown dleft dright, stick: up down left right.
 * The unit is a pad scan: the game reads the pad on every second scan, so 2 scans are 1 game frame (30 Hz). With `warp` the script
 * starts on the first frame of the scene (pm_padscript_start), without it on the first scan. After the last token the real pad is back. */
#define PS_MAX 2500 /* ROUTE: was 450, a play-through route has about 700 tokens */
static struct { unsigned short b; signed char x, y; unsigned short n; unsigned keys; unsigned char mark, go; short gx, gz; } ps[PS_MAX]; /* go: token go_<x>_<z>_<scans>, walk to a world point */ /* mark: 1 + index in ps_marks, the entry takes no scan */
static char ps_marks[96][20]; /* ROUTE: names of the seg_<name> tokens */
static int ps_nmarks;
static char ps_route[24]; /* ROUTE: name of the route=<name> switch, empty without it */
extern unsigned pm_frame_count(void);
static unsigned ps_pkeys; /* UI1: 3DS keys of the script step in the last scan */
static int ps_n, ps_i, ps_run, ps_wait_start;
static unsigned ps_left;
static unsigned short ps_prev;
void pm_route_goto(const char* map, int entry);
void pm_route_note(void);
void pm_route_save(int slot);
int pm_route_aim(int tx, int tz, int* sx, int* sy);
/* ROUTE: run the seg_<name> marks at ps_i: one log line `route: seg <name> f<game step>` each, no scan used */
static void ps_run_marks(void) {
    while (ps_i < ps_n && ps[ps_i].mark) {
        if (ps[ps_i].go == 2) { pm_log("route: goto %s:%d f%u", ps_marks[ps[ps_i].mark - 1], (int)ps[ps_i].gx, pm_frame_count()); pm_route_goto(ps_marks[ps[ps_i].mark - 1], ps[ps_i].gx); }
        else if (ps[ps_i].go == 3) pm_route_save(ps[ps_i].gx);
        else pm_log("route: seg %s f%u scan %u", ps_marks[ps[ps_i].mark - 1], pm_frame_count(), pm_pad_scans);
        ps_i++;
    }
    if (ps_i < ps_n) ps_left = ps[ps_i].n;
}

/* UI1: tokens k_dup k_ddown k_dleft k_dright k_L k_R k_A k_B k_X k_Y k_START press a 3DS key before the mapping (D-pad modes, tabs) */
static unsigned ps_key(const char* t) {
    static const struct { const char* n; unsigned k; } kt[] = { { "dup", KEY_DUP }, { "ddown", KEY_DDOWN }, { "dleft", KEY_DLEFT }, { "dright", KEY_DRIGHT }, { "L", KEY_L }, { "R", KEY_R },
        { "A", KEY_A }, { "B", KEY_B }, { "X", KEY_X }, { "Y", KEY_Y }, { "START", KEY_START }, { "csup", KEY_CSTICK_UP } };
    for (unsigned i = 0; i < sizeof(kt) / sizeof(kt[0]); i++) if (strcasecmp(t, kt[i].n) == 0) return kt[i].k;
    return 0;
}

static int ps_button(const char* t, unsigned short* b, signed char* x, signed char* y) {
    static const struct { const char* n; unsigned short bit; } bt[] = {
        { "SELECT", 0x0040 }, { "A", 0x8000 }, { "B", 0x4000 }, { "Z", 0x2000 }, { "START", 0x1000 }, { "L", 0x0020 }, { "R", 0x0010 },
        { "cdown", 0x0004 }, { "dup", 0x0800 }, { "ddown", 0x0400 }, { "dleft", 0x0200 }, { "dright", 0x0100 },
    };
    for (unsigned i = 0; i < sizeof(bt) / sizeof(bt[0]); i++) if (strcasecmp(t, bt[i].n) == 0) { *b |= bt[i].bit; return 1; }
    if (strcasecmp(t, "up") == 0) { *y = 80; return 1; }
    if (strcasecmp(t, "down") == 0) { *y = -80; return 1; }
    if (strcasecmp(t, "left") == 0) { *x = -80; return 1; }
    if (strcasecmp(t, "right") == 0) { *x = 80; return 1; }
    return 0;
}

static void ps_parse(char* s) {
    char* save = NULL;
    const char* sep = ", \r\n\t";
    for (char* tok = strtok_r(s, sep, &save); tok && ps_n < PS_MAX; tok = strtok_r(NULL, sep, &save)) {
        unsigned n = 1;
        char* star = strchr(tok, '*');
        if (star) { *star = 0; n = (unsigned)atoi(star + 1); }
        unsigned short b = 0;
        unsigned keys = 0;
        signed char x = 0, y = 0;
        unsigned char mk = 0, go = 0;
        int gx = 0, gz = 0;
        if (strncmp(tok, "seg_", 4) == 0) { /* ROUTE: segment marker */
            if (ps_nmarks < 96) { strncpy(ps_marks[ps_nmarks], tok + 4, 19); mk = (unsigned char)++ps_nmarks; }
            n = 1;
        } else if (strncmp(tok, "goto_", 5) == 0) { /* ROUTE: goto_<map>:<entry> starts a real map change (as the tour switch does), takes no scan */
            char* c = strchr(tok + 5, ':');
            if (c && ps_nmarks < 96) { *c = 0; strncpy(ps_marks[ps_nmarks], tok + 5, 19); mk = (unsigned char)++ps_nmarks; go = 2; gx = atoi(c + 1); n = 1; }
            else pm_log_w("padscript: bad goto token '%s'", tok);
        } else if (strncmp(tok, "save_", 5) == 0) { /* ROUTE: save_<slot> runs fio_save_game(slot), as the save block does. Takes no scan. */
            if (ps_nmarks < 96) { strcpy(ps_marks[ps_nmarks], "save"); mk = (unsigned char)++ps_nmarks; go = 3; gx = atoi(tok + 5); n = 1; }
        } else if (strncmp(tok, "go_", 3) == 0) { /* ROUTE: go_<x>_<z>_<max scans>: stick toward the point until Mario is within 24 units */
            int gn = 0;
            if (sscanf(tok + 3, "%d_%d_%d", &gx, &gz, &gn) == 3 && gn > 0) { go = 1; n = (unsigned)gn; }
            else pm_log_w("padscript: bad go token '%s'", tok);
        } else if (strncasecmp(tok, "wait", 4) == 0) {
            n = (unsigned)atoi(tok + 4);
        } else {
            char* sv2 = NULL;
            for (char* p = strtok_r(tok, "+", &sv2); p; p = strtok_r(NULL, "+", &sv2))
                if (!strncmp(p, "k_", 2) && ps_key(p + 2)) keys |= ps_key(p + 2);
                else if (!ps_button(p, &b, &x, &y)) pm_log_w("padscript: unknown button '%s'", p);
        }
        if (n == 0) n = 1;
        ps[ps_n].mark = mk; ps[ps_n].go = go; ps[ps_n].gx = (short)gx; ps[ps_n].gz = (short)gz; ps[ps_n].keys = keys; ps[ps_n].b = b; ps[ps_n].x = x; ps[ps_n].y = y; ps[ps_n].n = (unsigned short)n;
        ps_n++;
    }
}

static void ps_load(void) {
    static char buf[24576]; /* slice S19d: was 1024, a battle script needs about 2.5 KB. ROUTE: 24 KB */
    char v[200]; /* was 40: a long inline script was cut after 39 characters */
    if (pm_debug_value("route", ps_route, sizeof ps_route)) { /* ROUTE: route=<name> reads routes/<name>.txt in the SD app folder */
        snprintf(v + 1, sizeof v - 1, "routes/%s.txt", ps_route);
        v[0] = '@';
    } else if (!pm_debug_value("padscript", v, sizeof v)) return;
    if (v[0] == '@') {
        FILE* f = fopen(v + 1, "rb");
        size_t n = f ? fread(buf, 1, sizeof(buf) - 1, f) : 0;
        if (f) fclose(f);
        buf[n] = 0;
        for (char* c = buf; *c; c++) if (*c == '#') while (*c && *c != '\n') *c++ = ' '; /* ROUTE: # starts a comment up to the end of the line */
        if (!f) pm_log_e("padscript: cannot open %s", v + 1);
    } else {
        strncpy(buf, v, sizeof buf - 1);
    }
    ps_parse(buf);
    ps_i = 0;
    ps_left = ps_n ? ps[0].n : 0;
    {
        char t[8];
        ps_wait_start = pm_debug_value("warp", t, sizeof t) || pm_debug_value("preset", t, sizeof t) || pm_debug_value("story", t, sizeof t);
    }
    ps_run = ps_n > 0 && !ps_wait_start;
    pm_log("padscript: %d steps, %d marks, starts %s%s%s", ps_n, ps_nmarks, ps_wait_start ? "in the scene" : "now", ps_route[0] ? ", route " : "", ps_route);
    if (ps_run) ps_run_marks();
}

void pm_padscript_start(void) {
    if (ps_n && !ps_run && ps_wait_start && ps_i == 0) { ps_run = 1; pm_log("padscript: start at scan %u f%u", pm_pad_scans, pm_frame_count()); ps_run_marks(); }
}

static int clamp_stick(int v) {
    v = v * 80 / 154;
    if (v > 80) v = 80;
    if (v < -80) v = -80;
    return v;
}

/* C-O3DS and F11-5: pm_map[i] is the N64 button of 3DS input i (options.txt map.*, the Select menu). Default: A, B, START as is, the D-pad and
 * X, Y and the C-stick give N64 C, L and R give Z, ZL and ZR give N64 L and R. O3DS (no ZL, ZR): with the Z key held (X, or Y by options.txt
 * o3dsz=1) L gives N64 L and R gives N64 R. SELECT is reserved for the port menu. */
int pm_new3ds;
static unsigned short prev_sh;
static const u32 in_key[PM_NIN] = { KEY_A, KEY_B, KEY_X, KEY_Y, KEY_L, KEY_R, KEY_ZL, KEY_ZR, KEY_START, KEY_DUP, KEY_DDOWN, KEY_DLEFT, KEY_DRIGHT,
                                    KEY_CSTICK_UP, KEY_CSTICK_DOWN, KEY_CSTICK_LEFT, KEY_CSTICK_RIGHT };
static unsigned short shoulder_map(u32 k) {
    if (!pm_new3ds && (k & (pm_opt.o3ds_z ? KEY_Y : KEY_X)))
        return (unsigned short)(((k & KEY_L) ? 0x0020 : 0) | ((k & KEY_R) ? 0x0010 : 0));
    return (unsigned short)(((k & KEY_L) ? pm_map[PM_IN_L] : 0) | ((k & KEY_R) ? pm_map[PM_IN_R] : 0) | ((k & KEY_ZL) ? pm_map[PM_IN_ZL] : 0) | ((k & KEY_ZR) ? pm_map[PM_IN_ZR] : 0));
}

void pm_pad_scan(void) {
    hidScanInput();
    u32 k = hidKeysHeld();
    u32 d = hidKeysDown();
    {   /* UI1: scripted 3DS keys (padscript k_ tokens), loaded here so they act before the mapping */
        static int ps_init0;
        if (!ps_init0) { ps_init0 = 1; ps_load(); }
        if (ps_run) { k |= ps[ps_i].keys; d |= ps[ps_i].keys & ~ps_pkeys; ps_pkeys = ps[ps_i].keys; } else ps_pkeys = 0;
    }
    if (d & (KEY_L | KEY_R)) { tab_scan = pm_pad_scans; tab_dir = (d & KEY_L) ? -1 : 1; }
    /* UI1c: D-pad modes. cbtn: the D-pad gives its C buttons. emu: it gives the stick. Multifunction: stick and C in every state but the free overworld. */
    const int dmode = pm_opt.dpad_mode == 2 && !pm_new3ds ? 0 : pm_opt.dpad_mode;
    const int cbtn = dmode != 2;
    const int emu = dmode == 2 || (dmode == 0 && !pm_free_overworld());
    unsigned short b = 0;
    unsigned short edge = 0;
    for (int i = 0; i < PM_NIN; i++) {
        if (i >= PM_IN_L && i <= PM_IN_ZR) continue; /* shoulders below */
        if (i >= PM_IN_DUP && i <= PM_IN_DRIGHT && (pm_map[i] & 0x000F) && !cbtn) continue; /* D-pad to analog only */
        if (k & in_key[i]) b |= pm_map[i];
        if (d & in_key[i]) edge |= pm_map[i];
    }
    unsigned short sh = shoulder_map(k);
    b |= sh;
    edge |= sh & ~prev_sh;
    prev_sh = sh;
    if ((d & KEY_SELECT) && !pm_menu_open) { select_scan = pm_pad_scans; select_pending = 1; }
    /* autoa switch: press A on two of every four scans, so Azahar runs leave the menus fast.
     * The old pattern was 1 scan in 240. */
    {
        if (pm_debug_has("autoa")) {
            if (pm_pad_scans % 4 < 2) {
                b |= 0x8000;
                edge |= 0x8000;
            }
        }
        /* taptest switch: a press that lasts one scan, once per 60 scans. Half of them land on a scan the game never reads. */
        if (pm_debug_has("taptest") && pm_pad_scans % 60 == 0) {
            b |= 0x8000;
            edge |= 0x8000;
            tap_injected++;
        }
        /* padrand switch (slice F01): a seeded random walk. The stick holds one direction for 20 to 60
         * scans, B and START and Z tap now and then. With autoa this jumps, bumps walls and walks into loading zones. */
        if (pm_debug_has("padrand")) {
            static unsigned rng = 12345, left;
            if (left == 0) {
                rng = rng * 1103515245u + 12345u;
                unsigned r = rng >> 8;
                left = 20 + r % 41;
                int dir = (r >> 6) % 9;
                rand_x = dir == 8 ? 0 : (int)(70 * __builtin_cosf(dir * 0.7853982f));
                rand_y = dir == 8 ? 0 : (int)(70 * __builtin_sinf(dir * 0.7853982f));
            }
            left--;
            rand_on = 1;
            if (pm_pad_scans % 97 < 3) { b |= 0x4000; edge |= 0x4000; }
            if (pm_pad_scans % 701 < 3) { b |= 0x1000; edge |= 0x1000; }
            if (pm_pad_scans % 211 < 3) { b |= 0x2000; edge |= 0x2000; }
        }
        {
            if (ps_run) {
                ps_menu_edge = ps[ps_i].b & ~ps_prev;
                if ((ps_menu_edge & 0x0040) && !pm_menu_open) { select_scan = pm_pad_scans; select_pending = 1; } /* SELECT token: bit 0x40 is the script's own flag */
                b |= ps[ps_i].b & ~0x0040;
                edge |= ps[ps_i].b & ~ps_prev & ~0x0040;
                ps_prev = ps[ps_i].b;
                rand_x = ps[ps_i].x;
                rand_y = ps[ps_i].y;
                rand_on = 1;
                if (ps_route[0] && pm_pad_scans % 30 == 0) pm_route_note();
                if (ps[ps_i].go) { /* ROUTE: closed loop walk */
                    int ax = 0, ay = 0;
                    if (pm_route_aim(ps[ps_i].gx, ps[ps_i].gz, &ax, &ay)) ps_left = 1;
                    rand_x = ax; rand_y = ay;
                }
                if (--ps_left == 0) {
                    ps_i++;
                    ps_run_marks();
                    if (ps_i >= ps_n) { ps_run = 0; rand_on = 0; ps_prev = 0; pm_log("padscript: done at scan %u f%u", pm_pad_scans, pm_frame_count()); if (ps_route[0]) pm_log("route: end f%u", pm_frame_count()); }
                }
            }
        }
        /* faulttest switch (slice F01): one read of address 0x10 at scan 300, to check the fault detector */
        if (pm_debug_has("faulttest") && pm_pad_scans == 300) {
            pm_log_d("faulttest read 0x10");
            pm_log_d("faulttest value %u", *(volatile unsigned*)0x10);
        }
        pm_pad_scans++;
    }
    {   /* F11-6: the port menu. A Select press that no game code took in 5 scans opens it. While open the game gets no button and no stick. */
        unsigned md = d | ((ps_menu_edge & 0x8000) ? KEY_A : 0) | ((ps_menu_edge & 0x4000) ? KEY_B : 0) | ((ps_menu_edge & 0x0800) ? KEY_DUP : 0) |
                      ((ps_menu_edge & 0x0400) ? KEY_DDOWN : 0) | ((ps_menu_edge & 0x0040) ? KEY_SELECT : 0);
        ps_menu_edge = 0;
        if (!pm_menu_open) {
            if (select_pending && pm_pad_scans - select_scan >= 5u) { select_pending = 0; pm_menu_set(1); }
        } else pm_menu_key(md);
        if (pm_menu_open) { b = 0; edge = 0; }
    }
    held = b;
    /* The game reads the pad every second retrace, the pad is scanned every retrace.
     * Keep every press edge until the game reads it. */
    down |= edge;
    press_seen |= edge;
    circlePosition c;
    hidCircleRead(&c);
    stick_x = (signed char)clamp_stick(c.dx);
    stick_y = (signed char)clamp_stick(c.dy);
    if (rand_on) {
        stick_x = (signed char)rand_x;
        stick_y = (signed char)rand_y;
    }
    if (emu) {   /* UI1c: full cardinal value, both axes for a diagonal, the circle pad wins when it is pushed further */
        int ex = ((k & KEY_DRIGHT) && (pm_map[PM_IN_DRIGHT] & 0x000F) ? 80 : 0) - ((k & KEY_DLEFT) && (pm_map[PM_IN_DLEFT] & 0x000F) ? 80 : 0);
        int ey = ((k & KEY_DUP) && (pm_map[PM_IN_DUP] & 0x000F) ? 80 : 0) - ((k & KEY_DDOWN) && (pm_map[PM_IN_DDOWN] & 0x000F) ? 80 : 0);
        if (ex && abs(ex) > abs(stick_x)) stick_x = (signed char)ex;
        if (ey && abs(ey) > abs(stick_y)) stick_y = (signed char)ey;
    }
    if (pm_menu_open) stick_x = stick_y = 0;
}

void pm_pad_read(unsigned short* buttons, signed char* x, signed char* y, unsigned short* trigger) {
    if (buttons) *buttons = held | press_seen;
    if (x) *x = stick_x;
    if (y) *y = stick_y;
    if (trigger) *trigger = down;
    if ((down & 0x8000) && pm_debug_has("taptest")) {
        tap_seen++;
        if (tap_seen % 5 == 0) pm_log_d("taptest injected %u seen %u", tap_injected, tap_seen);
    }
    down = 0;
    press_seen = 0;
}
